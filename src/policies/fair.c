#include "scheduler.h"

#include <stddef.h>

#include "runqueue.h"

static void fair_enqueue(runqueue_t *rq, task_t *task)
{
    runqueue_push_tail(rq, task);
}

static void fair_dequeue(runqueue_t *rq, task_t *task)
{
    runqueue_remove(rq, task);
}

static task_t *fair_pick_next(runqueue_t *rq)
{
    task_t *best = rq->head;
    task_t *cursor = rq->head;

    while (cursor != NULL) {
        if (cursor->vruntime < best->vruntime) {
            best = cursor;
        }
        cursor = cursor->next;
    }

    return best;
}

static int fair_should_preempt(runqueue_t *rq, long quantum)
{
    task_t *best;

    (void)quantum;

    if (rq->current == NULL || rq->nr_running <= 1) {
        return 0;
    }

    best = fair_pick_next(rq);
    return best != NULL && best != rq->current &&
           best->vruntime < rq->current->vruntime;
}

const scheduler_t *fair_scheduler(void)
{
    static const scheduler_t scheduler = {
        .name = "fair",
        .enqueue = fair_enqueue,
        .dequeue = fair_dequeue,
        .pick_next = fair_pick_next,
        .should_preempt = fair_should_preempt,
    };

    return &scheduler;
}
