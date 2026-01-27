## Group 47

Operating Systems for Embedded Systems Project 2025/26.

## Description
This project implements a deterministic, time-triggered scheduler for FreeRTOS, designed to replace the default priority-based scheduler with a **static timeline-driven execution model**.
Instead of relying on dynamic priorities and runtime decisions, tasks are executed at predefined instants within a repeating major frame, ensuring predictable timing, bounded jitter, and full repeatability across cycles.

This approach differentiates itself from:
- the standard FreeRTOS priority scheduler, which is event-driven and non-deterministic in execution order under load;
- rate-monotonic or EDF schedulers, which still rely on dynamic priority arbitration.

## Contributing
This is a **university project**, and contributions are currently limited in scope.

> External contributions are not accepted at the moment.

## Authors
|NAME|ID|
|-|-|
|Samuel Bruno|F641850|
|Gaetano Carbone|359776|
|Lorenzo Deplano|361511|
|Elsa Francesca Ionita|361804|
|Mattia Martello|361907|

## Project status
In Development

For more information about the project consult the full [Development Roadmap](./Instruction.md)