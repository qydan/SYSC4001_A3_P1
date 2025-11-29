### Scheduler Simulation Report

#### Overview

This document details the implementation of the Scheduler Simulator. The simulator is designed as an Ideal System with no overhead transitions to match the provided test cases output file.

#### Real System Comparison (Overhead Analysis)

While our simulation uses idealized test cases, a real OS doesn't switch tasks instantaneously. In a realistic environment, every hardware interaction has a delay or overhead which has also been implemented but has been commented out, it can be found in `interrupts_AydanEng_EricCui.hpp`.

If this were mimicing a real system including ISR and Context Switch latency, the timeline would be pushed forward by 13-14ms for every single state change:

| Operation | Overhead Cost | Components in a Real System |
| :--- | :--- | :--- |
| Context Switch | 14 ms | Switch (1) + Save (10) + Vector (1) + Load Addr (1) + IRET (1) |
| System Call (I/O Request) | 13 ms| Switch (1) + Save (4) + Vector (1) + Load Addr (1) + ISR Addr (1) + Driver (2) + Check (1) + Send (1) + IRET (1) |
| End I/O (Interrupt) | 13 ms| Switch (1) + Save (4) + Vector (1) + Load Addr (1) + ISR Addr (1) + Store (2) + Reset (1) + Standby (1) + IRET (1) |

Impact: In a real-world implementation, these overheads would cause significant drift from the idealized timestamps.