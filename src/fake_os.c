#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../include/fake_os.h"

char usage_buffer[1024] = "Usage: %s <num_processes> <scheduler> \n\
\n\
<num_processes>: Number of processes to create and simulate. \n\
<scheduler>: The scheduling algorithm to use: \n\
	1: First Come First Served (FCFS) \n\
	2: Shortest Job First (SJF) with prediction \n\
	3: Shortest Job First (SJF) preemptive with prediction \n\
	4: Shortest Job First (SJF) no prediction \n\
	5: Shortest Remaining Time First (SRTF) (a sjf with preemptive)\n\
	6: Priority \n\
	7: Priority preemptive \n\
	8: Round Robin (RR) \n\
	9: Multi-Level Feedback Queue (MLFQ) \n\
\n\
Example: ./disastros 10 2 \n\
\n\
This command will create and simulate 10 processes using the SJF with prediction & quantum scheduling algorithm.\n";



void printPCB(ListItem *item)
{
	FakePCB *pcb = (FakePCB *)item;
	printf("PID: %d\n", pcb->pid);
	printf("Priority: %d\n", pcb->priority);
	printf("Duration: %d\n", pcb->duration);
	// List_print(&pcb->events, printProcessEvent);
}

/**
 * @brief Increase the duration of the running processes
 *
 * @param os
 */
void increaseDuration(FakeOS *os)
{
	FakePCB *pcb;
	int i = -1;

	while (++i < os->cores && (pcb = *(os->running + i)))
	{
		if (pcb->pid)
			pcb->duration++;
	}
}

/**
 * @brief Check if all cores are taken by a process
 *
 * @param running
 * @return int
 */
int cpuFull(FakePCB **running, int cores)
{
	int i = -1;
	while (++i < cores)
	{
		if (!running[i] || !(*running[i]).pid)
			return 0;
	}
	return 1;
}

/**
 * @brief Initialize the fake OS structure
 *
 * @param os
 */
void FakeOS_init(FakeOS *os, int cores)
{
	os->running = calloc(cores, sizeof(FakePCB *));
	List_init(&os->ready);
	List_init(&os->waiting);
	List_init(&os->processes);
	os->timer = 0;
	os->schedule_fn = 0;
	os->cores = cores;
}

/**
 * @brief Set the scheduler function and arguments
 *
 * @param os
 * @param scheduler
 */
void FakeOS_setScheduler(FakeOS *os, SchedulerType scheduler)
{
    assert(scheduler >= FCFS && scheduler <= MLFQ && "illegal scheduler");
    assert(os && "null pointer");

    void *args = NULL;  // Variabile generica per gestire i diversi tipi di args

    switch (scheduler)
    {
    case FCFS:
        os->schedule_fn = 0;
        os->schedule_args = 0;
        break;

    case SJF_PREDICT:
    case SJF_PREDICT_PREEMPTIVE:
    case SJF_PURE:
	case SRTF:
        if ((args = malloc(sizeof(SchedSJFArgs)))) {
            SchedSJFArgs *srr_args = (SchedSJFArgs *)args;
            srr_args->quantum = !(scheduler & ~(SJF_PREDICT_PREEMPTIVE | SRTF)) ? QUANTUM : 0;
            srr_args->prediction = (scheduler < (SJF_PREDICT | SJF_PREDICT_PREEMPTIVE));
            srr_args->preemptive = !(scheduler & ~(SJF_PREDICT_PREEMPTIVE | SRTF));
        } else {
            assert(0 && "malloc failed setting scheduler arguments");
        }
        os->schedule_fn = schedSJF;
        break;
	case PRIORITY:
	case PRIORITY_PREEMPTIVE:
		if ((args = malloc(sizeof(SchedPriorArgs)))) {
			SchedPriorArgs *prior_args = (SchedPriorArgs *)args;
			prior_args->quantum = (scheduler == PRIORITY_PREEMPTIVE) ? QUANTUM : 0;
			prior_args->preemptive = (scheduler == PRIORITY_PREEMPTIVE);
		} else {
			assert(0 && "malloc failed setting scheduler arguments");
		}
		os->schedule_fn = schedPriority;
		break;
	case RR:
        if ((args = malloc(sizeof(SchedRRArgs)))) {
            SchedRRArgs *rr_args = (SchedRRArgs *)args;
            rr_args->quantum = QUANTUM;
        } else {
            assert(0 && "malloc failed setting scheduler arguments");
        }
        os->schedule_fn = schedRR;
        break;
    case MLFQ:
        os->schedule_fn = 0;
        break;

    default:
        assert(0 && "illegal scheduler");
    }

	// set the scheduler arguments and type
    os->scheduler = scheduler;
    os->schedule_args = args;
}
/**
 * @brief
 *
 * @param p
 * @param num_processes
 * @param num_bursts_per_process
 * @return int
 */
