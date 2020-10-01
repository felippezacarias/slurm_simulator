/*****************************************************************************\
 *  lrmodel.h - Linear regression model functions
 *****************************************************************************
 * Applying linear regression to predict application speed from the application's
 * sensitivity curves.
 * 
 * More complex examples:
 * https://github.com/VISWESWARAN1998/sklearn
 * https://codereview.stackexchange.com/questions/226829/simple-linear-regression-in-c 
 \*****************************************************************************/


#ifndef _LINEAR_REGRESSION_H_
#define _LINEAR_REGRESSION_H_

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include "src/common/log.h"


#define MODEL_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/*
 * Sum values of a vector and return its value.
 * Input:  - vector - buffer with values
 *         - size   - size of buffer
 * 
 * Returns sum of all values of the buffer.
 */
double sum(double*vector,int size);

/*
 * Multiply two arrays and sum the values.
 * Input:  - vector1 - buffer with values
 *         - vector2 - buffer with values
 *         - size    - size of buffer
 * 
 * Returns sum of all values of the buffer.
 */
double sum2v(double*vector1,double*vector2,int size);

/*
 * Calculates the slope of the function.
 * Input:  - sx   - sum of array x
 *         - sy   - sum of array y
 *         - sxx  - sum of array x^2
 *         - sxy  - sum of array x*y
 *         - size - size of buffer
 * 
 * Returns the value of the slope
 */
double slope(double sx,double sy,double sxx,double sxy,int size);

/*
 * Calculates the intercept of the function.
 * Input:  - m    - value of the slope
 *         - sx   - sum of array x
 *         - sy   - sum of array y
 *         - size - size of buffer
 * 
 * Returns the intercept value
 */
double intercept(double m,double sx,double sy,int size);

/*
 * Using intecept and slope values, predict the value of the speed.
 * Input:  - x_   - value independent
 *         - m    - value of the intercept
 *         - b    - value of the slope
 *         - size - size of buffer
 * 
 * Return the predicted value.
 */
double predict(double x_,double m,double b);

/*
 * Read the sensitivity curves from the file.
 * Input:  - app_         - value independent
 *         - app_proc     - app number of procs
 *         - interf_nodes - number of interfering remote/local nodes
 *         - is_local     - boolean to read local or remote sensitivity curve
 *         - x50          - buffer to sensitivity curve of 50% rw
 *         - y50          - buffer to speed values of 50% rw
 *         - x100         - buffer to sensitivity curve of 100% rw
 *         - y100         - buffer to speed values of 100% rw
 * 
 */
void read_sensitivity_file(FILE* stream, int app_, int app_proc, int interf_nodes, int is_local, double* x50,double* y50,
                double* x100,double* y100);

/*
 * Auxiliar function to return the field in each line
 * Input:  - line   - read file line
 *         - num    - required token
 * 
 * obs: in this function the buffer line is modified.
 * Return - token
 */
const char* getfield(char* line, int num);


/*
 * Auxiliar function to return the field in each line
 * Input:  - app_   - index of the app
 *         - list_procs    - list that will receive the proc values
 * 
 * Return - size of the list
 */
int list_of_nodes(int app_, int **list_procs);

/*
 * Auxiliar function to calculate the lower and upper bounds
 * to read the sensitivity curves
 * Input:  - proc_value   - Proc value of the curve or app info
 *         - list_size    - size of the list with the real values
 *         - list         - list with the collected real procs
 *         - lb and ub    - interger will receive the calculated values
 * 
 * Return - return a interger. if the proc_value is in the file the function
 *          returns 1. Otherwise it returns 0;
 */
int boundary(int proc_value, int list_size, int *list, int *lb, int* ub);

/*
 * Auxiliar function to return the bw and rw ratio of the interfering app.
 * The function interpolates the values when the requested number of procs
 * weren't collect
 * Input:  - interf_   - id of the interfering app
 *         - interf_procs    - number of procs
 *         - idx            - position in the list
 *         - is_local       - boolean to use local or remote sensitivity curve
 *         - interf_rwratio - list of values
 *         - interf_bw      - list of values
 * 
 * Return - void
 */
void interfering_bw_rw(int interf_, int interf_procs, int idx, int is_local, int *interf_rwratio,double *interf_bw);

/*
 * Auxiliar function to return the application local to remote memory ratio access
 * Input:  - target_   - id of the target app
 *         - target_procs    - number of procs
 * 
 * Return - double - the ratio value
 */
double read_app_remote_ratio(int target_, int target_procs);


void read_sensitivity_curve(int app_, int app_proc, int interf_nodes, int is_local, double* x50,double* y50,
            double* x100,double* y100);


/*
 * Main function to return speed value.
 * Input:  - app_index      - simulated application index to look up 
 *                              in the referencefile
 *         - app_proc       - Number of target application's nodes
 *         - max_bw         - Max bandwidth of the interfering apps
 *         - low_rw         - Lowest rw ratio of the interfering apps
 *         - nodes          - Sum of nodes sharing with the interfering apps
 *         - is_local       - boolean to use local or remote sensitivity curve
 */
double speed(int app_index, int app_proc, double max_bw, int low_rw, int nodes, int is_local);

/*
 * Interface function to return speed value.
 * Input:  - bw_threshold   - Threshold to be used in case of the multi curves approach
 *         - target_info - array with target execution info. The values are: 
 *                       - app_index      - simulated application index to look up 
 *                              in the referencefile
 *                       - app_nodes        - number of nodes of the target app
 *                       - interf_n         - number of entries on interf list
 *                       - is_target_local  - boolean to use local or remote sensitivity curve
 *         - interf_apps_index - list of index of sharing apps
 *         - interf_apps_nodes - list of nodes of sharing apps
 */
double* model_speed(double bw_threshold, int* target_info, int* interf_apps_index, int* interf_apps_nodes);

void swap(int* xp, int* yp);

void selectionSort(int *arr, int n);

#endif /* _LINEAR_REGRESSION_H_ */