#ifdef SLURM_SIMULATOR
/*
 ** Definitions for simulation mode
 ** */

typedef struct simulator_event{
    int job_id;
    int type;
    time_t when;
    time_t hardwhen;
    char *nodelist;
    volatile struct simulator_event *next;
}simulator_event_t;

typedef struct simulator_event_info{
    int job_id;
    int duration;
    int wclimit;
    struct simulator_event_info *next;
}simulator_event_info_t;
#endif
