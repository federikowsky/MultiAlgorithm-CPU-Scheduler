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
    double previousPrediction;
} ProcSJFArgs;

void schedSJF(struct FakeOS *os, void *args_);
