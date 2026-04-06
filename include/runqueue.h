#ifndef RUNQUEUE_H
#define RUNQUEUE_H

#include "task.h"

typedef struct {
    task_t *head;
    task_t *current;
    long current_time;
    long current_slice;
    int nr_running;
    int context_switches;
    long busy_ticks;
} runqueue_t;

void runqueue_init(runqueue_t *rq);
void runqueue_push_tail(runqueue_t *rq, task_t *task);
void runqueue_remove(runqueue_t *rq, task_t *task);
void runqueue_move_task_to_tail(runqueue_t *rq, task_t *task);

#endif
