#pragma once

#include "fake_process.h"
#include "linked_list.h"

#define ANSI_ORANGE "\x1b[38;5;208m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"


#define CORES 3 // number of cores in the fake OS 
#define QUANTUM 10 // quantum for the prediction scheduler
#define AGING 50 // aging time for the MLFQ scheduler
#define MLFQ_QUEUES 3 // number of queues in the MLFQ scheduler

struct  FakeOS;
typedef void (*ScheduleFn)(struct FakeOS *os, void *args);

typedef enum SchedulerType
{
	FCFS,
	SJF_PREDICT,
	SJF_PREDICT_PREEMPTIVE,
	SJF_PURE,
	SRTF,
	PRIORITY,
	PRIORITY_PREEMPTIVE,
	RR,
	MLFQ
} SchedulerType;

typedef enum processPriority
{
	IDLE,
	NORMAL,
	HIGH,
	REALTIME,
	MAX_PRIORITY // add a new priority before this one 
} ProcessPriority;

typedef struct 
{
	int quantum;
	int prediction;
	int preemptive;
} SchedSJFArgs;

typedef struct 
{
	int quantum;
} SchedRRArgs;

typedef struct 
{
	int preemptive;
	int quantum;
} SchedPriorArgs;

typedef struct 
{
	int num_ready_queues;
	int *quantum;
	void **schedule_args;
	ScheduleFn *schedule_fn;
	ListHead *ready;
} SchedMLFQArgs;

typedef struct
{
    double previousPrediction;
} ProcSJFArgs;


typedef struct FakePCB
{
	ListItem list;
	int pid;
	int priority;
	int duration;
	int promotion; // da assegnare 
	unsigned int last_enqueued_time;
	void *args;
	ListHead events;
} FakePCB;

typedef struct FakeOS
{
	int timer;
	int cores;
	FakePCB **running;
	ListHead ready;
	ListHead waiting;
	SchedulerType scheduler;
	ScheduleFn schedule_fn;
	void *schedule_args;
	ListHead processes;
} FakeOS;


void printPCB(ListItem *item);

// function auxiliar for the scheduler 
void sched_preemption(struct FakePCB *pcb, int quantum);
int cmp(ListItem *a, ListItem *b);

void *RRArgs(int quantum, enum SchedulerType scheduler);
void *SJFArgs(int quantum, enum SchedulerType scheduler);
void *PriorArgs(int quantum, enum SchedulerType scheduler);
void *MLFQArgs(int quantum);

void schedSJF(struct FakeOS *os, void *args_);
void schedRR(struct FakeOS *os, void *args_);
void schedPriority(struct FakeOS *os, void *args_);
void schedMLFQ(struct FakeOS *os, void *args_);
void schedFCFS(struct FakeOS *os, void *args_);

// function for MLFQ scheduler
void promote_process(struct FakeOS *os, struct FakePCB *pcb);
void demote_process(struct FakeOS *os, struct FakePCB *pcb);
void aging_proc(FakeOS *os);