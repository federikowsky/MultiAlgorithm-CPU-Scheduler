#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/fake_os.h"

void printProcessEvent(ListItem *item)
{
	ProcessEvent *e = (ProcessEvent *)item;
	switch (e->type)
	{
	case CPU:
		printf("CPU: %d\n", e->duration);
		break;
	case IO:
		printf("IO: %d\n", e->duration);
		break;
	default:
		assert(0 && "illegal resource");
	}
}

/**
 * @brief Arguments for the SJF scheduler.
 * 
 * @param pcb The process control block to set the arguments for.
 */
void FakeProcess_SJFArgs(FakePCB *pcb)
{
	ProcSJFArgs *args = (ProcSJFArgs *)malloc(sizeof(ProcSJFArgs));
	args->previousPrediction = 0;

	pcb->args = args;
}

void FakeProcess_PriorArgs(FakePCB *pcb)
{
	ProcPriorArgs *args = (ProcPriorArgs *)malloc(sizeof(ProcPriorArgs));
	args->last_aging = 0;
	args->curr_priority = pcb->priority;

	pcb->args = args;
}

void FakeProcess_MLFQArgs(FakePCB *pcb)
{
	ProcMLFQArgs *args = (ProcMLFQArgs *)malloc(sizeof(ProcMLFQArgs));
	args->queue = 0;
	args->last_aging = 0;

	pcb->args = args;
}

/**
 * @brief populate the arguments of the process 
 * 
 * @param pcb 
 * @param scheduler 
 */
void FakeProcess_setArgs(FakePCB *pcb, SchedulerType scheduler)
{
	switch (scheduler)
	{
	case SJF_PREDICT:
	case SJF_PREDICT_PREEMPTIVE:
		FakeProcess_SJFArgs(pcb);
		break;
	case PRIORITY:
	case PRIORITY_PREEMPTIVE:
		FakeProcess_PriorArgs(pcb);
		break;
	case MLFQ:
		FakeProcess_MLFQArgs(pcb);
		break;
	default:
		pcb->args = NULL;
		return ;
	}
}

/**
 * @brief Initialize the statistics of the process
 * 
 * @return ProcessStats* 
 */
ProcessStats *FakeProcess_initiStats()
{
	ProcessStats *stats = (ProcessStats *)malloc(sizeof(ProcessStats));
	if (!stats)
		assert(0 && "malloc failed setting process stats");
	
	stats->list.next = stats->list.prev = 0;
	stats->arrival_time = 0;
	stats->waiting_time = 0;
	stats->last_ready_enqueue = 0;
	stats->turnaround_time = 0;
	stats->response_time = 0;
	stats->complete_time = 0;

	return stats;
}

/**
 * @brief Update the arrival time of the process, the time the process arrived in the system
 * 
 * @param pcb 
 * @param timer 
 */
void FakeProcess_arrivalTime(FakePCB *pcb, unsigned int timer)
{
	pcb->stats->arrival_time = timer;
}

/**
 * @brief Update the last enqueued time of the process, the last time the process was in the ready queue
 * 
 * @param pcb 
 * @param timer 
 */
void FakeProcess_lastEnqueuedTime(FakePCB *pcb, unsigned int timer)
{
	pcb->stats->last_ready_enqueue = timer;
}

/**
 * @brief Update the waiting time of the process, how long the process has been waiting in the ready queue
 * 
 * @param pcb 
 * @param timer 
 */
void FakeProcess_waitingTime(FakePCB *pcb, unsigned int timer)
{
	if (pcb->stats->last_ready_enqueue)
		pcb->stats->waiting_time += timer - pcb->stats->last_ready_enqueue;
}

/**
 * @brief Update the complete time of the process, the time the process completed execution
 * 
 * @param pcb 
 * @param timer 
 */
void FakeProcess_completeTime(FakePCB *pcb, unsigned int timer)
{
	pcb->stats->complete_time = timer;
}

/**
 * @brief Update the turnaround time of the process, the time the process effectively spent in the system
 * 
 * @param pcb 
 * @param timer 
 */
void FakeProcess_turnaroundTime(FakePCB *pcb, unsigned int timer)
{
	pcb->stats->turnaround_time = pcb->stats->complete_time - pcb->stats->arrival_time;
}

/**
 * @brief Update the response time of the process, the time the process took to start executing
 * 
 * @param pcb 
 * @param timer 
 */
void FakeProcess_responseTime(FakePCB *pcb, unsigned int timer)
{
	if (!pcb->stats->response_time)
		pcb->stats->response_time = timer - pcb->stats->arrival_time;
}

