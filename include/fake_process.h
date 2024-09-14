#pragma once

#include "linked_list.h"
#include "stats.h"

#define PREDICTION_WEIGHT 0.125 // 1/8 

// forward declaration
struct FakePCB;
enum SchedulerType;

typedef enum
{
	CPU = 0,
	IO = 1
} ResourceType;

// event of a process, is in a list
typedef struct
{
	ListItem list;
	ResourceType type;
	int duration;
} ProcessEvent;

// fake process
typedef struct
{
	ListItem list;
	int pid; // assigned by us
	void *args;
	int arrival_time;
	int priority;
	ListHead events;
} FakeProcess;


int FakeProcess_save(const FakeProcess *p, const char *filename);
void printProcessEvent(ListItem *item);
void FakeProcess_SJFArgs(struct FakePCB *pcb);
void FakeProcess_setArgs(struct FakePCB *pcb, enum SchedulerType scheduler);