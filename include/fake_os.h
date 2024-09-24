#pragma once

#include "fake_process.h"
#include "linked_list.h"
#include "stats.h"

#define ANSI_ORANGE "\x1b[38;5;208m"
#define ANSI_GREY "\x1b[38;5;240m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

#define PREDICTION_WEIGHT 0.125 // 1/8 
#define AGING_TIME 100
#define AGING_FACTOR 5 // 5 times the mean burst time


struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS *os, void *args);

typedef enum SchedulerType
{
	FCFS,
	FCFS_PREEMPTIVE,
	SJF_PREDICT,
	SJF_PREDICT_PREEMPTIVE,
	SJF_PURE,
	SRTF,
	PRIORITY,
	PRIORITY_PREEMPTIVE,
	RR,
	MLFQ,
	MAX_SCHEDULERS // add a new scheduler before this one
} SchedulerType;


typedef struct
{
	int quantum;
	int preemptive;
} SchedFCFSArgs;

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



typedef struct FakeOS
{
	unsigned int timer;
	unsigned int cores;
	FakePCB **running;
	ListHead ready;
	ListHead waiting;
	SchedulerType scheduler;
	ScheduleFn schedule_fn;
	void *schedule_args; 
	const char *histogram_file;
	ListHead processes;

	// Statistiche
	ListHead terminated_stats;
	unsigned int cpu_busy_time; 
} FakeOS;


void printPCB(ListItem *item);

// function auxiliar for the scheduler
int weightedMeanQuantum(const char *histogram_file);
float calculateAgingThreshold(const BurstHistogram *hist, int size);
void schedule(FakeOS *os, FakePCB *pcb);
void sched_preemption(struct FakePCB *pcb, int quantum);
int cmp(ListItem *a, ListItem *b);
void resetAging(FakePCB *pcb);

void *FCFSArgs(int quantum, SchedulerType scheduler);
void *SJFArgs(int quantum, enum SchedulerType scheduler);
void *PriorArgs(int quantum, enum SchedulerType scheduler);
void *RRArgs(int quantum, enum SchedulerType scheduler);

void schedFCFS(FakeOS *os, void *args_);
void schedSJF(FakeOS *os, void *args_);
void schedRR(FakeOS *os, void *args_);
void schedPriority(FakeOS *os, void *args_);


void FakeOS_loadHistogram(const char *filename, BurstHistogram *cpu_hist, int *cpu_size, BurstHistogram *io_hist, int *io_size);
void FakeOS_procUpdateStats(FakeOS *os, FakePCB *pcb, ProcStatsType type);