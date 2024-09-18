#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

void *MLFQArgs(int quantum)
{
    int high_prior_queue;

    SchedMLFQArgs *args = (SchedMLFQArgs *)malloc(sizeof(SchedMLFQArgs));
    if (!args)
        assert(0 && "malloc failed setting scheduler arguments");
    if (!(args->quantum = (int *)malloc(sizeof(int) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    if(!(args->schedule_args = (void **)malloc(sizeof(void *) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    if(!(args->schedule_fn = (ScheduleFn *)malloc(sizeof(ScheduleFn *) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    if (!(args->ready = (ListHead *)malloc(sizeof(ListHead) * MLFQ_QUEUES)))
        assert(0 && "malloc failed setting scheduler arguments");
    
    args->num_ready_queues = MLFQ_QUEUES;

    // Set number of high and low priority queues as 70% and 30% of total queues
    high_prior_queue = (int)(MLFQ_QUEUES * 0.70);
    high_prior_queue = (high_prior_queue == MLFQ_QUEUES) ? high_prior_queue - 1 : high_prior_queue;

    // Set the quantum for each queue
    for (int i = 0; i < high_prior_queue; i++)
    {
        List_init(&args->ready[i]);
        args->quantum[i] = quantum;
        args->schedule_args[i] = RRArgs(args->quantum[i], RR);
        args->schedule_fn[i] = schedRR;
        // increment the quantum for the next queue by 40%
        quantum += quantum * 0.40;
    }

    for (int i = high_prior_queue; i < MLFQ_QUEUES; i++)
    {
        List_init(&args->ready[i]);
        args->quantum[i] = 0;
        args->schedule_args[i] = 0;
        args->schedule_fn[i] = schedFCFS;
    }

    return args;
}


void promote_process(FakeOS *os, FakePCB *pcb)
{
    if (os->scheduler == MLFQ && pcb->priority > 0)
    {
        pcb->priority--;
        pcb->promotion = 0;
    }
}

void demote_process(FakeOS *os, FakePCB *pcb)
{
    if (os->scheduler == MLFQ)
    {
        int max_priority = ((SchedMLFQArgs *)os->schedule_args)->num_ready_queues - 1;
        if (pcb->priority < max_priority)
        {
            pcb->priority++;
            pcb->promotion = 0;
        }
    }
}

void aging_proc(FakeOS *os)
{
    if (os->scheduler == MLFQ)
    {
        SchedMLFQArgs *args = (SchedMLFQArgs *)os->schedule_args;
        for (int i = 0; i < args->num_ready_queues; i++)
        {
            FakePCB *pcb;
            ListItem *item = args->ready[i].first;
            while ((pcb = (FakePCB *)item) != NULL)
            {
                if (os->timer - pcb->last_enqueued_time > AGING)
                    promote_process(os, pcb);
                item = item->next;
            }
        }
    }
}


void schedMLFQ(FakeOS *os, void *args_)
{
    SchedMLFQArgs *args = (SchedMLFQArgs *)args_;
    for (int i = 0; i < args->num_ready_queues; i++) {
        if (args->ready[i].size > 0) {
            // Assign the ready queue to the OS struct
            os->ready = args->ready[i];
            // Call the scheduler function
            (*args->schedule_fn[i])(os, args->schedule_args[i]);
            return;
        }
    }
    return;
}