#pragma once

#include "fake_process.h"
#include "linked_list.h"
#include "scheduler.h"

#define ANSI_ORANGE "\x1b[38;5;208m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"


#define CORES 4 // number of cores in the fake OS 
#define QUANTUM 10 // quantum for the prediction scheduler


typedef struct FakePCB
{
	ListItem list;
	int pid;
	void *args;
	int priority; // used for priority scheduling 
	int duration;
	ListHead events;
} FakePCB;

typedef enum SchedulerType
{
	FCFS,
	SJF_PREDICT,
	SJF_PREDICT_PREEMPTIVE,
	SJF_PURE,
	RR,
	MLFQ,
	SRTF
} SchedulerType;

struct  FakeOS;
typedef void (*ScheduleFn)(struct FakeOS *os, void *args);

typedef struct FakeOS
{
	FakePCB **running;
	ListHead ready;
	ListHead waiting;
	int timer;
	int cores;
	SchedulerType scheduler;
	ScheduleFn schedule_fn;
	void *schedule_args;
	ListHead processes;
} FakeOS;


void printPCB(ListItem *item);
void FakeOS_init(FakeOS *os, int cores);
void FakeOS_simStep(FakeOS *os);
void FakeOS_destroy(FakeOS *os);