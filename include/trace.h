#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>

#include "task.h"

// Function to emit a trace event to the given stream, with the specified timestamp, CPU, previous task, next task, and reason for the context switch
void trace_emit(FILE *stream, long ts, int cpu, const task_t *prev,
                const task_t *next, const char *reason);

#endif

