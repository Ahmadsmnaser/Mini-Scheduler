#include "scheduler.h"

#include <stddef.h>

#include "runqueue.h"

static void rr_enqueue(runqueue_t *rq, task_t *task)
{
    runqueue_push_tail(rq, task);
}

static void rr_dequeue(runqueue_t *rq, task_t *task)
{
    runqueue_remove(rq, task);
}

static task_t *rr_pick_next(runqueue_t *rq)
{
    return rq->head;
}

static int rr_should_preempt(runqueue_t *rq, long quantum)
{
    if (rq->current == NULL || rq->nr_running <= 1 || quantum <= 0) {
        return 0;
    }

    return rq->current_slice >= quantum;
}

const scheduler_t *rr_scheduler(void)
{
    static const scheduler_t scheduler = {
        .name = "rr",
        .enqueue = rr_enqueue,
        .dequeue = rr_dequeue,
        .pick_next = rr_pick_next,
        .should_preempt = rr_should_preempt,
    };

    return &scheduler;
}
