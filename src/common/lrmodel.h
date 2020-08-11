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
#include <string.h>

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
 * Input:  - app_    - value independent
 *         - x50     - buffer to sensitivity curve of 50% rw
 *         - y50     - buffer to speed values of 50% rw
 *         - x100    - buffer to sensitivity curve of 100% rw
 *         - y100    - buffer to speed values of 100% rw
 * 
 */
void read_sensitivity_file(FILE* stream, int app_, int interf_nodes,double* x50,double* y50,
                double* x100,double* y100);

/*
 * Auxiliar function to return the field in each line
 * Input:  - line   - read file line
 *         - num    - required token
 * 
 * obs: in this function the buffer line is modified.
 * Return - token
 */

void read_sensitivity_curve(int app_, int app_proc, int interf_nodes, double* x50,double* y50,
            double* x100,double* y100);

const char* getfield(char* line, int num);

/*
 * Main function to return speed value.
 * Input:  - app_index      - simulated application index to look up 
 *                              in the referencefile
 *         - app_proc       - Number of target application's nodes
 *         - bw_threshold   - Threshold to be used in case of the multi curves approach
 *         - interf_n       - Number of apps sharing resources
 *         - interf_bw      - List of bandwidth of the interfering apps
 *         - interf_rwratio - List of rw ratio of the interfering apps
 *         - interf_nodes   - List of nodes sharing with tshe interfering apps
 */
double speed(int app_index, int app_nodes, double bw_threshold, int interf_n, double* interf_bw,int* interf_rwratio, int* interf_nodes);

/*
 * Interface function to return speed value.
 * Input:  - app_index      - simulated application index to look up 
 *                              in the referencefile
 *         - app_nodes      - number of nodes of the target app
 *         - bw_threshold   - Threshold to be used in case of the multi curves approach
 *         - interf_n       - number of entries on interf list
 *         - interf_apps_index - list of index of sharing apps
 *         - interf_apps_nodes - list of nodes of sharing apps
 */
double model_speed(int app_index, int app_nodes, double bw_threshold, int interf_n, int* interf_apps_index, int* interf_apps_nodes);

#endif /* _LINEAR_REGRESSION_H_ */