# Mini Scheduler ⚙️

A deterministic, single-CPU, userspace scheduler simulator written in C11.

Built to model core scheduling concepts — enqueue, pick-next, preemption, vruntime — in a controlled environment with no kernel or QEMU overhead.

---

## 🧠 Part of a Two-Project Scheduler Study

This project is one half of a connected study on CPU scheduling:

| | Mini Scheduler | SchedScope |
|---|---|---|
| Level | Userspace C simulator | Linux kernel (fair.c) |
| Environment | Single binary, no dependencies | QEMU + custom kernel |
| Iteration speed | Milliseconds | Minutes per rebuild |
| Noise | Deterministic ticks | Real hardware, real OS |
| Purpose | Isolate and validate policy logic | Prove concepts on real hardware |

Both projects share the same trace format, the same metric definitions, and the same scheduling concepts.
This simulator was built to iterate on policy ideas fast. SchedScope proves they hold under real kernel conditions.

→ [SchedScope](https://github.com/Ahmadsmnaser/SchedScope-Linux-Kernel-Scheduler)

---

## 🚀 What This Project Does

- Simulates Round Robin and Fair scheduling in userspace
- Injects tasks at configurable arrival times from JSON configs
- Emits structured `[SCHED_TRACE]` events for every scheduling decision
- Computes per-task and aggregate metrics from trace output
- Compares RR vs Fair side-by-side on the same workload

---

## 📊 Measured Results

Same workload (`fair_simple.json`) run under both policies:

| Metric | Round Robin | Fair |
|---|---|---|
| Context switches | 6 | 28 |
| CPU utilization | 100% | 100% |
| Fairness index | 0.877 | 0.877 |

Per-task response time comparison:

| Task | RR response | Fair response |
|---|---|---|
| A (nice=0) | 0 | 0 |
| B (nice=+5) | 1 | 0 |
| C (nice=-3) | 7 | 0 |

**Observation:** Fair eliminated waiting for all tasks — response time dropped from 7 ticks to 0 for the lowest-priority task. The cost: +366% context switches. This is the classic fairness vs overhead trade-off.

**Known issue:** Fairness index is identical between RR and Fair on this workload — this is a known metrics limitation and a good candidate for improvement.

---

## 🏗️ Architecture

```
JSON config → Engine (tick loop) → Policy (RR or Fair)
                    ↓
              [SCHED_TRACE] log
                    ↓
              Metrics parser → JSON + summary
```

Policy logic is fully isolated behind a `scheduler_t` interface:

```c
typedef struct {
    const char *name;
    void   (*enqueue)(runqueue_t *rq, task_t *t);
    void   (*dequeue)(runqueue_t *rq, task_t *t);
    task_t *(*pick_next)(runqueue_t *rq);
    int    (*should_preempt)(runqueue_t *rq, long quantum);
} scheduler_t;
```

The engine calls only this interface — it has no knowledge of which policy is running.

---

## 📝 Trace Format

One line per event. Short. Parseable. No filler.

```
[SCHED_TRACE] ts=0  cpu=0 prev=-1 next=1 vruntime=0.0  nice=0  reason=enqueue
[SCHED_TRACE] ts=0  cpu=0 prev=-1 next=1 vruntime=0.0  nice=0  reason=pick_next
[SCHED_TRACE] ts=1  cpu=0 prev=1  next=1 vruntime=1.0  nice=0  reason=tick
[SCHED_TRACE] ts=5  cpu=0 prev=2  next=3 vruntime=3.0  nice=5  reason=preempt
[SCHED_TRACE] ts=12 cpu=0 prev=1  next=-1 vruntime=12.0 nice=0 reason=complete
```

Event types: `enqueue`, `dequeue`, `pick_next`, `tick`, `preempt`, `complete`

Same format used in SchedScope kernel traces.

---

## ⚖️ Fair Scheduler Model

```
pick_next:  task with smallest vruntime
per tick:   vruntime += 1.0 + (nice / 10.0)
preempt:    if newly arrived task has smaller vruntime than current
```

Lower nice → slower vruntime growth → more CPU time. This approximates the CFS priority model without implementing full EEVDF.

---

## 🛠️ Build and Run

```bash
# Build
make

# Run a single policy
./build/mini_scheduler configs/fair_simple.json
./build/mini_scheduler configs/rr_simple.json

# Compare RR vs Fair on same workload
./build/mini_scheduler --compare configs/fair_simple.json

# Run tests
make clean && make && make test

# Sanitizer check
make clean && make CFLAGS='-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined'
ASAN_OPTIONS=detect_leaks=0 make test CFLAGS='-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined'
```

---

## 📁 Example Config

```json
{
  "policy": "fair",
  "quantum": 4,
  "tasks": [
    { "id": 1, "name": "A", "arrival": 0, "runtime": 12, "nice":  0 },
    { "id": 2, "name": "B", "arrival": 3, "runtime": 8,  "nice":  5 },
    { "id": 3, "name": "C", "arrival": 5, "runtime": 20, "nice": -3 }
  ]
}
```

---

## 📂 Project Layout

```
mini_scheduler/
├── configs/        Sample workload configurations
├── include/        Public headers and scheduler_t interface
├── src/
│   ├── engine.c    Tick loop, task injection, completion
│   ├── trace.c     Trace emission only
│   ├── metrics.c   Trace parsing and metric computation
│   ├── compare.c   RR vs Fair comparison runner
│   └── policies/
│       ├── rr.c    Round Robin
│       └── fair.c  Mini Fair (vruntime-based)
├── tests/          Unit-style validation per component
├── results/        Output directory
└── docs/
    └── PROJECT_PHASES.md
```

---

## 📈 Metrics Computed

Per task: `waiting_time`, `turnaround_time`, `response_time`, `runtime`

Aggregate: `context_switches`, `cpu_utilization`, `throughput`, `fairness_index`

All metrics are derived from the trace log — not from internal engine state.

---

## 🧪 Tests

Coverage: engine correctness, RR rotation, Fair preemption, config parsing, metrics parsing, compare mode.

```bash
make clean && make && make test
```

---

## 🔧 What I Would Do Differently

- Fix the fairness index computation to show meaningful delta between RR and Fair
- Add a starvation detection metric — useful for showing priority inversion scenarios
- Save trace and metrics to `results/` automatically per run
- Add a priority-skewed workload config to stress-test the fair scheduler more aggressively

---

## 📚 Docs

- [Phase-by-Phase Implementation Notes](docs/PROJECT_PHASES.md)
