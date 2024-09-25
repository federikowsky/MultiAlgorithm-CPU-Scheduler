#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

void *FCFSArgs(int quantum, SchedulerType scheduler)
{
    SchedFCFSArgs *args = (SchedFCFSArgs *)malloc(sizeof(SchedFCFSArgs));
    if (!args)
    {
        assert(0 && "malloc failed setting scheduler arguments");
    }
    args->quantum = (scheduler == FCFS_PREEMPTIVE) ? quantum : 0;
    args->preemptive = (scheduler == FCFS_PREEMPTIVE);
    return args;
}

void schedFCFS(struct FakeOS *os, void *args_)
{
    // look for the first process in ready
    // if none, return
    if (!os->ready.first)
        return;

    SchedFCFSArgs *args = (SchedFCFSArgs *)args_;
    FakePCB *pcb = (FakePCB *)List_popFront(&os->ready);
    pcb->duration = 0;

    // put it in running list (first empty slot)
    dispatcher(os, pcb);

	/*********************** FCFS Preemptive ***********************/
    // Preempt the current CPU burst event if it exceeds the given quantum
	if (args->preemptive)
        sched_preemption(pcb, args->quantum);
};