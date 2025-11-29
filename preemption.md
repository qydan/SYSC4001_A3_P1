### Logic Explanation: Round Robin Implementation in EP+RR

This block found in `interrupts_AydanEng_EricCui_EP_RR.cpp` handles the specific requirement to implement Round Robin (100ms timeout) within a Priority Scheduler, despite the contradiction that every process has a unique PID, thus a unique priority as priority is based on PID.

```cpp
if (!ready_queue.empty()) {
    sort_by_priority(ready_queue);
    // Force preemption to simulate RR sharing, even if priorities are unique
    preempt = true; 
} else {
    running.time_slice_time = 0; // Renew slice if running alone
}