int FakeOS_createEventProc(FakeProcess *p, int num_bursts_per_process)
{
	assert(p && "null pointer");

	// Definition of the histograms for CPU burst and I/O burst durations for the processes
	BurstHistogram cpu_burst_hist[] = {
		{5, 0.15},	// 15% of bursts last 5 ms
		{10, 0.30}, // 30% of bursts last 10 ms
		{20, 0.50}, // 50% of bursts last 20 ms
		{40, 0.05}	// 5% of bursts last 40 ms
	};

	BurstHistogram io_burst_hist[] = {
		{4, 0.25}, // 25% of bursts last 4 ms
		{8, 0.50}, // 50% of bursts last 8 ms
		{12, 0.25} // 25% of bursts last 12 ms
	};

	static unsigned int pid = 1;
	int process_arrived = rand() % 10;
	int cpu_histogram_size = sizeof(cpu_burst_hist) / sizeof(BurstHistogram);
	int io_histogram_size = sizeof(io_burst_hist) / sizeof(BurstHistogram);

	p->pid = pid++;
	p->arrival_time = process_arrived;
	p->priority = (rand() % 10) + 1;
	List_init(&p->events);
	p->list.prev = p->list.next = 0;

	// Create a number of bursts for the process based on the number of bursts per process
	// and the histograms for CPU and I/O burst
	// The process will have alternating CPU and I/O bursts
	for (int i = 0; i < num_bursts_per_process; i++)
	{
		int cpu_burst = generateBurstDuration(cpu_burst_hist, cpu_histogram_size, 1);
		int io_burst = generateBurstDuration(io_burst_hist, io_histogram_size, 1);

		ProcessEvent *cpu_event = (ProcessEvent *)malloc(sizeof(ProcessEvent));
		cpu_event->list.prev = cpu_event->list.next = 0;
		cpu_event->type = CPU;
		cpu_event->duration = cpu_burst;
		List_pushBack(&p->events, (ListItem *)cpu_event);

		ProcessEvent *io_event = (ProcessEvent *)malloc(sizeof(ProcessEvent));
		io_event->list.prev = io_event->list.next = 0;
		io_event->type = IO;
		io_event->duration = io_burst;
		List_pushBack(&p->events, (ListItem *)io_event);
	}

	// number of events for a single process, given by sum of CPU burst
	// and I/O burst.
	return 2 * num_bursts_per_process;
}

/**
 * @brief Create a process and put it in the list of processes to be created
 *
 * @param os The fake OS instance
 * @param proc_num The process number to create
 */
void FakeOS_createProcess(FakeOS *os, int proc_num)
{
	FakeProcess new_process = {0};
	int num_events = FakeOS_createEventProc(&new_process, BURST_PROCESS);
	printf("created [Process: %d], pid: %d, events:%d, arrival_time:%d\n",
		   proc_num, new_process.pid, num_events, new_process.arrival_time);
	if (num_events)
	{
		FakeProcess *new_process_ptr = (FakeProcess *)malloc(sizeof(FakeProcess));
		*new_process_ptr = new_process;
		List_pushBack(&os->processes, (ListItem *)new_process_ptr);
	}
}

/**
 * @brief Create a PCB for a process and put it in the ready or waiting list
 *
 * @param os The fake OS instance
 * @param p The process to create the PCB for
 */
