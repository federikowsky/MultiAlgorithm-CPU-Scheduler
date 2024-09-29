#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

void Prior_printQueue(FakeOS *os)
{
    ListItem *aux = os->ready.first;
    printf(ANSI_BLUE "\nREADY QUEUE:\n" ANSI_RESET);
    while (aux)
    {
        FakePCB *pcb = (FakePCB *)aux;
        ProcPriorArgs *proc_args = (ProcPriorArgs *)pcb->args;
        ProcessEvent *e = (ProcessEvent *)pcb->events.first;
        assert(e->type == CPU);
        printf(ANSI_BLUE "\tPID: %2d - CPU_burst: %3d - CurrPriority: %-8s -  BasePriority: %-8s\n" ANSI_RESET, 
            pcb->pid, e->duration, print_priority(proc_args->curr_priority), print_priority(pcb->priority));
        aux = aux->next;
    }
}

void *PriorArgs(int quantum, float aging_threshold, SchedulerType scheduler)
{
    SchedPriorArgs *args = (SchedPriorArgs *)malloc(sizeof(SchedPriorArgs));
    if (!args)
        assert(0 && "malloc failed setting scheduler arguments");
    args->preemptive = (scheduler == PRIORITY_PREEMPTIVE);
    args->quantum = (scheduler == PRIORITY_PREEMPTIVE) ? quantum : 0;
    args->agingThreshold = aging_threshold;
    return args;
}

void resetAging(FakePCB *pcb)
{
    ProcPriorArgs *proc_args = (ProcPriorArgs *)pcb->args;
    proc_args->curr_priority = pcb->priority;
}

void agingProc(FakeOS *os, FakePCB *pcb)
{
    // max priority level to be incremented
    ProcessPriority max_proc_priority = HIGH;

    int currTimer = os->timer;
    SchedPriorArgs *sched_args = (SchedPriorArgs *)os->schedule_args;
    ProcPriorArgs *proc_args = (ProcPriorArgs *)pcb->args;
    ProcessStats *proc_stats = pcb->stats;

    if (proc_args->curr_priority > max_proc_priority)
    {
        if (currTimer - proc_stats->last_ready_enqueue >= sched_args->agingThreshold && 
            currTimer - proc_args->last_aging >= sched_args->agingThreshold)
        {
            proc_args->last_aging = currTimer;
            proc_args->curr_priority--;
        }
    }
}

FakePCB *getByPriority(FakeOS *os)
{
    FakePCB *pcb;
    FakePCB *highest_priority_pcb = NULL;
    ListItem *item = os->ready.first;

    while ((pcb = (FakePCB *)item) != NULL) {
        // Aging process if wasn't scheduled for a long time
        agingProc(os, pcb);
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

    SchedPriorArgs *sched_args = (SchedPriorArgs *)args_;
    FakePCB *pcb = getByPriority(os);

    // Rimuovi il processo dalla ready queue
    List_detach(&os->ready, (ListItem *)pcb);

    // Metti il processo nella running list (primo slot vuoto)
    dispatcher(os, pcb);

    assert(pcb->events.first);
    ProcessEvent *e = (ProcessEvent *)pcb->events.first;
    assert(e->type == CPU);

	/*********************** Priority Preemptive ***********************/
    // Preempt the current CPU burst event if it exceeds the given quantum
	if (sched_args->preemptive)
		sched_preemption(pcb, sched_args->quantum);
    
    resetAging(pcb);
}