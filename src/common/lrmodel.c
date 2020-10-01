#include "src/common/lrmodel.h"

double* model_speed(double bw_threshold, int* target_info, int* interf_apps_index, int* interf_apps_nodes){
    int *interf_rwratio;
    double* interf_bw;
    double *res, res_speed, max_bw = 0.0;
    int low_rw = 100, nodes = 0, is_interf_local;
    int app_index, app_nodes, interf_n, is_target_local;

    app_index = target_info[0];
    app_nodes = target_info[1];
    interf_n = target_info[2];

    interf_rwratio = (int*) malloc(interf_n*sizeof(int));
    interf_bw = (double*) malloc(interf_n*sizeof(double));
    //res will be {speed,max_bw,interf_nodes} for sake of logging
    res = (double*) malloc(3*sizeof(double));

    is_target_local = target_info[3];
    // if target is local, means the interf apps are interfering remotely
    is_interf_local = (is_target_local) ? 0 : 1;

    // reading target app local/remote memory ratio speed
    //res[1] = read_app_remote_ratio(app_index,app_nodes);

    // Get the rw ratio and bw from the interfering applications
    for(int i = 0; i < interf_n; i++){
        //the function will read the bw and rw values, if we don't have the interfering proc we interpolate the values
        interfering_bw_rw(interf_apps_index[i],interf_apps_nodes[i],i,is_interf_local,interf_rwratio,interf_bw);
    }

    //Getting the highest bw and lowest RW from the interfering apps
    for(int i=0;i<interf_n;i++){
        if(interf_bw[i] > max_bw)
            max_bw = interf_bw[i];
        if(interf_rwratio[i] < low_rw)
            low_rw = interf_rwratio[i];
    }

    // Sum proc based on a threshold
    for(int i=0;i<interf_n;i++){
        if(interf_bw[i] >= max_bw*bw_threshold)
            nodes += interf_apps_nodes[i];
    }
    
    debug5("%s %d %f %d",__func__,nodes,max_bw,low_rw);

    free(interf_rwratio);
    free(interf_bw);

    debug5("%s after free interf_rwratio and interf_bw",__func__);

    res_speed = speed(app_index,app_nodes,max_bw,low_rw,nodes,is_target_local);

    // making sure that the highest speed is 1.
    res[0] = (res_speed > 1.0) ? 1.0 : res_speed;
    res[1] = max_bw;
    res[2] = nodes;
 
    return res;
}


double speed(int app_index, int app_proc, double max_bw, int low_rw, int nodes, int is_local){

    int N = 2;
    double pred50,pred100,w_100,speed;
    double x100[5];
    double y100[5];
    double x50[5];
    double y50[5];
    
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xx = 0.0; // X^2
    double sum_xy = 0.0; // X*Y    
    double m50 = 0.0; // Slope
    double b50 = 0.0; //Intercept
    double m100 = 0.0; // Slope
    double b100 = 0.0; //Intercept

    debug5("Entering %s",__func__);

    // This is because the remote sensitivity curve has only 2 points
    if(is_local)
        N = 5;

    read_sensitivity_curve(app_index,app_proc,nodes,is_local,x50,y50,x100,y100);

    for(int i=0; i<N; i++){
        debug5("%s %d - %f %.7f - %f %.7f",__func__,i,x50[i],y50[i],x100[i],y100[i]);
    }

    //Read bw curves
    //Create linear function of app/rwratio
    sum_x = sum(x50,N);
    sum_y = sum(y50,N);    
    sum_xx = sum2v(x50,x50,N);
    sum_xy = sum2v(x50,y50,N);

    m50 = slope(sum_x,sum_y,sum_xx,sum_xy,N);
    b50 = intercept(m50,sum_x,sum_y,N);

    sum_x = sum(x100,N);
    sum_y = sum(y100,N);    
    sum_xx = sum2v(x100,x100,N);
    sum_xy = sum2v(x100,y100,N);

    m100 = slope(sum_x,sum_y,sum_xx,sum_xy,N);
    b100 = intercept(m100,sum_x,sum_y,N);

    //Predict the value
    pred50 = predict(max_bw,m50,b50);
    pred100 = predict(max_bw,m100,b100);

    w_100=(low_rw-50.0)/(100.0-50.0);
    debug5("%s %d %.5f",__func__,low_rw,w_100);
    speed = ((pred50)*(1-w_100)+(pred100)*(w_100));

    return speed;
}

