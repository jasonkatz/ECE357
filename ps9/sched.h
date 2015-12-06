struct sched_proc {
    /* use this for each simulated task */
    /* internals are up to you */
    /* probably should include things like the tast state */
    /* priority, accumulated, cpu time, stack address, etc. */
    int state;
    int static_priority;
    int dynamic_priority;
    int total_ticks; // MAY HAVE TO MAKE THIS LONG?
}

struct sched_waitq {
    /* use this for each event/wakeup queue */
    /* internals are up to you */
}
#define SCHED_NPROC 1024

#define SCHED_READY     100
#define SCHED_RUNNING   200
#define SCHED_SLEEPING  300
#define SCHED_ZOMBIE    400
