#include "metrics.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    long arrival;
    long first_run;
    long finish;
    long runtime;
} metrics_task_state_t;

static metrics_task_state_t *find_or_create_task(metrics_task_state_t *tasks,
                                                 int *task_count, int id)
{
    int i;

    for (i = 0; i < *task_count; i++) {
        if (tasks[i].id == id) {
            return &tasks[i];
        }
    }

    if (*task_count >= METRICS_MAX_TASKS) {
        return NULL;
    }

    tasks[*task_count].id = id;
    tasks[*task_count].arrival = -1;
    tasks[*task_count].first_run = -1;
    tasks[*task_count].finish = -1;
    tasks[*task_count].runtime = 0;
    (*task_count)++;
    return &tasks[*task_count - 1];
}

static int compare_task_metrics(const void *lhs, const void *rhs)
{
    const task_metrics_t *a = lhs;
    const task_metrics_t *b = rhs;

    if (a->id < b->id) {
        return -1;
    }
    if (a->id > b->id) {
        return 1;
    }
    return 0;
}

int metrics_compute_from_trace(FILE *trace_stream, metrics_report_t *report)
{
    char line[256];
    metrics_task_state_t parsed[METRICS_MAX_TASKS] = {0};
    int parsed_count = 0;
    long max_ts = 0;
    int pending_completion_prev = -1;
    long pending_completion_ts = -1;
    int i;

    if (trace_stream == NULL || report == NULL) {
        return 1;
    }

    memset(report, 0, sizeof(*report));
    rewind(trace_stream);

    while (fgets(line, sizeof(line), trace_stream) != NULL) {
        long ts;
        int cpu;
        int prev;
        int next;
        double vruntime;
        int nice;
        char reason[32];

        if (sscanf(line,
                   "[SCHED_TRACE] ts=%ld cpu=%d prev=%d next=%d vruntime=%lf nice=%d reason=%31s",
                   &ts, &cpu, &prev, &next, &vruntime, &nice, reason) != 7) {
            return 1;
        }

        (void)cpu;
        (void)vruntime;
        (void)nice;

        if (ts > max_ts) {
            max_ts = ts;
        }

        if (strcmp(reason, "enqueue") == 0 && next >= 0) {
            metrics_task_state_t *task = find_or_create_task(parsed, &parsed_count, next);
            if (task == NULL) {
                return 1;
            }
            if (task->arrival < 0) {
                task->arrival = ts;
            }
            pending_completion_prev = -1;
        } else if (strcmp(reason, "pick_next") == 0 && next >= 0) {
            metrics_task_state_t *task = find_or_create_task(parsed, &parsed_count, next);
            if (task == NULL) {
                return 1;
            }
            if (task->first_run < 0) {
                task->first_run = ts;
            }
            if (pending_completion_prev >= 0 && pending_completion_ts == ts &&
                pending_completion_prev != next) {
                report->context_switches++;
            }
            pending_completion_prev = -1;
        } else if (strcmp(reason, "tick") == 0 && prev >= 0) {
            metrics_task_state_t *task = find_or_create_task(parsed, &parsed_count, prev);
            if (task == NULL) {
                return 1;
            }
            if (task->first_run < 0) {
                task->first_run = ts - 1;
            }
            task->runtime++;
            pending_completion_prev = -1;
        } else if (strcmp(reason, "preempt") == 0) {
            if (prev >= 0 && next >= 0 && prev != next) {
                report->context_switches++;
            }
            pending_completion_prev = -1;
        } else if (strcmp(reason, "complete") == 0 && prev >= 0) {
            metrics_task_state_t *task = find_or_create_task(parsed, &parsed_count, prev);
            if (task == NULL) {
                return 1;
            }
            if (task->first_run < 0) {
                task->first_run = ts - 1;
            }
            task->runtime++;
            task->finish = ts;
            report->completed_tasks++;
            pending_completion_prev = prev;
            pending_completion_ts = ts;
        }
    }

    report->task_count = parsed_count;
    report->total_ticks = max_ts;

    for (i = 0; i < parsed_count; i++) {
        report->tasks[i].id = parsed[i].id;
        report->tasks[i].arrival = parsed[i].arrival;
        report->tasks[i].first_run = parsed[i].first_run;
        report->tasks[i].finish = parsed[i].finish;
        report->tasks[i].runtime = parsed[i].runtime;
        report->tasks[i].waiting_time = parsed[i].first_run - parsed[i].arrival;
        report->tasks[i].turnaround_time = parsed[i].finish - parsed[i].arrival;
        report->tasks[i].response_time = parsed[i].first_run - parsed[i].arrival;
        report->busy_ticks += parsed[i].runtime;
    }

    qsort(report->tasks, (size_t)report->task_count, sizeof(report->tasks[0]),
          compare_task_metrics);

    if (report->total_ticks > 0) {
        report->cpu_utilization =
            (double)report->busy_ticks / (double)report->total_ticks;
        report->throughput =
            (double)report->completed_tasks / (double)report->total_ticks;
    }

    if (report->task_count > 0) {
        double sum = 0.0;
        double sum_sq = 0.0;

        for (i = 0; i < report->task_count; i++) {
            double runtime = (double)report->tasks[i].runtime;

            sum += runtime;
            sum_sq += runtime * runtime;
        }

        if (sum_sq > 0.0) {
            report->fairness_index =
                (sum * sum) / ((double)report->task_count * sum_sq);
        }
    }

    return 0;
}

void metrics_print_json(FILE *stream, const metrics_report_t *report)
{
    int i;

    fprintf(stream, "{\n");
    fprintf(stream, "  \"tasks\": [\n");
    for (i = 0; i < report->task_count; i++) {
        const task_metrics_t *task = &report->tasks[i];

        fprintf(stream,
                "    { \"id\": %d, \"waiting_time\": %ld, \"turnaround_time\": %ld, "
                "\"response_time\": %ld, \"runtime\": %ld }%s\n",
                task->id, task->waiting_time, task->turnaround_time,
                task->response_time, task->runtime,
                i + 1 == report->task_count ? "" : ",");
    }
    fprintf(stream, "  ],\n");
    fprintf(stream, "  \"context_switches\": %d,\n", report->context_switches);
    fprintf(stream, "  \"cpu_utilization\": %.6f,\n", report->cpu_utilization);
    fprintf(stream, "  \"throughput\": %.6f,\n", report->throughput);
    fprintf(stream, "  \"fairness_index\": %.6f\n", report->fairness_index);
    fprintf(stream, "}\n");
}

void metrics_print_summary(FILE *stream, const metrics_report_t *report)
{
    int i;

    fprintf(stream, "Summary\n");
    fprintf(stream, "context_switches=%d busy_ticks=%ld total_ticks=%ld\n",
            report->context_switches, report->busy_ticks, report->total_ticks);
    fprintf(stream, "cpu_utilization=%.6f throughput=%.6f fairness_index=%.6f\n",
            report->cpu_utilization, report->throughput, report->fairness_index);

    for (i = 0; i < report->task_count; i++) {
        const task_metrics_t *task = &report->tasks[i];

        fprintf(stream,
                "task=%d waiting=%ld turnaround=%ld response=%ld runtime=%ld\n",
                task->id, task->waiting_time, task->turnaround_time,
                task->response_time, task->runtime);
    }
}
