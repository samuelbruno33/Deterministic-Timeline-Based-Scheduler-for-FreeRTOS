# Test Journey

Todo list of the tests
- [x] Phase 0
- [x] Phase 1
- [x] Phase 2
- [ ] Phase 3
- [ ] Phase 4
- [ ] Phase 5

## Instruction

Navigate in the root of the project and execute:
```
make all
```
It will probably work fine, but it is recommended to check the makefile which needs changes.

Run the program
```
make qemu_start
```

Debug mode
```
make qemu_debug
```

## Phase 0: First test of the system

The first file created and tested is `FreeRTOS_test.c`. Designed to validate the baseline behavior of the standard FreeRTOS priority-based scheduler, before introducing any time-triggered mechanisms.

The test creates three dummy tasks (`TaskA`, `TaskB`, `TaskC`) with different priorities, where:
* `TaskA` has the highest priority,
* `TaskB` has an intermediate priority,
* `TaskC` has the lowest priority.

The purpose of this test is to observe and confirm that:

* task execution strictly follows **FreeRTOS priority rules**;
* higher-priority tasks preempt lower-priority ones;
* a task that does not block (`TaskC`) can still be preempted by higher-priority tasks;

## Phase 1: Data Structures and Logic

The file `test_data_structure.c` was created to validate the configuration API and the internal data structures of the Timeline Scheduler before integrating with the active system.

The test performs **White Box Testing** by directly inspecting the scheduler's internal tables (`xTimeline`) to verify:

* **Task Creation**: FreeRTOS handles are correctly allocated during configuration;
* **Sorting Algorithm**: HRT tasks must be automatically ordered by their absolute Start Time;
* **Error Detection**: The scheduler must detect and reject invalid configurations, such as overlapping tasks.

The purpose of this test is to confirm that:

* a set of unordered tasks (e.g., Start Times: 30, 10, 50) is correctly reordered in memory as `10 -> 30 -> 50`;
* defining two tasks with overlapping time windows triggers a `configASSERT` (captured via a flag in the test environment);
* the `vConfigureScheduler` function operates correctly without memory leaks or crashes.

The expected behavior is the output of three successful checks via UART:

## Phase 2: Timekeeper

After implementing the timekeeper, a test file `timekeeper_test.c` was created to validate the temporal logic of the Major Frame.

The test configures:

* a **Major Frame of 20 FreeRTOS ticks**;
* two **sub-frames**: `[0,10)` and `[10,20)`, used only as temporal partitions.

A single dummy task is created, which periodically blocks itself using `vTaskDelay(1000)`.

The purpose of the test is to check that:

* the FreeRTOS **Tick Hook** is correctly enabled;
* the timekeeper correctly tracks the passage of time;
* a **Major Frame reset event is generated exactly every 20 ticks**, regardless of which task is executing.

The expected behavior is the periodic emission of a `TIMEKEEPER MAJOR FRAME RESET` trace event at ticks:

```
20, 40, 60, ...
```