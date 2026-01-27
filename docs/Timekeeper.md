# TIMEKEEPER
## Role of the Timekeeper in the System

In this project, the Timekeeper is a core system component responsible for
maintaining a global, deterministic notion of time on top of FreeRTOS.

Its purpose is to support the implementation of a timeline-based,
time-triggered scheduler by providing precise information about:

- the current system tick,
- the current position within the Major Frame,
- the active sub-frame (Minor Cycle),
- the exact instant at which the Major Frame restarts.
The Timekeeper performs pure temporal bookkeeping and does not take
scheduling decisions.

## Timing Infrastructure
### FreeRTOS Tick as the Global Time Base

The system uses the FreeRTOS kernel tick as the single source of time.
- The tick counter is incremented by the RTOS tick interrupt.
- The tick frequency is defined by configTICK_RATE_HZ.
- Tick duration is fixed and deterministic.

All temporal quantities in the system (Major Frame, sub-frames, task start and end times) are expressed as integer multiples of the FreeRTOS tick.

This design choice ensures:
- portability (e.g. QEMU Cortex-M),
- compatibility with the FreeRTOS kernel,
- tick-level determinism required by the trace and testing infrastructure.

## Major Frame

The Major Frame (MF) is the fundamental repeating unit of the
timeline-based scheduling model.

It defines a fixed-duration temporal cycle that repeats indefinitely and
represents the complete scheduling horizon of the system.

In classical cyclic executives, the Major Frame is often defined as the
Least Common Multiple (LCM) of all task periods, ensuring that all periodic tasks
are released at least once per cycle.

In this project, the Major Frame duration is explicitly defined at compile
time and expressed in FreeRTOS ticks:
MajorFrameDuration = MF_ticks

At the end of each Major Frame:
- the Major Frame relative time counter is reset,
- the temporal execution pattern restarts from its initial state.
- This guarantees deterministic repetition across Major Frames.

## Sub-frames (Minor Cycles)

The Major Frame is subdivided into sub-frames, also referred to as
Minor Cycles.

Each sub-frame represents a fixed execution window within the Major Frame and
serves as a temporal container for one or more tasks in the scheduler.

In traditional time-triggered scheduling theory, the Minor Cycle is often chosen
as the Greatest Common Divisor (GCD) of task periods. In this system, sub-frames
are instead:
- statically defined,
- configured at compile time,
- expressed in FreeRTOS ticks,
- contiguous and non-overlapping.

The sum of all sub-frame durations equals the Major Frame duration:
MF_ticks = Σ SubFrame[i].duration

A necessary condition for schedulability is that the total execution time of
tasks assigned to a sub-frame does not exceed the sub-frame duration.

### Relationship Between Tick, Major Frame, and Sub-frame

The Timekeeper maintains two notions of time:
- Global tick: the absolute FreeRTOS tick count.
- Major Frame tick: the tick position relative to the current Major Frame.

The Major Frame relative tick is computed as:
mf_tick = global_tick % MF_ticks

The active sub-frame is determined by the condition:
SubFrame.start ≤ mf_tick < SubFrame.end


This allows the system to precisely identify:
- the current temporal window,
- sub-frame transitions,
- the exact instant at which the Major Frame restarts.

## Integration with FreeRTOS

The Timekeeper is integrated into the FreeRTOS kernel time management
infrastructure by extending the tick update logic.

At each tick:
- the global tick counter advances,
- the Major Frame relative tick is updated,
- Major Frame wrap-around is detected.

This integration enables precise temporal tracking while preserving
compatibility with the FreeRTOS kernel.

## Cyclic Behavior

System time follows a cyclic sequence:
[ Major Frame n ] → [ Major Frame n+1 ] → ...

Each Major Frame is temporally identical to the previous one.


## Project Assumptions
1. The FreeRTOS tick is monotonic and stable.
2. configTICK_RATE_HZ is constant.
3. All temporal boundaries align with tick granularity.
4. No sub-tick precision is required.
5. Clock drift compensation is outside the scope of the system.
6. The Timekeeper does not perform scheduling decisions.


### Timekeeper Responsibilities

The Timekeeper:
- maintains the global tick counter,
- computes the Major Frame relative tick,
- identifies the current sub-frame,
- detects and signals Major Frame restart events.

### Explicit Non-Responsibilities

The Timekeeper does not:
- create or destroy tasks,
- suspend or resume tasks,
- modify task priorities,
- distinguish between HRT and SRT tasks,
- enforce deadlines or kill policies.

The separation between time management and scheduling logic is
intentional and fundamental to the system design.

## Conclusion

The Timekeeper establishes a deterministic, tick-aligned temporal framework that
serves as the foundation for the timeline-based scheduler.

By isolating temporal logic from scheduling logic, the system becomes:
- predictable,
- analyzable,
- testable with tick-level precision,
- compliant with real-time system design principles.
