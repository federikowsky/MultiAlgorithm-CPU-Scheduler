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
	ProcSJFArgs args;
	args.previousPrediction = 0;

	pcb->args = &args;
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
	case SJF_PURE:
	default:
		return ;
	}
}


int FakeProcess_save(const FakeProcess *p, const char *filename)
{
	FILE *f = fopen(filename, "w");
	if (!f)
		return -1;
	fprintf(f, "PROCESS %d %d\n", p->pid, p->arrival_time);
	ListItem *aux = p->events.first;
	int num_events = 0;
	while (aux)
	{
		ProcessEvent *e = (ProcessEvent *)aux;
		switch (e->type)
		{
		case CPU:
			fprintf(f, "CPU_BURST %d\n", e->duration);
			++num_events;
			break;
		case IO:
			fprintf(f, "IO_BURST %d\n", e->duration);
			++num_events;
			break;
		default:;
		}
		aux = aux->next;
	}
	fclose(f);
	return num_events;
}
