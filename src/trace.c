#include "trace.h"

#include <stdio.h>
#include <string.h>

static int task_id_or_idle(const task_t *task)
{
    return task == NULL ? -1 : task->id;
}

static double task_vruntime_or_zero(const task_t *task)
{
    return task == NULL ? 0.0 : task->vruntime;
}

static int task_nice_or_zero(const task_t *task)
{
    return task == NULL ? 0 : task->nice;
}

void trace_emit(FILE *stream, long ts, int cpu, const task_t *prev,
                const task_t *next, const char *reason)
{
    const task_t *subject = next;

    if (strcmp(reason, "preempt") == 0 || strcmp(reason, "complete") == 0) {
        subject = prev;
    } else if (strcmp(reason, "tick") == 0) {
        subject = prev;
    }

    fprintf(stream,
            "[SCHED_TRACE] ts=%ld cpu=%d prev=%d next=%d vruntime=%.1f nice=%d "
            "reason=%s\n",
            ts, cpu, task_id_or_idle(prev), task_id_or_idle(next),
            task_vruntime_or_zero(subject), task_nice_or_zero(subject), reason);
}
