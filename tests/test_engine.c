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
    task_t task = {0};
    char lines[8][256];
    int line_count;

    trace_stream = tmpfile();
    assert(trace_stream != NULL);

    task.id = 1;
    strncpy(task.name, "single", sizeof(task.name) - 1);
    task.state = TASK_RUNNABLE;
    task.nice = 0;
    task.remaining = 5;
    task.arrival = 0;
    task.first_run = -1;
    task.finish = -1;
    task.wait_start = 0;

    assert(engine_run_single_task(trace_stream, rr_scheduler(), &task, 2) == 0);
    assert(task.state == TASK_DONE);
    assert(task.remaining == 0);
    assert(task.executed == 5);
    assert(task.first_run == 0);
    assert(task.finish == 5);
    assert(task.wait_start == 0);

    line_count = read_trace_lines(trace_stream, lines, 8);
    assert(line_count == 7);
    assert(strcmp(lines[0], "[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=1 vruntime=0.0 nice=0 reason=enqueue\n") == 0);
    assert(strcmp(lines[1], "[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=1 vruntime=0.0 nice=0 reason=pick_next\n") == 0);
    assert(strcmp(lines[2], "[SCHED_TRACE] ts=1 cpu=0 prev=1 next=1 vruntime=1.0 nice=0 reason=tick\n") == 0);
    assert(strcmp(lines[3], "[SCHED_TRACE] ts=2 cpu=0 prev=1 next=1 vruntime=2.0 nice=0 reason=tick\n") == 0);
    assert(strcmp(lines[4], "[SCHED_TRACE] ts=3 cpu=0 prev=1 next=1 vruntime=3.0 nice=0 reason=tick\n") == 0);
    assert(strcmp(lines[5], "[SCHED_TRACE] ts=4 cpu=0 prev=1 next=1 vruntime=4.0 nice=0 reason=tick\n") == 0);
    assert(strcmp(lines[6], "[SCHED_TRACE] ts=5 cpu=0 prev=1 next=-1 vruntime=5.0 nice=0 reason=complete\n") == 0);

    fclose(trace_stream);
    return 0;
}