void FakeOS_createPcb(FakeOS *os, FakeProcess *p)
{
	// sanity check
	assert(p->arrival_time == os->timer && "time mismatch in creation");
	// we check that in the list of PCBs there is no
	// pcb having the same pid
	int i = -1;
	while (++i < os->cores)
	{
		FakePCB *running = os->running[i];
		assert((!running || running->pid != p->pid) && "pid taken");
	}

	ListItem *aux = os->ready.first;
	while (aux)
	{
		FakePCB *pcb = (FakePCB *)aux;
		assert(pcb->pid != p->pid && "pid taken");
		aux = aux->next;
	}

	aux = os->waiting.first;
	while (aux)
	{
		FakePCB *pcb = (FakePCB *)aux;
		assert(pcb->pid != p->pid && "pid taken");
		aux = aux->next;
	}

	// all fine, no such pcb exists
	FakePCB *new_pcb = (FakePCB *)malloc(sizeof(FakePCB));
	new_pcb->list.next = new_pcb->list.prev = 0;
	new_pcb->pid = p->pid;
	new_pcb->events = p->events;
	new_pcb->priority = p->priority;
	new_pcb->duration = 0;
	FakeProcess_setArgs(new_pcb, os->scheduler);

	assert(new_pcb->events.first && "process without events");

	// depending on the type of the first event
	// we put the process either in ready or in waiting
	ProcessEvent *e = (ProcessEvent *)new_pcb->events.first;
	switch (e->type)
	{
	case CPU:
		List_pushBack(&os->ready, (ListItem *)new_pcb);
		printf(ANSI_ORANGE "\t\tmove to ready\n" ANSI_RESET);
		break;
	case IO:
		List_pushBack(&os->waiting, (ListItem *)new_pcb);
		printf(ANSI_YELLOW "\t\tmove to waiting\n" ANSI_RESET);
		break;
	default:
		assert(0 && "illegal resource");
		;
	}
}

/**
 * @brief Simulate a step of the fake OS process scheduler
 *
 * @param os
 */
void FakeOS_simStep(FakeOS *os)
{
	printf("\n************** TIME: %08d **************\n", os->timer);

	/*
	Scan the list of processes waiting to be started
	Create all processes that are scheduled to start at the current time
	Place the newly created processes in the ready list or waiting list
	depending on the type of their first event
	*/
	ListItem *aux = os->processes.first;
	while (aux)
	{
		FakeProcess *proc = (FakeProcess *)aux;
		FakeProcess *new_process = 0;
		// if process is scheduled to start now (arrival_time == timer) create it and put it in ready or waiting list
		// and remove it from the list of processes to be created (processes)
		if (proc->arrival_time == os->timer)
		{
			new_process = proc;
		}
		aux = aux->next;
		if (new_process)
		{
			printf(ANSI_CYAN "\t[+] new process coming - pid : %2d\n" ANSI_RESET, new_process->pid);
			new_process = (FakeProcess *)List_detach(&os->processes, (ListItem *)new_process);
			FakeOS_createPcb(os, new_process);
			memset(new_process, 0, sizeof(FakeProcess));
			free(new_process);
			new_process = 0;
		}
	}

	/********************************* READY QUEUE *********************************/

	// scan ready list, and print the pid of the process in the ready list
	aux = os->ready.first;
	printf(ANSI_BLUE "\nREADY QUEUE:\n" ANSI_RESET);
	while (aux)
	{
		FakePCB *pcb = (FakePCB *)aux;
		aux = aux->next;
		ProcessEvent *e = (ProcessEvent *)pcb->events.first;
		assert(e->type == CPU);
		printf(ANSI_BLUE "\tPID: %2d - CPU_burst: %2d - Priority: %2d\n" ANSI_RESET, pcb->pid, e->duration, pcb->priority);
	}

	/********************************* WAITING QUEUE *********************************/

	// scan waiting list, and put in ready all items whose event terminates
	aux = os->waiting.first;
	printf(ANSI_MAGENTA "\nWAITING QUEUE:\n" ANSI_RESET);
	while (aux)
	{
		FakePCB *pcb = (FakePCB *)aux;
		aux = aux->next;
		ProcessEvent *e = (ProcessEvent *)pcb->events.first;
		assert(e->type == IO);
		printf(ANSI_MAGENTA "\tPID: %2d - remaining time : %2d\n" ANSI_RESET, pcb->pid, --(e->duration));
		if (e->duration == 0)
		{
			List_popFront(&pcb->events);
			memset(e, 0, sizeof(ProcessEvent));
			free(e);
			e = 0;
			List_detach(&os->waiting, (ListItem *)pcb);
			if (pcb->events.first)
			{
				// handle next event
				e = (ProcessEvent *)pcb->events.first;
				switch (e->type)
				{
				case CPU:
					printf(ANSI_ORANGE "\t\t[!] end burst -> move to ready\n" ANSI_RESET);
					List_pushBack(&os->ready, (ListItem *)pcb);
					break;
				case IO:
					printf(ANSI_YELLOW "\t\t[!] end burst -> move to waiting\n" ANSI_RESET);
					List_pushBack(&os->waiting, (ListItem *)pcb);
					break;
				}
			}
			else
			{
				// kill process
				printf(ANSI_RED "\t\t[-] end process\n" ANSI_RESET);
				memset(pcb, 0, sizeof(FakePCB));
				free(pcb);
				pcb = 0;
			}
		}
	}

	/********************************* RUNNING QUEUE *********************************/

	/*
	scan running list, and put in ready all items whose event terminates
	decrement the duration of running
	if event over, destroy event
	and reschedule process
	if last event, destroy running
	*/

	FakePCB **running;
	int i = -1;
	printf(ANSI_GREEN "\nRUNNING QUEUE:\n" ANSI_RESET);
	while (++i < os->cores)
	{
		// call schedule, if defined
		running = &os->running[i];

		// if no process is running, skip this core
		if (!(*running) || !(*running)->pid)
		{
			printf("\tPID: -1 on Core: %d\n", i);
			continue;
		}
		// printf(ANSI_GREEN "\tpid: %d on core: %d\n" ANSI_RESET, (*running)->pid ? (*running)->pid : -1, i);
		if ((*running)->pid)
		{
			ProcessEvent *e = (ProcessEvent *)(*running)->events.first;
			assert(e->type == CPU);
			printf(ANSI_GREEN "\tPID: %2d on Core: %2d - remaining time : %2d\n" ANSI_RESET, (*running)->pid, i, --(e->duration));
			if (e->duration == 0)
			{
				List_popFront(&(*running)->events);
				memset(e, 0, sizeof(ProcessEvent));
				free(e);
				e = 0;
				if ((*running)->events.first)
				{
					e = (ProcessEvent *)(*running)->events.first;
					switch (e->type)
					{
					case CPU:
						printf(ANSI_ORANGE "\t\t[!] end burst -> move to ready\n" ANSI_RESET);
						List_pushBack(&os->ready, (ListItem *)*running);
						break;
					case IO:
						printf(ANSI_YELLOW "\t\t[!] end burst -> move to waiting\n" ANSI_RESET);
						List_pushBack(&os->waiting, (ListItem *)*running);
						break;
					}
				}
				else
				{
					printf(ANSI_RED "\t\t[-] end process\n" ANSI_RESET);
					memset(*running, 0, sizeof(FakePCB));
					free(*running); // kill process
					*running = 0;
				}
				// set running to 0 to signal that the core is free
				*running = 0;
			}
		}
	}

	/********************************* SCHEDULING *********************************/

	i = -1;
	while (++i < os->cores)
	{
		// call schedule, if defined
		if (os->schedule_fn && !cpuFull(os->running, os->cores))
		{
			(*os->schedule_fn)(os, os->schedule_args);
		}

		// (FCFS) First Come First Served scheduling approach
		// if running not defined and ready queue not empty
		// put the first in ready to run
		else if (!cpuFull(os->running, os->cores) && os->ready.first)
		{
			FakePCB **temp = os->running;
			while (*temp)
			{
				if (*temp && !(*temp)->pid)
				{
					*temp = (FakePCB *)List_popFront(&os->ready);
					break;
				}
				(*temp)++;
			}
		}
	}

	increaseDuration(os);
	++os->timer;
}

