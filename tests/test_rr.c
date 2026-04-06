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
    task_t tasks[3] = {
        { .id = 1, .remaining = 4, .arrival = 0, .nice = 0 },
        { .id = 2, .remaining = 4, .arrival = 0, .nice = 0 },
        { .id = 3, .remaining = 4, .arrival = 0, .nice = 0 }
    };
    char lines[32][256];
    int line_count;

    trace_stream = tmpfile();
    assert(trace_stream != NULL);

    assert(engine_run_tasks(trace_stream, rr_scheduler(), tasks, 3, 2) == 0);
    assert(tasks[0].finish == 8);
    assert(tasks[1].finish == 10);
    assert(tasks[2].finish == 12);

    line_count = read_trace_lines(trace_stream, lines, 32);
    assert(line_count > 0);
    assert(strcmp(lines[0], "[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=1 vruntime=0.0 nice=0 reason=enqueue\n") == 0);
    assert(strcmp(lines[1], "[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=2 vruntime=0.0 nice=0 reason=enqueue\n") == 0);
    assert(strcmp(lines[2], "[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=3 vruntime=0.0 nice=0 reason=enqueue\n") == 0);
    assert(strcmp(lines[3], "[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=1 vruntime=0.0 nice=0 reason=pick_next\n") == 0);
    assert(strcmp(lines[6], "[SCHED_TRACE] ts=2 cpu=0 prev=1 next=2 vruntime=2.0 nice=0 reason=preempt\n") == 0);
    assert(strcmp(lines[9], "[SCHED_TRACE] ts=4 cpu=0 prev=2 next=3 vruntime=2.0 nice=0 reason=preempt\n") == 0);
    assert(strcmp(lines[12], "[SCHED_TRACE] ts=6 cpu=0 prev=3 next=1 vruntime=2.0 nice=0 reason=preempt\n") == 0);

    fclose(trace_stream);
    return 0;
}
