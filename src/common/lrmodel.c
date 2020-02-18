#include "src/common/lrmodel.h"

double speed(int app_index,int interf_bw,int interf_rwratio){

    int N = 5; 
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

    read_sensitivity_file(app_index,x50,y50,x100,y100);

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
    pred50 = predict(interf_bw,m50,b50);
    pred100 = predict(interf_bw,m100,b100);

    w_100=(interf_rwratio-50.0)/(100.0-50.0);
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

void read_sensitivity_file(int app_, double* x50,double* y50,
            double* x100,double* y100)
{
    FILE* stream = fopen("/home/bscuser/bsc/simulators/workload_traces/extending_trace/curves_apps.csv", "r");
    char line[1024];
    int id,read_level;
    char *id_, *x_, *y_, *tmp;
    int i50 = 0, i100 = 0;
    double x,y;

    //read_level,app,ibw_high_mean,speed,id
    //getfield function mess with the pointer
    while (fgets(line, 1024, stream))
    {
        id_ = strdup(line);
        id = atoi(getfield(id_, 5));
        if((id == app_)){
            tmp = strdup(line);
            x_ = strdup(line);
            y_ = strdup(line);
            
            read_level = atoi(getfield(tmp, 1));
            x = atof(getfield(x_, 3));
            y = atof(getfield(y_, 4));
            
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
    }
    fclose(stream);
}
