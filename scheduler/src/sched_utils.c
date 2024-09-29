#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/fake_os.h"


/**
 * @brief Schedule a process to run on a core.
 *
 * This function updates the process's stats and adds it to the list of running processes.
 *
 * @param os Pointer to the FakeOS structure.
 * @param pcb Pointer to the FakePCB structure representing the process.
 */
void dispatcher(FakeOS *os, FakePCB *pcb)
{
    int i = 0;
	FakePCB **running = os->running;
	while (running[i])
		++i;
	running[i] = pcb;   
    FakeOS_procUpdateStats(os, pcb, WAITING_TIME); 

#ifdef _SBS_DEBUG_
	// to debug the simulation step by step press enter to continue
	getchar();
#endif
}

/**
 * @brief Preempt the current CPU burst event if it exceeds the given quantum.
 *
 * This function checks if the current CPU burst duration exceeds the specified quantum. 
 * If so, it creates a new CPU burst event for the remaining duration and updates the duration of the
 * current CPU burst event. The new event is then added to the process's event list.
 *
 * @param pcb Pointer to the FakePCB structure representing the process.
 * @param quantum The maximum duration of a CPU burst.
 */
void sched_preemption(FakePCB *pcb, int quantum)
{
    assert(pcb->events.first);
    ProcessEvent *event = (ProcessEvent *)pcb->events.first;
    assert(event->type == CPU);

    if (event->duration > quantum) 
    {
        // Create a new CPU burst event for the remaining duration
        ProcessEvent *newEvent = (ProcessEvent *)malloc(sizeof(ProcessEvent));
        if (!newEvent)
            assert(0 && "malloc failed creating new CPU burst event");
        newEvent->list.prev = newEvent->list.next = 0;
        newEvent->type = CPU;
        // Set the duration of the new CPU burst event to the quantum
        newEvent->duration = quantum;
        // Update the duration of the current CPU burst event
        event->duration -= quantum;

        pcb->quantum_used = 1;

        // Add the new CPU burst event to the process's event list
        List_pushFront(&pcb->events, (ListItem *)newEvent);
    }
}

/**
 * @brief Comparison function to sort processes by the duration of their next CPU burst.
 *
 * @param a First list item to compare.
 * @param b Second list item to compare.
 * @return int Negative if a < b, zero if a == b, positive if a > b.
 */
int cmp(ListItem *a, ListItem *b)
{
	FakePCB *procA = (FakePCB *)a;
	FakePCB *procB = (FakePCB *)b;

	// Ensure both processes have events
	assert(procA->events.first);
	assert(procB->events.first);

	ProcessEvent *eventA = (ProcessEvent *)procA->events.first;
	ProcessEvent *eventB = (ProcessEvent *)procB->events.first;

	// Ensure both events are CPU events
	assert(eventA->type == CPU);
	assert(eventB->type == CPU);

	return eventA->duration - eventB->duration;
}

