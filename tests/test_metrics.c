#include <assert.h>
#include <stdio.h>

#include "engine.h"
#include "scheduler.h"
#include "metrics.h"
#include "task.h"

int main(void)
{
    FILE *trace_stream;
    metrics_report_t report;
    task_t tasks[2] = {
        { .id = 1, .remaining = 2, .arrival = 0, .nice = 0 },
        { .id = 2, .remaining = 2, .arrival = 0, .nice = 0 }
    };

    trace_stream = tmpfile();
    assert(trace_stream != NULL);

    assert(engine_run_tasks(trace_stream, rr_scheduler(), tasks, 2, 1) == 0);
    assert(metrics_compute_from_trace(trace_stream, &report) == 0);

    assert(report.task_count == 2);
    assert(report.completed_tasks == 2);
    assert(report.context_switches == 3);
    assert(report.busy_ticks == 4);
    assert(report.total_ticks == 4);
    assert(report.tasks[0].id == 1);
    assert(report.tasks[0].waiting_time == 0);
    assert(report.tasks[0].turnaround_time == 3);
    assert(report.tasks[0].runtime == 2);
    assert(report.tasks[1].id == 2);
    assert(report.tasks[1].waiting_time == 1);
    assert(report.tasks[1].turnaround_time == 4);
    assert(report.tasks[1].runtime == 2);

    fclose(trace_stream);
    return 0;
}
