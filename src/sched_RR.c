#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

void *RRArgs(int quantum, SchedulerType scheduler)
{
    SchedRRArgs *args = (SchedRRArgs *)malloc(sizeof(SchedRRArgs));
    if (!args)
    {
        assert(0 && "malloc failed setting scheduler arguments");
    }
    args->quantum = !(scheduler ^ RR) ? quantum : 0;
    return args;
}

void schedRR(struct FakeOS *os, void *args_)
{
    // look for the first process in ready
    // if none, return

    // prendo la coda tramite get_queue
    if (!os->ready.first)
        return;

    SchedRRArgs *args = (SchedRRArgs *)args_;
    FakePCB *pcb = NULL;

    // take the first process in ready queue 
    pcb = (FakePCB *)List_popFront(&os->ready);
    pcb->duration = 0;

	/*********************** RR Preemptive ***********************/
    // Preempt the current CPU burst event if it exceeds the given quantum
    sched_preemption(pcb, args->quantum);

    // put it in running list (first empty slot)
    int i = 0;
    FakePCB **running = os->running;
    while (running[i])
        ++i;
    running[i] = pcb;
};