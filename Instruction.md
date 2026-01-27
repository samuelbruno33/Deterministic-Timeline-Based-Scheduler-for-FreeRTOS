Politecnico di Torino - Accademic Year 2025/26
# Precise Scheduler for FreeRTOS
## Project 1 - Assignment
### Overview
Developing of a precise, timeline-based scheduler that replace the standard priority-based FreeRTOS scheduler.

The schduler must enforces deterministic task execution based on a major frame and sub-frame structure, following the principles of time-triggered architectures. 

Each task runs at predefined times in a global schedule, ensuring predictable real-time behavior and repeatability, without relying on dynamic priority changes.

### Key Features
### 1 - Major Frame Structure
The scheduler operates within a major frame, with a duration defined at compile time (e.g. 100ms). Each major frame is divided into sub-frames, smaller time windows that host groups of tasks. Each sub-frame can contain multiple tasks with assigned timing and order.

### 2 - Task Model
Each task is implemented as a single function that executes from start to end and then terminates. Tasks do not include periodicity or self-rescheduling logic, but the scheduler controls activation timing. If two tasks need to communicate, they do through polling.

### 3 - Task Categories
**Hard Real-Time Tasks (HRT)**

Task assigned to a specific sub-frame. The scheduler knows the start time and end times of each task within the sub-frame. A task is spawned at the beginning of its slot and either
- run until completion, or
- is terminated if it exceeds its deadline.

Tasks are non-preemptive, once started they cannot be interrupted.

**Soft Real-Time Tasks (SRT)**

Tasks executed during idle time left by HRT tasks. They are scheduled in a fixed compile-time order (e.g. Task_X -> Task_Y -> Task_Z). They are preemptible by any HRT Task, and there's no guarantee of completation within the frame.

### 4 - Periodic Repetition
At the end of each major frame:
- All tasks are reset and reinitialized. This can be done by destroying and recreating all tasks or by resetting their state to an initial value.
- The scheduler replays the same timeline, guaranteeing deterministic repetiotin across frames.

### 5 - Configuration Interface
All scheduling parameters (start/end time, sub-frame, order, category, ...) are defined in a dedicated OS Data Structure.

**Example**
```c
typedef struct {
    const char* task_name;
    TaskFunction_t function;
    TaskType_t type; // HARD_RT or SOFT_RT
    uint32_t ulStart_time;
    uint32_t ulEnd_time;
    uint32_t ulSubframe_id;
} TimelineTaskConfig_t;
```
A system call (like ```vConfigureScheduler(TimelineConfig_t *cfg)```) will:
- Parse this configuration;
- Create required FreeRTOS data structures;
- Initialize the timeline-based scheduling enviroment.

### Non-Functional Requirements
- **Release Jitter**: $\le$ 1 tick;
- **Overhead**: 10% CPU ($\le$ 8 tasks);
- **Portability**: QEMU Cortex-M;
- **Thread Safety**: FreeRTOS-safe synchronization;
- **Documentation**: Clear API and Timing Model;
- **Coding Guidelines**: Strictly follow [FreeRTOS Coding Guidelines](https://freertos.org/Documentation/02-Kernel/06-Coding-guidelines/02-FreeRTOS-Coding-Standard-and-Style-Guide).

### Error Handling & Edge Cases
Handle all errors with dedicated hook functions following the FreeRTOS style.

### Trace and Monitoring System
A trace module must be developed for test and validation purposes to log and visualize scheduler behavior with tick-level precision. The information to be logged are:
- Task start and end ticks;
- Deadline misses or forced terminations;
- CPU idle time.

**Example**
```
[ 21 ms ] Task_A start
[ 26 ms ] Task_A end
[ 40 ms ] Task_B start
[ 47 ms ] Task_B deadline miss → terminated
```

```
[   0] A RELEASE
[   0] A START
[   5] A COMPLETE
[  10] A RELEASE
[  10] B RELEASE
[  10] A START
[  12] B START
[  25] B DEADLINE_MISS (D=15 @ tick 25)
[  30] A OVERRUN → SKIP
[  40] A RELEASE
...
```
### Testing Suite
Develop an automated test suite to validate the scheduler's correctness and robustness. The suite must include:
- Stress tests (e.g. overlapping HRT tasks);
- Edge-case tests (e.g. minimal time gaps);
- Preemption and timing consistency checks.

Each test must:
- Produce human-readable summaries;
- Include automatic pass/fail checks for regression testing.

### Configuration Framework
Create an high-level configuration framework (e.g a Python script) that enables the user to describe the target problem, perform schedulability analysis and eventually generate the FreeRTOS skeleton application.

## Development Roadmap (Test-Driven Development)
Every phase will start with the definition of the test suite, and the expected results in the Trace System.

### Phase 0: Setup
Setting a working QEMU enviroment and a reliable Trace System.
- **Goals**:
    - QEMU enviroment working for all members;
    - Trace Module capable of recording events (Context Switch, Tick, User Events) to an in-memory buffer with a precise timestamp;
    - Produce a post-processing script that reads the buffer and generates a readable report.
- **Test**:
    - Create two simple FreeRTOS tasks with a delay;
    - Keeping track of the log with the Trace;
```
[Tick X] Task A Start
[Tick Y] Task A End
```

### Phase 1: Configuration and Data Structures
- **Goals**:
    - Define C data structures;
    - Implement a ```vConfigureScheduler``` which allocates memory, creates FreeRTOS Task Control Blocks (TCBs) and puts them in a "Suspended State", ready to be handled by the timeline scheduler.
- **Test**:
    - In ```main.c``` define a complex timeline and call ```vConfigureScheduler```;
    - When inspecting memory, the scheduler's internal table must contain exactly the past data, sorted chronologically by start-time.

### Phase 2: Timekeeper
Implement the Major Frame logic without forcing the task switch yet.
- **Goal**:
    - Modify ```xTaskIncrementTick``` (in ```task.c```);
    - The system needs to know what sub-frame it is in and when the Major Frame starts again;
    - Reset of the timeline at the end of the Major Frame.
- **Test**:
    - Configure a Major Frame of 50ms (50 ticks) and execute the system. The Trace must log a "Major Frame Reset" event every 50 ticks exactly.

### Phase 3: Hard Real-Time (HRT) Scheduling
Replace the priority-based scheduler.
- **Goals**:
    - Modify ```vTaskSwitchContext```;
    - Configure two tasks HRT_A and HRT_B in two slots (e.g. 10-20ms and 30-40ms).
- **Test**: The Trace must display HRT_A active exactly at tick 10 and suspended at tick 20. CPU must be idle between 20 and 30.

### Phase 4: Deadline Management and Kill Policy
- **Goal**:
    - If an HRT Task arrives at the end of ots slot and it's still running, kill it;
    - Errors handle and logging
- **Test**:
    - Create a task with an infinite loop and kill it. Log everything.

### Phase 5: Soft Real-Time (SRT) Scheduling
- **Goal**:
    - Task executed during CPU Idle Time, to be executed in a predefined order.
    - When a HRT Task arrives, SRT Task must be preempted immediately.
- **Test**:
    - Write a HRT Task (0-10), two SRT Tasks and another HRT Task and verify that SRT tasks do not delay the start of a HRT Task.