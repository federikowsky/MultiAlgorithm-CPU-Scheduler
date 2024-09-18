#include <assert.h>
#include <stdlib.h>

#include "../include/fake_os.h"

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
        newEvent->list.prev = newEvent->list.next = 0;
        newEvent->type = CPU;
        // Set the duration of the new CPU burst event to the quantum
        newEvent->duration = quantum;
        // Update the duration of the current CPU burst event
        event->duration -= quantum;

        // Set the process as promotion to indicate that it was promotion
        pcb->promotion = -1;

        // Add the new CPU burst event to the process's event list
        List_pushFront(&pcb->events, (ListItem *)newEvent);
    } else
    {
        pcb->promotion = 1;
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


