#include "src/common/lrmodel.h"

double model_speed(int app_index, int app_nodes, double bw_threshold, int interf_n, int* interf_apps_index, int* interf_apps_nodes){
    FILE* stream = NULL;
    int* interf_rwratio;
    double* interf_bw;
    char line[1024];
    double res;

    interf_rwratio = (int*) malloc(interf_n*sizeof(int));
    interf_bw = (double*) malloc(interf_n*sizeof(double));

    stream = fopen("~/SLURM_SIMULATOR/workload_traces/extending_trace/mpi/app_info_list.csv", "r");
    for(int i = 0; i < interf_n; i++){
        char *id_, *rw_, *bw_, *nodes_;
        int id, rw_ratio, nodes;
        double bw;

        while (fgets(line, 1024, stream))
        {
            id_ = strdup(line);
            id = atoi(getfield(id_, 2));
            nodes_ = strdup(line);
            nodes = atoi(getfield(nodes_, 4));
            if((id == interf_apps_index[i]) && (nodes == interf_apps_nodes[i])){
                rw_ = strdup(line);
                bw_ = strdup(line);
                
                rw_ratio = atoi(getfield(rw_, 7));
                bw = atof(getfield(bw_, 6));

                printf("x-%d %d %f %d\n",id,nodes,bw,rw_ratio);

                interf_rwratio[i] = rw_ratio;
                interf_bw[i] = bw;

                free(rw_);
                free(bw_);
                free(id_);
                free(nodes_);
                break;
            }
            free(id_);
            free(nodes_);
        }
        rewind(stream);
    }

    res = speed(app_index,app_nodes,bw_threshold,interf_n,interf_bw,interf_rwratio,interf_apps_nodes);

    free(interf_rwratio);
    free(interf_bw);
    fclose(stream);

    return res;
}


double speed(int app_index, int app_proc, double bw_threshold, int interf_n, double* interf_bw, int* interf_rwratio, int* interf_nodes){

    int N = 5, nodes = 0;
    double max_bw = 0.0;
    int low_rw = 100; 
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

    //Getting the highest bw and lowest RW
    for(int i=0;i<interf_n;i++){
        if(interf_bw[i] > max_bw)
            max_bw = interf_bw[i];
        if(interf_rwratio[i] < low_rw)
            low_rw = interf_rwratio[i];
    }

    // Sum proc based on a threshold
    for(int i=0;i<interf_n;i++){
        if(interf_bw[i] > max_bw*bw_threshold)
            nodes += interf_nodes[i];
    }

    printf("%d %f %d\n",nodes,max_bw,low_rw);

    read_sensitivity_curve(app_index,app_proc,nodes,x50,y50,x100,y100);

    for(int i=0; i<5; i++){
        printf("%d - %f %f - %f %f\n",i,x50[i],y50[i],x100[i],y100[i]);
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
    speed = ((pred50)*(1-w_100)+(pred100)*(w_100));

    return speed;
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

void read_sensitivity_file(FILE* stream, int app_, int interf_nodes, double* x50,double* y50,
            double* x100,double* y100){
    char line[1024];
    int i50 = 0, i100 = 0;
    char *id_, *x_, *y_, *interf_, *tmp;
    int id,read_level,interf;
    double x,y;

    while (fgets(line, 1024, stream))
    {
        id_ = strdup(line);
        id = atoi(getfield(id_, 6));
        interf_ = strdup(line);
        interf = atoi(getfield(interf_, 7));
        if((id == app_) && (interf == interf_nodes)){
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
    }
}

void read_sensitivity_curve(int app_, int app_proc, int interf_nodes, double* x50,double* y50,
            double* x100,double* y100)
{
    FILE* stream = NULL;
    int has_curve = 0, lb = 1, ub = 1, ratio = 1;
    double w_lb,w_ub;

    stream = fopen("~/SLURM_SIMULATOR/workload_traces/extending_trace/mpi/curves_apps_multi.csv", "r");

    int ninterf[4];
    //Get the number of interf_curves
    //We just collected 4 examples varying the number of interf nodes. modify this part if the file changes
    //if(app_proc==31) app_proc = 32;
    //ninterf[0]=1;
    //ninterf[1]=((app_proc/4)*2);
    //ninterf[2]=((app_proc/4)*3);
    //ninterf[3]=app_proc;
    //In our tests we just collected the curves for app using 4, 16 and 31 nodes
    if(app_proc==31) app_proc = 32;
    ratio = (app_proc%4) ? 1 : (app_proc/4);
    for(int i = 0; i < 4; i++){
        ninterf[i]=(i+1)*ratio;
    }
    if(ninterf[3]>31) ninterf[3]=31;

    for(int i =0; i < 4; i++){
        if(ninterf[i] == interf_nodes){
            has_curve = 1;
            break;
        }
        if(ninterf[i] < interf_nodes)
            lb = ninterf[i];
        //we issue a break here to get the upper bound near the curve we want
        if(ninterf[i] > interf_nodes){
            ub = ninterf[i];
            break;
        }
    }

    for(int i = 0; i< 4; i++){
        printf("%d - %d %d %d [%d]\n",i,ninterf[i],app_proc,ratio,has_curve);
    }

    if(has_curve){
        //single curve example
        //read_level,app,proc,ibw_high_mean,speed,id,interf
        //getfield function mess with the pointer
        read_sensitivity_file(stream,app_,interf_nodes,x50,y50,x100,y100);
    }
    else
    {
        //multi curve, when we have to interpolate to create the curve
        double xlb100[5],xub100[5];
        double ylb100[5],yub100[5];
        double xlb50[5],xub50[5];
        double ylb50[5],yub50[5];

        read_sensitivity_file(stream,app_,lb,xlb50,ylb50,xlb100,ylb100);
        rewind(stream);
        read_sensitivity_file(stream,app_,ub,xub50,yub50,xub100,yub100);

        w_ub=(interf_nodes-lb)/(ub-lb);
        w_lb=(1-w_ub);

        for(int i=0;i<5;i++){
            x50[i]=(xlb50[i]*w_lb + xub50[i]*w_ub)/(w_lb+w_ub);
            y50[i]=(ylb50[i]*w_lb + yub50[i]*w_ub)/(w_lb+w_ub);
            x100[i]=(xlb50[i]*w_lb + xub100[i]*w_ub)/(w_lb+w_ub);
            y100[i]=(ylb100[i]*w_lb + yub100[i]*w_ub)/(w_lb+w_ub);
        }
    }
    


    fclose(stream);
}