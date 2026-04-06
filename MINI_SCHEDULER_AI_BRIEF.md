# Mini Scheduler — Build Brief

A deterministic, userspace scheduler simulator written in C.

The goal is to capture the core scheduling concepts cleanly — enqueue, pick-next,
preemption, tracing, metrics — without kernel or QEMU overhead.
This serves as both a learning tool and a companion to the SchedScope kernel project.

---

## Hard Rules

- Userspace only. No kernel code, no QEMU dependency.
- Written in C (C11).
- Single CPU only in v1.
- Deterministic tick-based simulation — no wall-clock time, no threads.
- Every scheduling decision must emit a trace line.
- Every trace line must be machine-parseable.
- Policy logic must be isolated from engine logic.
- Trace writing must be isolated from metrics parsing.
- Keep it small enough that you can explain every file in an interview.

---

## Conceptual Model

### Task

```c
typedef struct task {
    int     id;
    char    name[32];
    int     state;          // TASK_RUNNABLE, TASK_RUNNING, TASK_DONE
    int     nice;
    long    remaining;      // remaining runtime in ticks
    long    vruntime;       // for fair scheduling
    long    arrival;        // tick at which task becomes runnable
    long    first_run;      // tick of first execution (-1 if not yet run)
    long    finish;         // tick of completion (-1 if not done)
    long    wait_start;     // tick when task last entered runqueue
    struct task *next;      // intrusive linked list
} task_t;
```

### RunQueue

```c
typedef struct {
    task_t *head;           // head of runnable list
    task_t *current;        // currently running task
    long    current_time;   // current tick
    int     nr_running;     // number of runnable tasks
    int     context_switches;
} runqueue_t;
```

### Scheduler Interface

```c
typedef struct {
    const char *name;
    void   (*enqueue)(runqueue_t *rq, task_t *t);
    void   (*dequeue)(runqueue_t *rq, task_t *t);
    task_t *(*pick_next)(runqueue_t *rq);
    int    (*should_preempt)(runqueue_t *rq, long quantum);
} scheduler_t;
```

This interface is the only thing the engine calls. Policies implement it.

---

## Trace Format

One line per event. Short. Parseable. No filler text.

```
[SCHED_TRACE] ts=120 cpu=0 prev=2 next=5 vruntime=340 nice=0 reason=pick_next
[SCHED_TRACE] ts=121 cpu=0 prev=5 next=5 vruntime=341 nice=0 reason=tick
[SCHED_TRACE] ts=122 cpu=0 prev=5 next=3 vruntime=342 nice=0 reason=preempt
[SCHED_TRACE] ts=130 cpu=0 prev=3 next=-1 vruntime=0 nice=0 reason=complete
```

Rules:
- Always starts with `[SCHED_TRACE]`
- Always uses `key=value` pairs
- `prev=-1` means idle → task, `next=-1` means task → idle
- `reason` is one of: `enqueue`, `dequeue`, `pick_next`, `tick`, `preempt`, `complete`

---

## Scheduler Policies

### Round Robin
- Fixed time quantum (configurable)
- On tick: decrement remaining, preempt if quantum expired
- On pick: take head of queue
- Simple. Used as baseline for comparison.

### Mini Fair Scheduler
- Pick task with smallest `vruntime`
- Each tick: increment `vruntime` of running task
- Nice weighting: `vruntime += 1 + (nice / 10.0)` (higher nice = faster vruntime growth)
- Preempt if a newly enqueued task has smaller `vruntime` than current

Do not implement EEVDF. This simplified model is enough.

---

## Workload Config Format

```json
{
  "policy": "fair",
  "quantum": 4,
  "tasks": [
    { "id": 1, "name": "A", "arrival": 0,  "runtime": 12, "nice":  0 },
    { "id": 2, "name": "B", "arrival": 3,  "runtime": 8,  "nice":  5 },
    { "id": 3, "name": "C", "arrival": 5,  "runtime": 20, "nice": -3 }
  ]
}
```

