#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "scheduler.h"
#include "task.h"

static int read_trace_lines(FILE *stream, char lines[][256], int max_lines)
{
    char line[256];
    int count = 0;

    rewind(stream);
    while (count < max_lines && fgets(line, sizeof(line), stream) != NULL) {
        strncpy(lines[count], line, sizeof(lines[count]) - 1);
        lines[count][sizeof(lines[count]) - 1] = '\0';
        count++;
    }

    return count;
}

int main(void)
{
    FILE *trace_stream;
    task_t weighted_tasks[2] = {
        { .id = 1, .remaining = 20, .arrival = 0, .nice = 5 },
        { .id = 2, .remaining = 20, .arrival = 0, .nice = -3 }
    };
    task_t arrival_tasks[3] = {
        { .id = 1, .remaining = 12, .arrival = 0, .nice = 0 },
        { .id = 2, .remaining = 8, .arrival = 3, .nice = 5 },
        { .id = 3, .remaining = 20, .arrival = 5, .nice = -3 }
    };
    char lines[128][256];
    int line_count;
    int saw_arrival_preempt = 0;
    int i;

    trace_stream = tmpfile();
    assert(trace_stream != NULL);

    assert(engine_run_tasks(trace_stream, fair_scheduler(), weighted_tasks, 2, 0) == 0);
    assert(weighted_tasks[1].finish < weighted_tasks[0].finish);

    rewind(trace_stream);
    assert(freopen(NULL, "w+", trace_stream) != NULL);

    assert(engine_run_tasks(trace_stream, fair_scheduler(), arrival_tasks, 3, 0) == 0);
    line_count = read_trace_lines(trace_stream, lines, 128);
    assert(line_count > 0);

    for (i = 0; i < line_count; i++) {
        if (strcmp(lines[i], "[SCHED_TRACE] ts=5 cpu=0 prev=-1 next=3 vruntime=0.0 nice=-3 reason=enqueue\n") == 0) {
            assert(i + 1 < line_count);
            assert(strcmp(lines[i + 1], "[SCHED_TRACE] ts=5 cpu=0 prev=2 next=3 vruntime=3.0 nice=5 reason=preempt\n") == 0);
            saw_arrival_preempt = 1;
            break;
        }
    }

    assert(saw_arrival_preempt == 1);

    fclose(trace_stream);
    return 0;
}
