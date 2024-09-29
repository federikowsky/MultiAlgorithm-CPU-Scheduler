#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

/**
 * @brief Print the processes in the ready queue.
 * 
 * @param args The arguments of the MLFQ scheduler.
 */
void MLFQ_printQueue(SchedMLFQArgs *sched_args)
{
    for (int i = 0; i < sched_args->num_ready_queues; i++)
    {
        ListItem *aux = sched_args->ready[i].first;
        printf(ANSI_BLUE "\nREADY QUEUE %d:\n" ANSI_RESET, i);
        while (aux)
        {
            FakePCB *pcb = (FakePCB *)aux;
            ProcessEvent *e = (ProcessEvent *)pcb->events.first;
            assert(e->type == CPU);
            printf(ANSI_BLUE "\tPID: %2d - CPU_burst: %3d - Priority: %-8s\n" ANSI_RESET, 
                pcb->pid, e->duration, print_priority(pcb->priority));
            aux = aux->next;
        }
    }
}

/**
 * @brief Create the arguments for the MLFQ scheduler.
 * 
 * @param quantum The quantum of the scheduler.
 * @return void* The arguments of the MLFQ scheduler.
 */
void *MLFQArgs(int quantum, float aging_threshold)
{
    int high_prior_queue;

    SchedMLFQArgs *args = (SchedMLFQArgs *)malloc(sizeof(SchedMLFQArgs));
    if (!args)
        assert(0 && "malloc failed setting scheduler arguments");
    if(!(args->schedule_args = (void **)malloc(sizeof(void *) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    if(!(args->schedule_fn = (ScheduleFn *)malloc(sizeof(ScheduleFn *) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    if (!(args->ready = (ListHead *)malloc(sizeof(ListHead) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    
    args->num_ready_queues = MLFQ_QUEUES;
    args->agingThreshold = aging_threshold;

    // Set number of high and low priority queues as 70% and 30% of total queues
    high_prior_queue = (int)(MLFQ_QUEUES * 0.70);
    high_prior_queue = (high_prior_queue == MLFQ_QUEUES) ? high_prior_queue - 1 : high_prior_queue;

    // Set the quantum for each queue
    for (int i = 0; i < high_prior_queue; i++)
    {
        List_init(&args->ready[i]);
        args->schedule_args[i] = RRArgs(quantum, RR);
        args->schedule_fn[i] = schedRR;
        // increment the quantum for the next queue by 40%
        quantum += quantum * 0.40;
    }

    for (int i = high_prior_queue; i < MLFQ_QUEUES; i++)
    {
        List_init(&args->ready[i]);
        args->schedule_args[i] = FCFSArgs(0, FCFS);
        args->schedule_fn[i] = schedFCFS;
    }

    return args;
}

/**
 * @brief Destroy the arguments of the MLFQ scheduler and free the memory.
 * 
 * @param args_ The arguments of the MLFQ scheduler.
 */
void MLFQ_destroyArgs(void *args_)
{
    SchedMLFQArgs *args = (SchedMLFQArgs *)args_;
    for (int i = 0; i < args->num_ready_queues; i++)
    {
        free(args->schedule_args[i]);
    }
    free(args->ready);
    free(args->schedule_args);
    free(args->schedule_fn);
    free(args);
}

/**
 * @brief Promote a process to a higher priority queue.
 * 
 * @param args The arguments of the MLFQ scheduler.
 * @param pcb The process to promote.
 */
void promote_process(SchedMLFQArgs *sched_args, FakePCB *pcb)
{
    // if the process is already in the highest priority queue, don't promote
    ProcMLFQArgs *proc_args = (ProcMLFQArgs *)pcb->args;

    if (proc_args->queue > 0)
    {
        List_detach(&sched_args->ready[proc_args->queue], (ListItem *)pcb);
        proc_args->queue--;
        List_pushBack(&sched_args->ready[proc_args->queue], (ListItem *)pcb);
    }
}

/**
 * @brief Demote a process to a lower priority queue.
 * 
 * @param args The arguments of the MLFQ scheduler.
 * @param pcb The process to demote.
 */
void demote_process(SchedMLFQArgs *sched_args, FakePCB *pcb)
{
    // if the process is already in the lowest priority queue, don't demote
    ProcMLFQArgs *proc_args = (ProcMLFQArgs *)pcb->args;

    if (proc_args->queue < sched_args->num_ready_queues - 1)
    {
        List_detach(&sched_args->ready[proc_args->queue], (ListItem *)pcb);
        proc_args->queue++;
        List_pushBack(&sched_args->ready[proc_args->queue], (ListItem *)pcb);
    }
}

/**
 * @brief Aging of the processes in the ready queues.
 * 
 * @param args The arguments of the MLFQ scheduler.
 * @param currTimer The current time of the OS.
 */
void MLFQ_aging(SchedMLFQArgs *sched_args, unsigned int currTimer)
{
    // 1 cause the first queue is not subject to aging 
    for (int i = 1; i < sched_args->num_ready_queues; i++)
    {
        ListItem *aux = sched_args->ready[i].first;
        FakePCB *pcb;
        ProcMLFQArgs *proc_args;

        while (aux)
        {
            pcb = (FakePCB *)aux;
            proc_args = (ProcMLFQArgs *)pcb->args;
            ProcessStats *proc_stats = pcb->stats;

            if (proc_args->queue > 0)
            {
                if (currTimer - proc_stats->last_ready_enqueue >= sched_args->agingThreshold && 
                    currTimer - proc_args->last_aging >= sched_args->agingThreshold)
                {
                    promote_process(sched_args, pcb);
                    proc_args->last_aging = currTimer;
                }
            }
            aux = aux->next;
        }
    }
}

/**
 * @brief Enqueue a process in the MLFQ scheduler.
 * 
 * @param os The fake OS instance.
 * @param pcb The process to enqueue.
 */
void MLFQ_enqueue(FakeOS *os, FakePCB *pcb)
{
    SchedMLFQArgs *sched_args = (SchedMLFQArgs *)os->schedule_args;
    ProcMLFQArgs *proc_args = (ProcMLFQArgs *)pcb->args;

    if (pcb->quantum_used)
    {
        demote_process(sched_args, pcb);
        pcb->quantum_used = 0;
    }
    else
        List_pushBack(&sched_args->ready[proc_args->queue], (ListItem *)pcb);
}

/**
 * @brief Schedule a process in the MLFQ scheduler.
 * 
 * @param os The fake OS instance.
 * @param args_ The arguments of the MLFQ scheduler.
 */
void schedMLFQ(FakeOS *os, void *args_)
{
    SchedMLFQArgs *args = (SchedMLFQArgs *)args_;

    MLFQ_aging(args, os->timer);

    for (int i = 0; i < args->num_ready_queues; i++)
    {
        if (args->ready[i].size > 0)
        {
            // Assign the ready queue to the OS struct
            os->ready = args->ready[i];
            // Call the scheduler function
            (*args->schedule_fn[i])(os, args->schedule_args[i]);
            // reassign the ready queue to save the changes made by the scheduler
            args->ready[i] = os->ready;
            
            return;
        }
    }
}