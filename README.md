# Mini Scheduler

Mini Scheduler is a deterministic, single-CPU, userspace scheduler simulator written in C11.

It models core scheduling concepts in a small and interview-friendly codebase:

- enqueue and dequeue
- task selection
- quantum-based preemption
- fair scheduling with `vruntime`
- machine-parseable trace generation
- metrics computed from trace output
- side-by-side RR vs Fair comparison

## Why This Project Exists

The project is a userspace companion to the larger SchedScope kernel work.

Instead of dealing with kernel complexity, QEMU, and wall-clock noise, this simulator focuses on the scheduling ideas themselves in a deterministic tick-based environment.

## Features

- C11 implementation
- deterministic tick-based simulation
- single CPU in v1
- Round Robin scheduler
- simplified Fair scheduler
- JSON workload configs
- parseable `[SCHED_TRACE]` logs
- metrics output in JSON and summary formats
- RR vs Fair comparison mode
- test coverage for engine, policies, config loading, metrics, and compare mode

## Project Layout

```text
.
├── configs/        Sample workload configurations
├── docs/           Project documentation
├── include/        Public headers and interfaces
├── src/            Engine, policies, config, metrics, compare mode
├── tests/          Validation tests
├── results/        Output directory for future generated artifacts
├── Makefile
└── README.md
```

## Build

```sh
make
```

## Run a Config

Run the simulator with a workload config:

```sh
./build/mini_scheduler configs/rr_simple.json
./build/mini_scheduler configs/fair_simple.json
```

This prints:

- the scheduler trace
- a JSON metrics report
- a short human-readable summary

## Compare RR and Fair

Run the same workload under both policies:

```sh
./build/mini_scheduler --compare configs/fair_simple.json
```

This prints:

- RR aggregate metrics
- Fair aggregate metrics
- metric deltas
- per-task comparison lines

## Example Config

```json
{
  "policy": "fair",
  "quantum": 4,
  "tasks": [
    { "id": 1, "name": "A", "arrival": 0, "runtime": 12, "nice": 0 },
    { "id": 2, "name": "B", "arrival": 3, "runtime": 8, "nice": 5 },
    { "id": 3, "name": "C", "arrival": 5, "runtime": 20, "nice": -3 }
  ]
}
```

Supported fields:

- `policy`: `rr` or `fair`
- `quantum`: used by RR and accepted by Fair
- `tasks[].id`
- `tasks[].name`
- `tasks[].arrival`
- `tasks[].runtime`
- `tasks[].nice`

## Example Trace

```text
[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=1 vruntime=0.0 nice=0 reason=enqueue
[SCHED_TRACE] ts=0 cpu=0 prev=-1 next=1 vruntime=0.0 nice=0 reason=pick_next
[SCHED_TRACE] ts=1 cpu=0 prev=1 next=1 vruntime=1.0 nice=0 reason=tick
[SCHED_TRACE] ts=5 cpu=0 prev=2 next=3 vruntime=3.0 nice=5 reason=preempt
```

## Example Metrics Output

```json
{
  "tasks": [
    { "id": 1, "waiting_time": 0, "turnaround_time": 28, "response_time": 0, "runtime": 12 }
  ],
  "context_switches": 8,
  "cpu_utilization": 1.000000,
  "throughput": 0.083333,
  "fairness_index": 1.000000
}
```

## Tests

Run the full test suite:

```sh
make clean && make && make test
```

The current tests cover:

- engine correctness
- RR rotation
- Fair scheduling behavior
- config parsing
- metrics parsing
- compare mode

## Sanitizer Check

```sh
make clean && make CFLAGS='-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined'
ASAN_OPTIONS=detect_leaks=0 make test CFLAGS='-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined'
```

## Documentation

For a full implementation walkthrough from Phase A through Phase F, see:

- `docs/PROJECT_PHASES.md`

## Current Status

Implemented:

- Phase A: engine + single task
- Phase B: Round Robin
- Phase C: simplified Fair scheduler
- Phase D: config loader
- Phase E: metrics
- Phase F: comparison mode

## Future Improvements

Good next steps could include:

- saving artifacts to `results/`
- more robust config validation
- richer fairness metrics
- visualization helpers
- additional workload suites
- benchmarking and demo scripts
