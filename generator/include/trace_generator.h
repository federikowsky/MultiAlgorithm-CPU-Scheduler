#pragma once

#include "linked_list.h"

typedef enum processPriority
{
	REALTIME,
	HIGH,
	NORMAL,
	IDLE,
	BATCH,
	MAX_PRIORITY // add a new priority before this one
} ProcessPriority;



typedef struct {
    int burst_time; 
    float probability;
} BurstDist;

typedef struct {
    ListItem list;
    int cpu_size;
    int io_size;
    BurstDist *cpu_hist;
    BurstDist *io_hist;
    char *source_type;
} BurstProfile;

typedef struct {
    int num_processes;
    const char *histogram_folder;
    int burst_per_process;
    ListHead burst_profiles;
} TraceGen;

void TraceGen_init(TraceGen *tg, int num_processes, int burst_per_process, const char *histogram_folder);
void createEventProc(BurstProfile *bf, int proc_id, int bursts_per_process, const char *dest_folder);
BurstProfile *BurstProfile_loadHistogram(const char *filename);
int getBurstDuration(BurstDist *hist, int size, int num_samples);