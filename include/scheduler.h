#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "runqueue.h"

typedef struct scheduler {
    const char *name; // Name of the scheduler
    void (*enqueue)(runqueue_t *rq, task_t *task); // Function to add a task to the runqueue
    void (*dequeue)(runqueue_t *rq, task_t *task); // Function to remove a task from the runqueue
    task_t *(*pick_next)(runqueue_t *rq); // Function to pick the next task to run from the runqueue
    int (*should_preempt)(runqueue_t *rq, long quantum); // Function to determine if the current task should be preempted
} scheduler_t;

const scheduler_t *rr_scheduler(void); // Round Robin scheduler
const scheduler_t *fair_scheduler(void); // Fair scheduler (CFS-like)

#endif

