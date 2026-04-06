# Mini Scheduler Project Phases

This document records what was implemented in the project from Phase A through Phase F, why each phase exists, and what the final codebase contains.

## Project Goal

Mini Scheduler is a deterministic, single-CPU, userspace scheduler simulator written in C11.

The project was built to:

- model core scheduling ideas without kernel or QEMU overhead
- keep policy logic separate from engine logic
- emit machine-parseable trace lines for every scheduling decision
- compute metrics from traces rather than from hidden internal state
- compare scheduler behavior under the same workload

## Final Structure

The final project includes:

- `include/`: shared interfaces and public data structures
- `src/`: engine, policies, config loader, metrics, comparison flow
- `src/policies/rr.c`: Round Robin policy
- `src/policies/fair.c`: simplified Fair policy
- `tests/`: unit-style validation for engine, RR, Fair, config, metrics, and compare mode
- `configs/`: sample workload definitions
- `docs/`: supporting documentation

## Phase A: Engine + Single Task

### Goal

Build the smallest working scheduler simulator:

- define `task_t`
- define `runqueue_t`
- implement a deterministic tick loop
- run one task to completion
- emit parseable trace output

### What Was Implemented

- `task_t` with scheduling state, timing fields, and `vruntime`
- `runqueue_t` with current task, time, runnable count, slice tracking, and bookkeeping
- `engine_run_single_task()` and the shared engine path in `src/engine.c`
- trace emission in `src/trace.c`
- a baseline single-task executable path in `src/main.c`

### Validation Added

- `tests/test_engine.c`

This validates:

- a single task completes in exactly `runtime` ticks
- `first_run` and `finish` are correct
- emitted trace lines match the expected sequence

## Phase B: Round Robin

### Goal

Expand from one task to multiple tasks and implement real Round Robin behavior.

### What Was Implemented

- `scheduler_t`-driven policy selection
- RR policy in `src/policies/rr.c`
- quantum-based preemption
- queue rotation support in `src/runqueue.c`
- multi-task engine path through `engine_run_tasks()`

### RR Behavior

RR now:

- enqueues tasks in arrival order
- runs the head of the runqueue
- increments the current slice each tick
- preempts when `current_slice >= quantum`
- rotates the preempted task to the tail

### Validation Added

- `tests/test_rr.c`

This validates:

- three equal tasks rotate by quantum
- RR emits `enqueue`, `pick_next`, `tick`, and `preempt` events correctly

## Phase C: Mini Fair Scheduler

### Goal

Add a simplified fair scheduler driven by `vruntime`.

### What Was Implemented

- lowest-`vruntime` selection in `src/policies/fair.c`
- nice-weighted `vruntime` progression in `src/engine.c`
- fair preemption when a better runnable candidate exists
- corrected trace semantics so preemption lines show the outgoing task state clearly

### Fair Model Used

The current simplified fair model uses:

- `pick_next`: task with smallest `vruntime`
- per-tick update: `vruntime += 1.0 + nice / 10.0`
- lower `nice` values produce slower `vruntime` growth
- newly arrived tasks can trigger immediate preemption if their `vruntime` is smaller

### Validation Added

- `tests/test_fair.c`

This validates:

- lower-nice tasks get better service under the fair scheduler
- newly arrived lower-`vruntime` tasks can preempt the current task

## Phase D: Config Loader

### Goal

Stop hardcoding workloads in `main.c` and run the simulator from JSON configuration files.

### What Was Implemented

- `config_t` in `include/config.h`
- handwritten minimal JSON loader in `src/config.c`
- policy, quantum, and task parsing from `configs/*.json`
- config-driven execution in `src/main.c`

### Supported Config Fields

- `policy`
- `quantum`
- `tasks[].id`
- `tasks[].name`
- `tasks[].arrival`
- `tasks[].runtime`
- `tasks[].nice`

### Validation Added

- `tests/test_config.c`

This validates:

- RR config parsing
- Fair config parsing
- task field extraction from JSON

## Phase E: Metrics

### Goal

Parse the emitted trace and compute per-task and aggregate metrics.

### What Was Implemented

- `metrics_report_t` and `task_metrics_t` in `include/metrics.h`
- trace parser in `src/metrics.c`
- JSON metrics output
- human-readable summary output
- integration in `src/main.c`

### Metrics Computed

Per task:

- `waiting_time`
- `turnaround_time`
- `response_time`
- `runtime`

Aggregate:

- `context_switches`
- `busy_ticks`
- `total_ticks`
- `cpu_utilization`
- `throughput`
- `fairness_index`

### Validation Added

- `tests/test_metrics.c`

This validates:

- metrics parsing on a simple 2-task RR workload
- waiting, turnaround, runtime, total ticks, and context-switch counts

## Phase F: Comparison

### Goal

Run the same workload under RR and Fair and compare the results side by side.

### What Was Implemented

- comparison interface in `include/compare.h`
- comparison runner in `src/compare.c`
- `--compare` CLI mode in `src/main.c`

### Compare Mode Output

The comparison mode prints:

- RR aggregate metrics
- Fair aggregate metrics
- deltas between the two policies
- per-task wait, turnaround, and response differences

### Validation Added

- `tests/test_compare.c`

This validates:

- compare mode runs successfully
- compare output contains both policies and per-task comparison lines

## Testing and Validation Summary

Current automated checks cover:

- engine correctness
- RR rotation
- Fair preemption/service behavior
- config parsing
- metrics parsing
- comparison mode

The validation checklist in the project brief has been updated and marked complete for:

- single task completion timing
- RR rotation
- Fair scheduler lower-nice service advantage
- Fair arrival preemption
- parseable traces
- metrics matching a simple manual case
- sanitizer build/test pass

## Sanitizers and Memory Checks

### Sanitizers

The project was built and tested with:

```sh
make clean && make CFLAGS='-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined'
ASAN_OPTIONS=detect_leaks=0 make test CFLAGS='-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined'
```

AddressSanitizer and UBSan passed.

### Valgrind

Valgrind was not available in the current environment, so that checklist item remains unverified here.

## Key Design Decisions

The project intentionally keeps the design small and interview-friendly:

- policy logic stays behind `scheduler_t`
- trace generation is isolated in `trace.c`
- metrics are derived from traces rather than engine internals
- configs are small and handwritten-parsed instead of requiring a heavy dependency
- RR and Fair share one deterministic engine

## Current Capabilities

The simulator can now:

- run RR workloads from config
- run Fair workloads from config
- emit a full trace
- print metrics as JSON and summary text
- compare RR and Fair on the same workload

## Suggested Next Improvements

Possible future improvements include:

- stronger CLI flags
- saving trace and metrics into `results/`
- more input validation
- richer metrics
- plotting/visualization helpers
- additional workload suites
- README polish and demo examples

