#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

/**
 * @brief Print the processes in the ready queue.
 * 
 * @param args The arguments of the MLQ scheduler.
 */
void MLQ_printQueue(SchedMLQArgs *sched_args)
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
 * @brief Create the arguments for the MLQ scheduler.
 * 
 * @param quantum The quantum of the scheduler.
 * @return void* The arguments of the MLQ scheduler.
 */
void *MLQArgs(int quantum)
{
    int high_prior_queue;

    SchedMLQArgs *args = (SchedMLQArgs *)malloc(sizeof(SchedMLQArgs));
    if (!args)
        assert(0 && "malloc failed setting scheduler arguments");
    if(!(args->schedule_args = (void **)malloc(sizeof(void *) * MAX_PRIORITY)))
        assert(0 && "malloc failed setting scheduler arguments");
    if(!(args->schedule_fn = (ScheduleFn *)malloc(sizeof(ScheduleFn *) * MAX_PRIORITY)))
        assert(0 && "malloc failed setting scheduler arguments");
    if (!(args->ready = (ListHead *)malloc(sizeof(ListHead) * MAX_PRIORITY)))
        assert(0 && "malloc failed setting scheduler arguments");
    
    // Set number of high and low priority queues as 70% and 30% of total queues
    high_prior_queue = (int)(MAX_PRIORITY * 0.70);
    high_prior_queue = (high_prior_queue == MAX_PRIORITY) ? high_prior_queue - 1 : high_prior_queue;

    args->num_ready_queues = MAX_PRIORITY;
    args->high_priority_queues = high_prior_queue;

    // Set the quantum for each queue
    for (int i = 0; i < high_prior_queue; i++)
    {
        List_init(&args->ready[i]);
        args->schedule_args[i] = RRArgs(quantum, RR);
        args->schedule_fn[i] = schedRR;
        // increment the quantum for the next queue by 40%
        quantum += quantum * 0.40;
    }

    for (int i = high_prior_queue; i < MAX_PRIORITY; i++)
    {
        List_init(&args->ready[i]);
        args->schedule_args[i] = FCFSArgs(0, FCFS);
        args->schedule_fn[i] = schedFCFS;
    }

    return args;
}

/**
 * @brief Destroy the arguments of the MLQ scheduler and free the memory.
 * 
 * @param args_ The arguments of the MLQ scheduler.
 */
void MLQ_destroyArgs(void *args_)
{
    SchedMLQArgs *args = (SchedMLQArgs *)args_;
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
 * @brief Enqueue a process in the MLQ scheduler.
 * 
 * @param os The fake OS instance.
 * @param pcb The process to enqueue.
 */
void MLQ_enqueue(FakeOS *os, FakePCB *pcb)
{
    SchedMLQArgs *sched_args = (SchedMLQArgs *)os->schedule_args;
    ProcMLQArgs *proc_args = (ProcMLQArgs *)pcb->args;

    List_pushBack(&sched_args->ready[proc_args->queue], (ListItem *)pcb);
}


static 
void MLQ_dispatcher(FakeOS *os, SchedMLQArgs *args, int start, int end, int *time_counter)
{
    for (int i = start; i < end; i++)
    {
        if (args->ready[i].size > 0)
        {
            os->ready = args->ready[i];
            (*args->schedule_fn[i])(os, args->schedule_args[i]);
            args->ready[i] = os->ready;
            (*time_counter)++;
            return;
        }
    }
}

/**
 * @brief Schedule a process in the MLQ scheduler, 80% time for high priority queues and 20% for low priority queues.
 * 
 * @param os The fake OS instance.
 * @param args_ The arguments of the MLQ scheduler.
 */
void schedMLQ(FakeOS *os, void *args_)
{
    // times spent on high and low priority queues
    static int hpq_time = 0;
    static int lpq_time = 0;

    SchedMLQArgs *args = (SchedMLQArgs *)args_;

    // total time spent on both queues
    int total_time = hpq_time + lpq_time;

    // check if high priority queues should be scheduled
    // if the total time is 0 or the time spent on high priority queues is less than 80% of the total time
    int should_schedule_hpq = (total_time == 0) || (hpq_time < 0.8 * total_time);

    // variable to check if the high priority queue or the low priority queue is empty
    short int hpq_empty = 1;
    short int lpq_empty = 1;

    for (int i = 0; i < args->num_ready_queues; i++)
    {
        int is_hpq = (i < args->high_priority_queues);

        if (args->ready[i].size > 0)
        {
            if (is_hpq)
                hpq_empty = 0;
            else
                lpq_empty = 0;
        }
    }

    if (should_schedule_hpq && !hpq_empty)
        MLQ_dispatcher(os, args, 0, args->high_priority_queues, &hpq_time);
    else if (!lpq_empty)
        MLQ_dispatcher(os, args, args->high_priority_queues, args->num_ready_queues, &lpq_time);
    else if (!hpq_empty)
        MLQ_dispatcher(os, args, 0, args->high_priority_queues, &hpq_time);
}