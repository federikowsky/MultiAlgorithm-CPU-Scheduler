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
    args->preemptive = !(scheduler ^ PRIORITY_PREEMPTIVE);
    args->quantum = !(scheduler ^ PRIORITY_PREEMPTIVE) ? quantum : 0;
    return args;
}


FakePCB *getByPriority(ListHead queue)
{
    FakePCB *pcb;
    FakePCB *highest_priority_pcb = NULL;
    ListItem *item = queue.first;

    while ((pcb = (FakePCB *)item) != NULL) {
        if (!highest_priority_pcb || pcb->priority < highest_priority_pcb->priority) {
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
    FakePCB *pcb = getByPriority(os->ready);

    // Rimuovi il processo dalla ready queue
    List_detach(&os->ready, (ListItem *)pcb);

    // Metti il processo nella running list (primo slot vuoto)
    int i = 0;
    while (os->running[i])
        ++i;
    os->running[i] = pcb;

    assert(pcb->events.first);
    ProcessEvent *e = (ProcessEvent *)pcb->events.first;
    assert(e->type == CPU);

	/*********************** Priority Preemptive ***********************/
    // Preempt the current CPU burst event if it exceeds the given quantum
	if (args->preemptive)
		sched_preemption(pcb, args->quantum);
}