#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

void *SJFArgs(int quantum, SchedulerType scheduler)
{
	SchedSJFArgs *args = (SchedSJFArgs *)malloc(sizeof(SchedSJFArgs));
	if (!args)
	{
		assert(0 && "malloc failed setting scheduler arguments");
	}

	args->quantum = !(scheduler & ~(SJF_PREDICT_PREEMPTIVE | SRTF)) ? quantum : 0;
	args->prediction = (scheduler < (SJF_PREDICT | SJF_PREDICT_PREEMPTIVE));
	args->preemptive = !(scheduler & ~(SJF_PREDICT_PREEMPTIVE | SRTF));
	return args;
}


/**
 * @brief This function iterates through the list of processes and calculates the prediction time for each process.
 * The prediction time is determined by taking a weighted average of the previous prediction and the current burst time.
 * If the burst time is greater than the quantum, the quantum is used instead.
 * The process with the shortest prediction time is returned.
 *
 * @param items The list of processes to choose from and calculate the prediction.
 * @param quantum The time of the quantum to use in the prediction calculation.
 * @return The process with the shortest prediction time.
 *
 */
FakePCB *prediction(ListItem *items, int quantum)
{
	FakePCB *proc;
	FakePCB *shortProcess = NULL;
	double shortPrediction = __DBL_MAX__;

	

	while ((proc = (FakePCB *)items) != NULL)
	{
		double currPrediction = PREDICTION_WEIGHT * ((proc->duration < quantum) ? proc->duration : quantum) +
								(1 - PREDICTION_WEIGHT) * ((ProcSJFArgs *)proc->args)->previousPrediction;

		if (currPrediction < shortPrediction)
		{
			shortPrediction = currPrediction;
			shortProcess = proc;
			((ProcSJFArgs *)proc->args)->previousPrediction = currPrediction;
		}
		items = items->next;
	}

	return shortProcess;
}

/**
 * @brief Simulate a step of the fake OS process scheduler SJF with prediction.
 * This function selects the process with the shortest prediction time from the ready list,
 * updates its duration to 0, removes it from the ready list, and adds it to the running list.
 * If the selected process has a CPU event with a duration greater than the quantum,
 * a new CPU event with a duration equal to the quantum is added to the front of its event list,
 * and the duration of the original CPU event is reduced by the quantum.
 *
 * @param os The fake OS instance
 * @param args_ The arguments for the SJF scheduler
 */
void schedSJF(FakeOS *os, void *args_)
{
	// look for the first process in ready
	// if none, return
	if (!os->ready.first)
		return;

	SchedSJFArgs *args = (SchedSJFArgs *)args_;
	FakePCB *pcb = NULL;

	/*********************** SJF with prediction ***********************/
	// look for the process with the shortest prediction time
	if (args->prediction)
	{
		pcb = prediction(os->ready.first, args->quantum);
		if (pcb)
			pcb->duration = 0;
	}
	/*********************** pure SJF case (no prediction) ***********************/
	// look for the process with the shortest CPU burst time
	else
	{
		// sort the ready list
		List_sort(&os->ready, cmp);
		pcb = (FakePCB *)os->ready.first;
	}

	// remove it from the ready list
	pcb = (FakePCB *)List_detach(&os->ready, (ListItem *)pcb);

	// put it in running list (first empty slot)
	int i = 0;
	FakePCB **running = os->running;
	while (running[i])
		++i;
	running[i] = pcb;

	/*********************** SJF Preemptive ***********************/ 
	if (args->preemptive)
		sched_preemption(pcb, args->quantum);
};
