#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"


void *PriorArgs(int quantum, SchedulerType scheduler)
{
    SchedPriorArgs *args = (SchedPriorArgs *)malloc(sizeof(SchedPriorArgs));
    if (!args)
    {
        assert(0 && "malloc failed setting scheduler arguments");
    }
    args->preemptive = (scheduler == PRIORITY_PREEMPTIVE);
    args->quantum = (scheduler == PRIORITY_PREEMPTIVE) ? quantum : 0;
    return args;
}

void resetAging(FakePCB *pcb)
{
    ProcPriorArgs *args = (ProcPriorArgs *)pcb->args;
    args->curr_priority = pcb->priority;
}

void agingProc(ProcPriorArgs *args, int currTimer)
{
    // max priority level to be incremented
    ProcessPriority max_proc_priority = HIGH;

    if (currTimer - args->last_aging >= args->agingThreshold && args->curr_priority > max_proc_priority)
    {
        args->last_aging = currTimer;
        args->curr_priority--;
    }
}

FakePCB *getByPriority(FakeOS *os)
{
    FakePCB *pcb;
    FakePCB *highest_priority_pcb = NULL;
    ListItem *item = os->ready.first;

    while ((pcb = (FakePCB *)item) != NULL) {
        // Aging process if wasn't scheduled for a long time
        agingProc(((ProcPriorArgs *)pcb->args), os->timer);
        if (!highest_priority_pcb || ((ProcPriorArgs *)pcb->args)->curr_priority < ((ProcPriorArgs *)highest_priority_pcb->args)->curr_priority)
        {
            highest_priority_pcb = pcb;
        }
        item = item->next;
    }

    return highest_priority_pcb;
}

void schedPriority(FakeOS *os, void *args_) 
{
    if (!os->ready.first)
        return;

    SchedPriorArgs *args = (SchedPriorArgs *)args_;
    FakePCB *pcb = getByPriority(os);

    // Rimuovi il processo dalla ready queue
    List_detach(&os->ready, (ListItem *)pcb);

    // Metti il processo nella running list (primo slot vuoto)
    schedule(os, pcb);

    assert(pcb->events.first);
    ProcessEvent *e = (ProcessEvent *)pcb->events.first;
    assert(e->type == CPU);

	/*********************** Priority Preemptive ***********************/
    // Preempt the current CPU burst event if it exceeds the given quantum
	if (args->preemptive)
		sched_preemption(pcb, args->quantum);
    
    resetAging(pcb);
}