#include "compare.h"

#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "metrics.h"
#include "scheduler.h"

static const scheduler_t *scheduler_from_name(const char *name)
{
    if (strcmp(name, "rr") == 0) {
        return rr_scheduler();
    }

    if (strcmp(name, "fair") == 0) {
        return fair_scheduler();
    }

    return NULL;
}

static int run_policy_report(const config_t *base_config, const char *policy_name,
                             metrics_report_t *report)
{
    config_t config_copy;
    const scheduler_t *scheduler;
    FILE *trace_stream;

    config_copy = *base_config;
    strncpy(config_copy.policy, policy_name, sizeof(config_copy.policy) - 1);
    config_copy.policy[sizeof(config_copy.policy) - 1] = '\0';

    scheduler = scheduler_from_name(config_copy.policy);
    if (scheduler == NULL) {
        return 1;
    }

    trace_stream = tmpfile();
    if (trace_stream == NULL) {
        return 1;
    }

    if (engine_run_tasks(trace_stream, scheduler, config_copy.tasks,
                         config_copy.task_count, config_copy.quantum) != 0) {
        fclose(trace_stream);
        return 1;
    }

    if (metrics_compute_from_trace(trace_stream, report) != 0) {
        fclose(trace_stream);
        return 1;
    }

    fclose(trace_stream);
    return 0;
}

static const task_metrics_t *find_task(const metrics_report_t *report, int id)
{
    int i;

    for (i = 0; i < report->task_count; i++) {
        if (report->tasks[i].id == id) {
            return &report->tasks[i];
        }
    }

    return NULL;
}

int compare_run(FILE *stream, const config_t *base_config)
{
    metrics_report_t rr_report;
    metrics_report_t fair_report;
    int i;

    if (stream == NULL || base_config == NULL) {
        return 1;
    }

    if (run_policy_report(base_config, "rr", &rr_report) != 0) {
        return 1;
    }

    if (run_policy_report(base_config, "fair", &fair_report) != 0) {
        return 1;
    }

    fprintf(stream, "Comparison\n");
    fprintf(stream, "policy_rr context_switches=%d cpu_utilization=%.6f throughput=%.6f fairness_index=%.6f\n",
            rr_report.context_switches, rr_report.cpu_utilization,
            rr_report.throughput, rr_report.fairness_index);
    fprintf(stream, "policy_fair context_switches=%d cpu_utilization=%.6f throughput=%.6f fairness_index=%.6f\n",
            fair_report.context_switches, fair_report.cpu_utilization,
            fair_report.throughput, fair_report.fairness_index);
    fprintf(stream, "delta context_switches=%d cpu_utilization=%.6f throughput=%.6f fairness_index=%.6f\n",
            fair_report.context_switches - rr_report.context_switches,
            fair_report.cpu_utilization - rr_report.cpu_utilization,
            fair_report.throughput - rr_report.throughput,
            fair_report.fairness_index - rr_report.fairness_index);

    for (i = 0; i < rr_report.task_count; i++) {
        const task_metrics_t *rr_task = &rr_report.tasks[i];
        const task_metrics_t *fair_task = find_task(&fair_report, rr_task->id);

        if (fair_task == NULL) {
            return 1;
        }

        fprintf(stream,
                "task=%d rr_wait=%ld fair_wait=%ld rr_turnaround=%ld fair_turnaround=%ld rr_response=%ld fair_response=%ld\n",
                rr_task->id, rr_task->waiting_time, fair_task->waiting_time,
                rr_task->turnaround_time, fair_task->turnaround_time,
                rr_task->response_time, fair_task->response_time);
    }

    return 0;
}
