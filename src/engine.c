#include "engine.h"

#include <stddef.h>
#include <string.h>

#include "runqueue.h"
#include "trace.h"

static void initialize_task(task_t *task)
{
    task->state = TASK_RUNNABLE;
    task->first_run = -1;
    task->finish = -1;
    task->wait_start = task->arrival;
    task->executed = 0;
    task->vruntime = 0;
    task->next = NULL;
}

static int enqueue_arrivals(FILE *trace_stream, const scheduler_t *scheduler,
                            runqueue_t *rq, task_t *tasks, int task_count,
                            int *enqueued)
{
    int i;

    for (i = 0; i < task_count; i++) {
        if (!enqueued[i] && tasks[i].arrival == rq->current_time) {
            scheduler->enqueue(rq, &tasks[i]);
            trace_emit(trace_stream, rq->current_time, 0, NULL, &tasks[i], "enqueue");
            enqueued[i] = 1;
        }
    }

    return 0;
}

static double task_vruntime_delta(const task_t *task, const scheduler_t *scheduler)
{
    if (task == NULL) {
        return 0.0;
    }

    if (strcmp(scheduler->name, "fair") == 0) {
        double delta = 1.0 + ((double)task->nice / 10.0);

        return delta > 0.0 ? delta : 0.1;
    }

    return 1.0;
}

static void preempt_current(FILE *trace_stream, const scheduler_t *scheduler,
                            runqueue_t *rq)
{
    task_t *prev = rq->current;

    prev->state = TASK_RUNNABLE;
    prev->wait_start = rq->current_time;
    runqueue_move_task_to_tail(rq, prev);
    rq->current = scheduler->pick_next(rq);
    rq->current_slice = 0;
    rq->context_switches++;
    trace_emit(trace_stream, rq->current_time, 0, prev, rq->current, "preempt");
}

int engine_run_tasks(FILE *trace_stream, const scheduler_t *scheduler,
                     task_t *tasks, int task_count, long quantum)
{
    runqueue_t rq;
    int enqueued[128] = {0};
    int completed = 0;
    int i;

    if (trace_stream == NULL || scheduler == NULL || tasks == NULL ||
        task_count <= 0 || task_count > (int)(sizeof(enqueued) / sizeof(enqueued[0]))) {
        return 1;
    }

    runqueue_init(&rq);

    for (i = 0; i < task_count; i++) {
        initialize_task(&tasks[i]);
    }

    while (completed < task_count) {
        enqueue_arrivals(trace_stream, scheduler, &rq, tasks, task_count, enqueued);

        if (rq.current != NULL && scheduler->should_preempt(&rq, quantum)) {
            preempt_current(trace_stream, scheduler, &rq);
        }

        if (rq.current == NULL) {
            rq.current = scheduler->pick_next(&rq);
            rq.current_slice = 0;
            if (rq.current == NULL) {
                rq.current_time++;
                continue;
            }
            trace_emit(trace_stream, rq.current_time, 0, NULL, rq.current, "pick_next");
        }

        if (rq.current->first_run < 0) {
            rq.current->first_run = rq.current_time;
        }

        rq.current->state = TASK_RUNNING;
        rq.current->remaining--;
        rq.current->executed++;
        rq.current->vruntime += task_vruntime_delta(rq.current, scheduler);
        rq.busy_ticks++;
        rq.current_time++;
        rq.current_slice++;

        if (rq.current->remaining == 0) {
            task_t *completed_task = rq.current;

            completed_task->state = TASK_DONE;
            completed_task->finish = rq.current_time;
            scheduler->dequeue(&rq, completed_task);
            trace_emit(trace_stream, rq.current_time, 0, completed_task, NULL, "complete");
            rq.current = NULL;
            rq.current_slice = 0;
            completed++;
            continue;
        }

        trace_emit(trace_stream, rq.current_time, 0, rq.current, rq.current, "tick");

        if (scheduler->should_preempt(&rq, quantum)) {
            preempt_current(trace_stream, scheduler, &rq);
        }
    }

    return 0;
}

int engine_run_single_task(FILE *trace_stream, const scheduler_t *scheduler,
                           task_t *task, long quantum)
{
    return engine_run_tasks(trace_stream, scheduler, task, 1, quantum);
}
