#include <stdio.h>
#include <string.h>

#include "compare.h"
#include "config.h"
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

int main(int argc, char **argv)
{
    config_t config;
    metrics_report_t report;
    const scheduler_t *scheduler;
    const char *config_path = "configs/rr_simple.json";
    int compare_mode = 0;
    FILE *trace_stream;

    if (argc > 3) {
        fprintf(stderr, "usage: %s [config.json]\n", argv[0]);
        fprintf(stderr, "       %s --compare [config.json]\n", argv[0]);
        return 1;
    }

    if (argc >= 2 && strcmp(argv[1], "--compare") == 0) {
        compare_mode = 1;
        if (argc == 3) {
            config_path = argv[2];
        }
    } else if (argc == 2) {
        config_path = argv[1];
    }

    if (config_load(config_path, &config) != 0) {
        fprintf(stderr, "failed to load config: %s\n", config_path);
        return 1;
    }

    if (compare_mode) {
        return compare_run(stdout, &config);
    }

    scheduler = scheduler_from_name(config.policy);
    if (scheduler == NULL) {
        fprintf(stderr, "unknown policy: %s\n", config.policy);
        return 1;
    }

    trace_stream = tmpfile();
    if (trace_stream == NULL) {
        fprintf(stderr, "failed to create trace buffer\n");
        return 1;
    }

    if (engine_run_tasks(trace_stream, scheduler, config.tasks, config.task_count,
                         config.quantum) != 0) {
        fclose(trace_stream);
        return 1;
    }

    rewind(trace_stream);
    while (!feof(trace_stream)) {
        int ch = fgetc(trace_stream);

        if (ch != EOF) {
            fputc(ch, stdout);
        }
    }

    if (metrics_compute_from_trace(trace_stream, &report) != 0) {
        fclose(trace_stream);
        fprintf(stderr, "failed to compute metrics\n");
        return 1;
    }

    printf("\n");
    metrics_print_json(stdout, &report);
    printf("\n");
    metrics_print_summary(stdout, &report);

    fclose(trace_stream);
    return 0;
}