/**
 * @brief Destroy the fake OS structure and free the memory
 *
 * @param os
 */
void FakeOS_destroy(FakeOS *os)
{
	memset(os->running, 0, sizeof(FakePCB *) * os->cores);
	free(os->running);
	if (os->schedule_args)
	{
		memset(os->schedule_args, 0, sizeof(SchedSJFArgs));
		free(os->schedule_args);
	}
	os->running = 0;
	os->schedule_args = 0;
}




int main(int argc, char **argv)
{
	FakeOS os;

	if (argc < 3)
	{
		printf(usage_buffer, argv[0]);
		return 1;
	}

	srand(time(NULL));

	FakeOS_init(&os, CORES);
	FakeOS_setScheduler(&os, atoi(argv[2]) - 1);

	int num_processes = atoi(argv[1]);

	// create the processes and put them in the processes list
	for (int i = 0; i < num_processes; ++i)
	{
		FakeOS_createProcess(&os, i);
	}

	// run the simulation until all processes are terminated and all queues are empty
	// memcmp returns 0 if the two arrays are equal (in this case, all the pointers are NULL)
	FakePCB **temp = malloc(sizeof(FakePCB *) * os.cores);
	memset(temp, 0, sizeof(FakePCB *) * os.cores);
	while (memcmp(os.running, temp, sizeof(FakePCB *) * os.cores) ||
		   os.ready.first ||
		   os.waiting.first ||
		   os.processes.first)
	{
		FakeOS_simStep(&os);
	}
	memset(temp, 0, sizeof(FakePCB *) * os.cores);
	free(temp);
	temp = 0;
	FakeOS_destroy(&os);
}
