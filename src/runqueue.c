#include "runqueue.h"

#include <stddef.h>

void runqueue_init(runqueue_t *rq)
{
    rq->head = NULL;
    rq->current = NULL;
    rq->current_time = 0;
    rq->current_slice = 0;
    rq->nr_running = 0;
    rq->context_switches = 0;
    rq->busy_ticks = 0;
}

void runqueue_push_tail(runqueue_t *rq, task_t *task)
{
    task_t *cursor;

    task->next = NULL;

    if (rq->head == NULL) {
        rq->head = task;
        rq->nr_running++;
        return;
    }

    cursor = rq->head;
    while (cursor->next != NULL) {
        cursor = cursor->next;
    }

    cursor->next = task;
    rq->nr_running++;
}

void runqueue_remove(runqueue_t *rq, task_t *task)
{
    task_t *cursor = rq->head;
    task_t *prev = NULL;

    while (cursor != NULL) {
        if (cursor == task) {
            if (prev == NULL) {
                rq->head = cursor->next;
            } else {
                prev->next = cursor->next;
            }

            cursor->next = NULL;
            rq->nr_running--;
            return;
        }

        prev = cursor;
        cursor = cursor->next;
    }
}

void runqueue_move_task_to_tail(runqueue_t *rq, task_t *task)
{
    task_t *cursor;
    task_t *tail;
    task_t *prev = NULL;

    if (rq->head == NULL || rq->head->next == NULL || task == NULL) {
        return;
    }

    cursor = rq->head;
    while (cursor != NULL && cursor != task) {
        prev = cursor;
        cursor = cursor->next;
    }

    if (cursor == NULL || cursor->next == NULL) {
        return;
    }

    if (prev == NULL) {
        rq->head = cursor->next;
    } else {
        prev->next = cursor->next;
    }

    cursor->next = NULL;

    tail = rq->head;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    tail->next = cursor;
}