double read_app_remote_ratio(int target_, int target_procs){
    FILE *fp = NULL;
    char line[1024];
    char *id_, *r_, *nodes_;
    int id, nodes, has_info = 0, lb = 1, ub  = 1;
    double ratio = 1.0, ratio_, lb_ratio, ub_ratio;
    int list_size, *list_nodes = NULL;

    list_size = list_of_nodes(target_, &list_nodes);

    has_info = boundary(target_procs,list_size,list_nodes,&lb,&ub);

    fp = fopen("/home/bscuser/Dropbox/BSC_UPC/SLURM_SIMULATOR/workload_traces/extending_trace/mpi/app_info_list_ratio.csv", "r");

    while (fgets(line, 1024, fp))
    {
        id_ = strdup(line);
        id = atoi(getfield(id_, 2));
        nodes_ = strdup(line);
        nodes = atoi(getfield(nodes_, 4));
        if((id == target_)){
            r_ = strdup(line);
            ratio_ = atof(getfield(r_, 9));

            if(nodes == target_procs){            
                ratio = ratio_;
            }else{
                if(lb != ub){
                    if(nodes == lb){
                        lb_ratio = ratio_;
                    }
                    if(nodes == ub){
                        ub_ratio = ratio_;
                    }
                }else{
                    if(nodes == lb){
                        lb_ratio = ratio_;
                        ub_ratio = lb_ratio;
                    }
                }
            }
            free(r_);
        }
        free(id_);
        free(nodes_);
    }

    if(!has_info){
        double w_ub=(lb == ub) ? 1: ((float)target_procs-(float)lb)/((float)ub-(float)lb);
        double w_lb=(1.0-w_ub);
        ratio = MODEL_MIN(lb_ratio,ub_ratio);
    }
    
    debug5("%s - %d %d %.5f",__func__,target_,target_procs,ratio);

    free(list_nodes);
    fclose(fp);

    return ratio;
}

void interfering_bw_rw(int interf_, int interf_procs, int idx, int is_local, int *interf_rwratio, double *interf_bw){
    FILE *fp = NULL;
    char line[1024];
    char *id_, *rw_, *bw_, *nodes_;
    int id, nodes, has_info = 0, lb = 1, ub  = 1;
    int rw_ratio = 100, rw_lb_ratio, rw_ub_ratio, tmp_ratio;
    double bw = 1.0, bw_lb, bw_ub, tmp_bw;
    int list_size, *list_nodes = NULL;

    list_size = list_of_nodes(interf_, &list_nodes);

    has_info = boundary(interf_procs,list_size,list_nodes,&lb,&ub);

    fp = fopen("/home/bscuser/Dropbox/BSC_UPC/SLURM_SIMULATOR/workload_traces/extending_trace/mpi/app_info_list_ratio.csv", "r");

    while (fgets(line, 1024, fp))
    {
        id_ = strdup(line);
        id = atoi(getfield(id_, 2));
        nodes_ = strdup(line);
        nodes = atoi(getfield(nodes_, 4));
        if((id == interf_)){
            rw_ = strdup(line);
            bw_ = strdup(line);
            if(is_local)
                tmp_bw = atof(getfield(bw_, 7));
            else
                tmp_bw = atof(getfield(bw_, 6));
            
            tmp_ratio = atoi(getfield(rw_, 8));

            if(nodes == interf_procs){            
                rw_ratio = tmp_ratio;
                bw = tmp_bw;
            }else{
                if(lb != ub){
                    if(nodes == lb){
                        rw_lb_ratio = tmp_ratio;
                        bw_lb = tmp_bw;
                    }
                    if(nodes == ub){
                        rw_ub_ratio = tmp_ratio;
                        bw_ub = tmp_bw;
                    }
                }else{
                    if(nodes == lb){
                        rw_lb_ratio = tmp_ratio;
                        bw_lb = tmp_bw;
                        rw_ub_ratio = rw_lb_ratio;
                        bw_ub = bw_lb;
                    }
                }
            }
            free(rw_);
            free(bw_);
        }
        free(id_);
        free(nodes_);
    }

    if(!has_info){
        double w_ub=(lb == ub) ? 1: ((float)interf_procs-(float)lb)/((float)ub-(float)lb);
        double w_lb=(1.0-w_ub);
        debug5("%s interf[%d] - %d %d %.5f %.5f",__func__,interf_procs,lb,ub,w_ub,w_lb);
        bw=(bw_lb*w_lb + bw_ub*w_ub)/(w_lb+w_ub);
        rw_ratio = MODEL_MIN(rw_lb_ratio,rw_ub_ratio);
    }

    debug5("%s x - %d %d %f %d",__func__,interf_,interf_procs,bw,rw_ratio);

    interf_rwratio[idx] = rw_ratio;
    interf_bw[idx] = bw;

    free(list_nodes);
    fclose(fp);
}

