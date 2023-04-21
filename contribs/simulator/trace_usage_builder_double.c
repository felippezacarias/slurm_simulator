#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include "slurm/slurm.h"

static const char DEFAULT_OFILE[]    = "simple.trace"; //The output trace name should be passed as an argument on command line.

typedef struct job_usage_trace {
    int  job_id;
    int  node;
    unsigned long long int  pn_mim_memory;
    double  id_event;
    struct job_usage_trace *next;
} job_usage_trace_t;

int count_lines(FILE *fp){
    char line[1024];
    int lines = 0;

    while (fgets(line, sizeof(line), fp)) {
        lines++;
    }

    rewind(fp);

    return lines;
}


int main(int argc, char* argv[])
{
    int nrecs, i, aux, submission = 100;
    long first_arrival = NO_VAL64;
    int idx=0, errs=0, share = 0, mem_mb = 0;
    job_usage_trace_t* job_trace,* job_trace_head,* job_arr,* job_ptr;
    FILE* file;
    char line[1024], *p, *fileName;
    int node_cores, node_minmem, is_dual;
    double submission_rate, auxf;

    //jobid,node,new_mem,id_event
    if((argc < 2)){
        printf("Error(%d)! ./builder trace_filename\n",argc);
        exit(1);
    }

    fileName = argv[1];
        
    file = fopen(fileName, "r");
    if(file == NULL){
        printf("Error! Open file %s, return NULL value.\n",fileName);
        exit(1);
    }

    nrecs = count_lines(file);

    srand(time(NULL));   // Initialization,

    job_arr = (job_usage_trace_t*)malloc(sizeof(job_usage_trace_t)*nrecs);
    if (!job_arr) {
        printf("Error.  Unable to allocate memory for all job records.\n");
        return -1;
    }
    
    job_trace_head = &job_arr[0];
    job_ptr = &job_arr[0];
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        
        printf("line: %s",line);

        //if(idx > nrecs-1) break;
        
        p = strtok(line, ",");
        i=0;
        while(p!=NULL){
            if(i==0) {
                job_arr[idx].job_id = atoi(p);
            }   
            if(i==1) {
                job_arr[idx].node = atoi(p);
            } 
            if(i==2) {
                job_arr[idx].pn_mim_memory = (atoi(p) | MEM_PER_CPU);
            }   
            if(i==3) {
                job_arr[idx].id_event = atof(p);
            }
            
            p = strtok(NULL,",");
            i++; 
        }

        idx++; 

    }

    fclose(file);
    
    int trace_file, written;
    char *ofile         = NULL;
    if (!ofile) ofile = (char*)DEFAULT_OFILE;
    /* open trace file: */
    if ((trace_file = open(ofile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR |
                                                S_IRGRP | S_IROTH)) < 0) {
       printf("Error opening trace file %s\n", ofile);
       return -1;
    }
    job_ptr = job_trace_head;
    int j=0;
    while(j<nrecs){
        
         written = write(trace_file, &job_arr[j], sizeof(job_usage_trace_t));
              if (written <= 0) {
                        printf("Error! Zero bytes written.\n");
                        ++errs;
              }

        j++;
    }
    close(trace_file);
    free(job_arr);
    return 0;
}



