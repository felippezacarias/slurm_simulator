//#ifdef SLURM_SIMULATOR
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "slurm/slurm.h"



#define MAX_USERNAME_LEN 30
#define MAX_RSVNAME_LEN  30
#define MAX_QOSNAME      30
#define TIMESPEC_LEN     30
#define MAX_RSVNAME      30
#define CPUS_PER_NODE    8 
#define MAX_WF_FILENAME_LEN     1024
static const char DEFAULT_OFILE[]    = "simple.trace"; //The output trace name should be passed as an argument on command line.
#define MAX_DEPNAME              1024

typedef struct job_trace {
    int  job_id;
    char username[MAX_USERNAME_LEN];
    long int submit; /* relative or absolute? */
    int  duration;
    int  wclimit;
    int  tasks;
    char qosname[MAX_QOSNAME];
    char partition[MAX_QOSNAME];
    char account[MAX_QOSNAME];
    int  cpus_per_task;
    int  tasks_per_node;
    unsigned long long int  pn_mim_memory;
    int  min_nodes;
    int  min_cpus;
    int  shared;
    char reservation[MAX_RSVNAME];
    char dependency[MAX_DEPNAME];
    struct job_trace *next;
    char manifest_filename[MAX_WF_FILENAME_LEN];
    char *manifest;
} job_trace_t;




int main(int argc, char* argv[])
{
    int nrecs, i, first_arrival = 0, submit = 98;
    int idx=0, errs=0, share = 0, mem_mb = 0;
    job_trace_t* job_trace,* job_trace_head,* job_arr,* job_ptr;
    char* fileName;
    FILE* file;
    char line[1024], *p;


    if(argc < 2){
        printf("Error! ./swf2trace filename number_records\n");
        exit(1);
    }

    fileName = argv[1];
    if(argc > 2) nrecs = atoi(argv[2]);
    else nrecs = 50;

    file = fopen(fileName, "r");
    if(file == NULL){
        printf("Error! Open file %s, return NULL value.\n",fileName);
        exit(1);
    }

    srand(time(NULL));   // Initialization,

    job_arr = (job_trace_t*)malloc(sizeof(job_trace_t)*nrecs);
    if (!job_arr) {
        printf("Error.  Unable to allocate memory for all job records.\n");
        return -1;
    }
    
    job_trace_head = &job_arr[0];
    job_ptr = &job_arr[0];
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        if(idx > nrecs-1) break;
        printf("%s", line);
        p = strtok(line, " ");
        i=0;
        while(p!=NULL){
            if(i==0) {
                job_arr[idx].job_id = atoi(p);
                //printf("[%d] rec %d JOBID [%s]\n", idx+2, rec, p); 
            }   
            if(i==1) {
                job_arr[idx].submit = atoi(p);;
                //printf("Submit time: %s -> %ld\n", p,job_arr[idx].submit);
            }  // why submit cannot start from 0? 
            if(i==3) {
                job_arr[idx].duration = atoi(p); 
                //printf("Elapsed=%s -> %d\n", p,job_arr[idx].duration);
            }   
            if(i==4) {
                job_arr[idx].tasks = atoi(p);
                //printf("Tasks=%s -> %d\n", p,job_arr[idx].tasks);
            }
            if(i==8) {
                job_arr[idx].wclimit = atoi(p);
                //printf("Wall Clock Limit: %ld\n", job_arr[idx].wclimit );
            }
            if(i==9) {//in MB
                mem_mb = round(atoi(p)/1024);
                job_arr[idx].pn_mim_memory =  mem_mb | MEM_PER_CPU;
                //printf("pn_mim_memory: %llu\n", job_arr[idx].pn_mim_memory);
            }

            p = strtok(NULL," ");
            i++; 
        }

        //Default not to set
        job_arr[idx].shared = NO_VAL;
        job_arr[idx].tasks_per_node = NO_VAL;
        job_arr[idx].min_nodes = NO_VAL;
        job_arr[idx].cpus_per_task = NO_VAL;
        job_arr[idx].min_cpus = NO_VAL;       
        

        
        // for now keep username, partition and account constant.
        strcpy(job_arr[idx].username, "tester");
        strcpy(job_arr[idx].partition, "normal");
        strcpy(job_arr[idx].account, "1000");

        idx++; 

    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

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
         written = write(trace_file, &job_arr[j], sizeof(job_trace_t));
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