void swap(int* xp, int* yp) 
{ 
    int temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 
  
void selectionSort(int *arr, int n) 
{ 
    int i, j, min_idx; 
  
    for (i = 0; i < n - 1; i++) {   
        min_idx = i; 
        for (j = i + 1; j < n; j++) 
            if (arr[j] < arr[min_idx]) 
                min_idx = j; 
  
        swap(&arr[min_idx], &arr[i]); 
    } 
} 

int list_of_nodes(int app_, int **list_procs){
    char line[1024];
    char *id_,*proc_;
    int id,proc;
    int list_count=0, last_seen=0, included;
    FILE *fp = NULL;

    fp = fopen("/home/bscuser/Dropbox/BSC_UPC/SLURM_SIMULATOR/workload_traces/extending_trace/mpi/app_info_list_ratio.csv", "r");
    
    while (fgets(line, 1024, fp))
    {
        id_ = strdup(line);
        id = atoi(getfield(id_, 2));
        proc_ = strdup(line);
        proc = atoi(getfield(proc_, 4));
        if(id == app_){
            list_count++;
            *list_procs = (int *)realloc(*list_procs,sizeof(int)*list_count);
            (*list_procs)[list_count-1]=proc;       
        }
        free(id_);
        free(proc_);
    }
    // ordering
    selectionSort(*list_procs,list_count);

    fclose(fp);

    return list_count;
}

int boundary(int proc_value, int list_size, int *list, int *lb, int* ub){
    int has_value = 0;
    
    for(int i =0; i < list_size; i++){
        if(list[i] == proc_value){
            has_value = 1;
            break;
        }
        if(list[i] < proc_value)
            *lb = list[i];
        //we issue a break here to get the upper bound near the curve we want
        if(list[i] > proc_value){
            *ub = list[i];
            break;
        }
    }

    //checking upper bound
    if(*ub < *lb) *ub = *lb;

    return has_value;
}

double sum(double *vector, int size)
{
    double var = 0.0;
    for(int i = 0; i < size; i++){
        var += vector[i];
    }
    return var;
}

double sum2v(double *vector1, double *vector2, int size)
{
    double var = 0.0;
    for(int i = 0; i < size; i++){
        var += (vector1[i]*vector2[i]);
    }
    return var;
}

double slope(double sx, double sy, double sxx, double sxy, int size)
{
    return ((size*sxy)-(sx*sy))/((size*sxx)-(sx*sx));
}

double intercept(double m,double sx,double sy,int size)
{
    return (sy-(m*sx))/size;
}

double predict(double x_,double m,double b)
{
    return (m*x_)+b;
}

const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ",");
            tok && *tok;
            tok = strtok(NULL, ",\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

void read_sensitivity_file(FILE* stream, int app_, int app_proc, int interf_nodes, int is_local, double* x50,double* y50,
            double* x100,double* y100){
    char line[1024];
    int i50 = 0, i100 = 0;
    char *id_, *x_, *y_, *interf_, *tmp, *proc_, *local_;
    int id,read_level,interf,proc,local;
    double x,y;

    while (fgets(line, 1024, stream))
    {
        id_ = strdup(line);
        id = atoi(getfield(id_, 6));
        interf_ = strdup(line);
        interf = atoi(getfield(interf_, 7));
        proc_ = strdup(line);
        proc = atoi(getfield(proc_, 3));
        local_ = strdup(line);
        local = atoi(getfield(local_, 8));

        
        if((id == app_) && (interf == interf_nodes)
            && (proc == app_proc) && (local == is_local)){
            tmp = strdup(line);
            x_ = strdup(line);
            y_ = strdup(line);
            
            read_level = atoi(getfield(tmp, 1));
            x = atof(getfield(x_, 4));
            y = atof(getfield(y_, 5));
            
            if(read_level == 50){
                x50[i50] = x;
                y50[i50] = y;
                i50++;
            }
            else{
                x100[i100] = x;
                y100[i100] = y;
                i100++;
            }
            free(tmp);
            free(x_);
            free(y_);
        }
        free(id_);
        free(interf_);
        free(proc_);
        free(local_);
    }
}


void read_sensitivity_curve(int app_, int app_proc, int interf_nodes, int is_local, double* x50,double* y50,
            double* x100,double* y100)
{
    FILE* file = NULL;
    int has_curve = 0, to_extrapolate = 1;
    int lb = 1, ub = 1, ratio = 1, list_size;
    int aux_app_proc = 0, real_interf_nodes = 0;
    int  ninterf[4] = {0}, *list_procs = NULL;
    int N = 2;
    double x,y,w_lb,w_ub;
    double xlb100[5],xub100[5];
    double ylb100[5],yub100[5];
    double xlb50[5],xub50[5];
    double ylb50[5],yub50[5];

    // This is because the remote sensitivity curve has only 2 points
    if(is_local)
        N = 5;

    file = fopen("/home/bscuser/Dropbox/BSC_UPC/SLURM_SIMULATOR/workload_traces/extending_trace/mpi/curves_final_local_remote_sensitivity.csv", "r");

    //check whether the file has the real app_proc info or whether it is necessary extrapolate
    list_size = list_of_nodes(app_,&list_procs);

    for (int i = 0; i<list_size;i++){
        if(list_procs[i] == app_proc)
            to_extrapolate = 0;
    }

    //Before reading the right curve, check if the number of reported interf nodes
    //is higher than the target's number of nodes. Round it to the number of target's nodes
    //it is necessary because we use the number of nodes of the interf app when its bw is above the threshold
    real_interf_nodes = MODEL_MIN(app_proc,interf_nodes);


    if(to_extrapolate){//extrapolate the curve when the conversion of trace proc and apps procs is different
        //if trace procs > any app_proc info we have, we use the highest interf we have 
        //if trace proc is within the range we have, we interpolate the curve
        if(app_proc>list_procs[list_size-1]){
            int higher_curve_proc = list_procs[list_size-1];
            debug5("%s if-extrapolating[%d] - %d %d",__func__,app_proc,higher_curve_proc,higher_curve_proc);
            read_sensitivity_file(file,app_,higher_curve_proc,higher_curve_proc,is_local,x50,y50,x100,y100);
        }
        else{
            boundary(app_proc,list_size,list_procs,&lb,&ub);
            //using as the upperbound the highest curve interf (simplicity)
            //here we could also apply an interpolation regarding the number of interfering 
            //nodes for each target curve
            read_sensitivity_file(file,app_,lb,lb,is_local,xlb50,ylb50,xlb100,ylb100);
            rewind(file);
            read_sensitivity_file(file,app_,ub,ub,is_local,xub50,yub50,xub100,yub100);

            w_ub=((float)app_proc-(float)lb)/((float)ub-(float)lb);
            w_lb=(1.0-w_ub);
            debug5("%s else-extrapolating[%d] - %d %d %.5f %.5f %f",__func__,app_proc,lb,ub,w_ub,w_lb,(((float)app_proc-(float)lb)/((float)ub-(float)lb)));
            for(int i=0;i<N;i++){
                x50[i]=(xlb50[i]*w_lb + xub50[i]*w_ub)/(w_lb+w_ub);
                y50[i]=(ylb50[i]*w_lb + yub50[i]*w_ub)/(w_lb+w_ub);
                x100[i]=(xlb50[i]*w_lb + xub100[i]*w_ub)/(w_lb+w_ub);
                y100[i]=(ylb100[i]*w_lb + yub100[i]*w_ub)/(w_lb+w_ub);
            }
        }
    }
    else{
        //here for the target application we have the right number of procs collected
        //Get the number of interf_curves
        //We just collected 4 examples varying the number of interf nodes. modify this part if the file changes
        //In our tests we just collected the curves for the app using 4, 16 and 31 nodes
        //We calculate our interfering curves. We also adjust 31 to 32 nodes
        //we can improve later reading the list from the file, as list_of_nodes()
        aux_app_proc = (app_proc%4) ? (app_proc - (app_proc % 4)) + 4 : app_proc ;
        ratio = (aux_app_proc%4) ? 1 : (aux_app_proc/4);
        for(int i = 1; i < 4; i++){
            ninterf[i]=(i+1)*ratio;
        }
        ninterf[0] = 1;
        ninterf[3] = app_proc;

        has_curve = boundary(real_interf_nodes,4, ninterf, &lb, &ub);

        debug5("%s Ratio %d  has_curve [%d] aux_proc %d",__func__,ratio,has_curve,aux_app_proc);
        for(int i = 0; i< 4; i++){
            debug5("%s [%d] - %d %d",__func__,i,ninterf[i],app_proc);
        }

        if(has_curve){
            //single curve example
            //read_level,app,proc,ibw_high_mean,speed,id,interf
            //getfield function mess with the pointer
            read_sensitivity_file(file,app_,app_proc,real_interf_nodes,is_local,x50,y50,x100,y100);
        }
        else
        {
            //multi curve, when we have to interpolate to create the curve
            //based on the number of interfering nodes
            debug5("%s [%d] - %d %d %d",__func__,real_interf_nodes,app_,lb,ub);


            read_sensitivity_file(file,app_,app_proc,lb,is_local,xlb50,ylb50,xlb100,ylb100);
            rewind(file);
            read_sensitivity_file(file,app_,app_proc,ub,is_local,xub50,yub50,xub100,yub100);

            w_ub=((float)real_interf_nodes-(float)lb)/((float)ub-(float)lb);
            w_lb=(1.0-w_ub);
            debug5("%s [%d] - %d %d %.5f %.5f %f",__func__,real_interf_nodes,lb,ub,w_ub,w_lb,(((float)real_interf_nodes-(float)lb)/((float)ub-(float)lb)));
            for(int i=0;i<N;i++){
                x50[i]=(xlb50[i]*w_lb + xub50[i]*w_ub)/(w_lb+w_ub);
                y50[i]=(ylb50[i]*w_lb + yub50[i]*w_ub)/(w_lb+w_ub);
                x100[i]=(xlb50[i]*w_lb + xub100[i]*w_ub)/(w_lb+w_ub);
                y100[i]=(ylb100[i]*w_lb + yub100[i]*w_ub)/(w_lb+w_ub);
            }
        }
    }

    free(list_procs);
    fclose(file);
}