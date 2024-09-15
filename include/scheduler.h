#pragma once

// forward declaration
struct FakeOS;

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
    double previousPrediction;
} ProcSJFArgs;


void Sched_preemption(struct FakePCB *pcb, int quantum);
int cmp(ListItem *a, ListItem *b);

void schedSJF(struct FakeOS *os, void *args_);
void schedRR(struct FakeOS *os, void *args_);
void schedPriority(struct FakeOS *os, void *args_);