- `policy`: `"rr"` or `"fair"`
- `quantum`: time slice in ticks (used by RR, optional for fair)
- `arrival`: tick at which task becomes runnable
- `runtime`: total ticks needed to complete
- `nice`: priority bias (-20 to +19, like Linux)

---

## Metrics To Compute

Per task:
- `waiting_time` = first_run - arrival
- `turnaround_time` = finish - arrival
- `response_time` = first_run - arrival
- `runtime` = total ticks executed

Aggregate:
- `context_switches` (total)
- `cpu_utilization` = busy ticks / total ticks
- `throughput` = completed tasks / total ticks
- `fairness_index` = Jain's fairness index on per-task runtimes

Output format: JSON + human-readable summary.

---

## Project Structure

```
mini_scheduler/
├── Makefile
├── README.md
├── configs/
│   ├── rr_simple.json
│   ├── fair_simple.json
│   └── mixed_priority.json
├── include/
│   ├── task.h
│   ├── runqueue.h
│   ├── scheduler.h
│   └── trace.h
├── src/
│   ├── main.c          // entry point, config loader, simulation driver
│   ├── engine.c        // tick loop, task injection, completion
│   ├── runqueue.c      // enqueue, dequeue, runqueue state
│   ├── policies/
│   │   ├── rr.c        // Round Robin policy
│   │   └── fair.c      // Mini Fair policy
│   ├── trace.c         // trace emission only
│   └── metrics.c       // parse traces, compute metrics, export JSON
├── results/
└── tests/
    ├── test_rr.c
    ├── test_fair.c
    └── test_metrics.c
```

---

## Build Phases

### Phase A — Engine + Single Task
- Define `task_t`, `runqueue_t`
- Implement tick loop
- Run one task to completion
- Emit `tick` and `complete` trace events
- Validate: task finishes in exactly `runtime` ticks

### Phase B — Round Robin
- Implement `scheduler_t` interface for RR
- Fixed quantum preemption
- Test with 3 equal tasks — verify rotation
- Emit `enqueue`, `pick_next`, `preempt` events

### Phase C — Mini Fair
- Add `vruntime` to task model
- Implement pick-smallest-vruntime
- Add nice weighting
- Test fairness: task with nice=-3 should get more CPU than nice=+5
- Emit `vruntime` in trace lines

### Phase D — Config Loader
- Parse JSON workload config (use a minimal JSON parser or write one)
- Inject tasks at arrival ticks
- Support both `rr` and `fair` policies from config

### Phase E — Metrics
- Parse `[SCHED_TRACE]` log file
- Compute per-task and aggregate metrics
- Export JSON + text summary

### Phase F — Comparison
- Run same workload under RR and Fair
- Compare waiting time, turnaround, fairness index
- This is the interview talking point: same workload, different policy, measurable difference

---

## Validation Checklist

- [x] Single task runs to completion in exactly `runtime` ticks
- [x] Three equal tasks under RR rotate by quantum
- [x] Fair scheduler gives more CPU to lower-nice task
- [x] Newly arrived task with small vruntime preempts current task (fair mode)
- [x] All trace lines parse without special cases
- [x] Metrics match manual calculation for simple 2-task config
- [x] Sanitizers clean: `gcc -fsanitize=address,undefined`

---


## What Not To Build in v1

- Multi-core / per-CPU queues
- Kernel locking or atomics
- cgroups or namespaces
- Full EEVDF
- GUI or visualization
- Thread-based simulation

---

## Connection to SchedScope

This simulator is the userspace companion to the SchedScope kernel project.

The same scheduling concepts appear in both:

| Concept | SchedScope (kernel) | Mini Scheduler (userspace) |
|---------|---------------------|----------------------------|
| Task selection | `pick_next_task_fair()` | `fair_pick_next()` |
| vruntime update | `update_curr()` | `engine_tick()` |
| Preemption | `check_preempt_curr()` | `fair_should_preempt()` |
| Trace | `trace_printk()` | `trace_emit()` |
| Metrics | `analyze_trace.py` | `metrics.c` |

The userspace version makes the concepts iterable and debuggable in seconds.
The kernel version proves they work on real hardware under real OS conditions.
Together they form one coherent narrative for interviews.

--
