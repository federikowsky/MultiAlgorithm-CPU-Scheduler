#pragma once

#include "linked_list.h"
#include "stats.h"


// forward declaration
enum SchedulerType;

typedef enum
{
	CPU = 0,
	IO = 1
} ResourceType;

typedef enum processPriority
{
	REALTIME,
	HIGH,
	NORMAL,
	IDLE,
	BATCH,
	MAX_PRIORITY // add a new priority before this one 
} ProcessPriority;

typedef enum procstatstype
{
	ARRIVAL_TIME,
	WAITING_TIME,
	READY_ENQUEUE,
	TURNAROUND_TIME,
	RESPONSE_TIME,
	COMPLETE_TIME
} ProcStatsType;



typedef struct
{
    double previousPrediction;
} ProcSJFArgs;

typedef struct
{
	unsigned int last_aging;
	ProcessPriority curr_priority;
} ProcPriorArgs;

typedef struct 
{
	int queue;
	unsigned int last_aging;
} ProcMLFQArgs;



typedef struct ProcessEvent
{
	ListItem list;
	ResourceType type;
	int duration;
} ProcessEvent;

typedef struct ProcessStats
{
	ListItem list;
	unsigned int arrival_time;
	unsigned int waiting_time;
	unsigned int last_ready_enqueue;
	unsigned int turnaround_time;
	unsigned int response_time;
	unsigned int complete_time;
} ProcessStats;

typedef struct FakeProcess
{
	ListItem list;
	int pid;
	void *args;
	int arrival_time;
	ProcessPriority priority;
	ListHead events;
} FakeProcess;

typedef struct FakePCB
{
	ListItem list;
	int pid;
	int duration;
	int quantum_used;
	void *args;
	ProcessStats *stats;
	ProcessPriority priority;
	ListHead events;
} FakePCB;


void printProcessEvent(ListItem *item);
void FakeProcess_SJFArgs(FakePCB *pcb);
void FakeProcess_setArgs(FakePCB *pcb, enum SchedulerType scheduler);
ProcessStats *FakeProcess_initiStats();
void FakeProcess_arrivalTime(FakePCB *pcb, unsigned int timer);
void FakeProcess_lastEnqueuedTime(FakePCB *pcb, unsigned int timer);
void FakeProcess_waitingTime(FakePCB *pcb, unsigned int timer);
void FakeProcess_completeTime(FakePCB *pcb, unsigned int timer);
void FakeProcess_turnaroundTime(FakePCB *pcb, unsigned int timer);
void FakeProcess_responseTime(FakePCB *pcb, unsigned int timer);
