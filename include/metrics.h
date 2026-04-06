#ifndef METRICS_H
#define METRICS_H

#include <stdio.h>

#define METRICS_MAX_TASKS 128

typedef struct {
    int id;
    long arrival;
    long first_run;
    long finish;
    long runtime;
    long waiting_time;
    long turnaround_time;
    long response_time;
} task_metrics_t;

typedef struct {
    int task_count;
    task_metrics_t tasks[METRICS_MAX_TASKS];
    int context_switches;
    long busy_ticks;
    long total_ticks;
    int completed_tasks;
    double cpu_utilization;
    double throughput;
    double fairness_index;
} metrics_report_t;

int metrics_compute_from_trace(FILE *trace_stream, metrics_report_t *report);
void metrics_print_json(FILE *stream, const metrics_report_t *report);
void metrics_print_summary(FILE *stream, const metrics_report_t *report);

#endif
