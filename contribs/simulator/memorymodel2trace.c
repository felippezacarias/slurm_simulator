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
    int  executable;
    char reservation[MAX_RSVNAME];
    char dependency[MAX_DEPNAME];
    struct job_trace *next;
    char manifest_filename[MAX_WF_FILENAME_LEN];
    char *manifest;
} job_trace_t;




int main(int argc, char* argv[])
{
    int nrecs, i, aux, submission = 100;
    long first_arrival = NO_VAL64;
    int idx=0, errs=0, share = 0, mem_mb = 0;
    job_trace_t* job_trace,* job_trace_head,* job_arr,* job_ptr;
    FILE* file;
    char line[1024], *p, *fileName;
    int node_cores, node_minmem, is_dual;
    double submission_rate, auxf;


    if((argc < 5) || ((argc > 5) && (argc < 7))){
        printf("Error(%d)! ./swf2trace trace_filename number_records dual_partition submission_load_rate [node_cores node_min_memory]\n",argc);
        printf("dual_partiton=[0-single|1-dual]; if it is dual node_cores and node_min_memory must be set!\n");
        printf("submission_load_rate=0; means using the trace original submission rate starting from 100seg! Otherwise decrease the load x percent\n");
        exit(1);
    }

    fileName = argv[1];
    nrecs = atoi(argv[2]);
    is_dual = atoi(argv[3]);
    submission_rate = atoi(argv[4]);
    
    if(is_dual){
        node_cores = atoi(argv[5]);
        node_minmem = atoi(argv[6]);
    }

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
        //printf("%s", line);
        p = strtok(line, ",");
        i=0;
        while(p!=NULL){
            if(i==0) {
                job_arr[idx].job_id = atoi(p);
                //printf("[%d] JOBID [%d]\n", idx+2, job_arr[idx].job_id ); 
            }   
            if(i==1) {
                if (first_arrival == NO_VAL64) first_arrival = atoi(p);
                if(submission_rate){
                    //job_arr[idx].submit = submission;
                    //submission+=submission_rate;
                    aux = (atoi(p) - first_arrival);
                    auxf = (aux * (submission_rate/100));
                    job_arr[idx].submit = submission + (aux - auxf);
                }                    
                else
                    job_arr[idx].submit = submission + atoi(p) - first_arrival;
                
                //printf("Submit time: %s -> %ld -- %ld\n", p,job_arr[idx].submit,first_arrival);
            }  // why submit cannot start from 0? 
            if(i==2) {
                job_arr[idx].duration = atoi(p); 
                //printf("Elapsed=%s -> %d\n", p,job_arr[idx].duration);
            }   
            if(i==3) {
                //If it is a real trace we use min cpus, since its procs field means number of cores
                //for the synthetic trace we must use task and tasks_per_node to refer to a number of nodes
                //job_arr[idx].tasks = (is_real) ? NO_VAL : atoi(p);
                //job_arr[idx].min_cpus = (is_real) ? atoi(p) : NO_VAL;       
                //job_arr[idx].tasks_per_node = (is_real) ? NO_VAL : 1;
                job_arr[idx].min_cpus = atoi(p);       

                //printf("Tasks=%s -> %d\n", p,job_arr[idx].tasks);
            }
            if(i==4) {
                job_arr[idx].wclimit = atoi(p);
                if(job_arr[idx].wclimit == 0) job_arr[idx].wclimit = job_arr[idx].duration;
                //printf("Wall Clock Limit: %ld\n", job_arr[idx].wclimit );
            }
            if(i==5) {//in MB
                // If it is real trace, we use the mem_per_cpu option, otherwise it is mem per node
                mem_mb = round(atoi(p));
                //job_arr[idx].pn_mim_memory = (is_real) ? (mem_mb | MEM_PER_CPU) : mem_mb;
                job_arr[idx].pn_mim_memory = (mem_mb | MEM_PER_CPU);
                //printf("pn_mim_memory: %llu = [%d]\n", job_arr[idx].pn_mim_memory,mem_mb);
            }
            if(i==6){
                job_arr[idx].executable = atoi(p);
                //printf("sim_exec %d - %s\n",job_arr[idx].executable,p);
            }

            

            p = strtok(NULL,",");
            i++; 
        }

        //Default not to set
        job_arr[idx].shared = NO_VAL;
        //job_arr[idx].shared = 0;
        job_arr[idx].min_nodes = NO_VAL;
        job_arr[idx].cpus_per_task = NO_VAL;        

        
        // for now keep username, partition and account constant.
        strcpy(job_arr[idx].username, "tester");
        strcpy(job_arr[idx].partition, "normal");
        strcpy(job_arr[idx].account, "1000");

        if(is_dual && (mem_mb*node_cores > node_minmem)){
            strcpy(job_arr[idx].partition, "largemem");
        }

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

