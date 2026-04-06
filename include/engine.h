#ifndef ENGINE_H
#define ENGINE_H

#include <stdio.h>

#include "scheduler.h"

int engine_run_single_task(FILE *trace_stream, const scheduler_t *scheduler,
                           task_t *task, long quantum);
int engine_run_tasks(FILE *trace_stream, const scheduler_t *scheduler,
                     task_t *tasks, int task_count, long quantum);

#endif
