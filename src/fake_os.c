#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../include/fake_os.h"

char usage_buffer[1024] = "Usage: %s <num_cores> <num_processes> <scheduler> <histogram_file> \n\
\n\
<num_cores>: Number of cores to simulate the processes on. \n\
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
<histogram_file>: The path to the file containing the histogram data. \n\
\n\
Example: ./disastros 4 10 2 compiler.txt\n\
\n\
This command will create 4 core and simulate 10 processes using the SJF with prediction & quantum scheduling algorithm with compiler histogram example \n";


char *print_priority(ProcessPriority priority)
{
	switch (priority)
	{
	case REALTIME:
		return "REALTIME";
	case HIGH:
		return "HIGH";
	case NORMAL:
		return "NORMAL";
	case IDLE:
		return "IDLE";
	case BATCH:
		return "BATCH";
	default:
		return "UNKNOWN";
	}
}

void printPCB(ListItem *item)
{
	FakePCB *pcb = (FakePCB *)item;
	printf("PID: %d\n", pcb->pid);
	printf("Priority: %s\n", print_priority(pcb->curr_priority));
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
void FakeOS_init(FakeOS *os, int cores, const char *histogram_file)
{
	os->running = calloc(cores, sizeof(FakePCB *));
	List_init(&os->ready);
	List_init(&os->waiting);
	List_init(&os->processes);
	List_init(&os->terminated_stats);
	os->timer = 0;
	os->schedule_fn = 0;
	os->cores = cores;
	os->histogram_file = histogram_file;
	os->cpu_busy_time = 0;
}

/**
 * @brief load the CPU and I/O histogram for a given file.
 *
 * @param filename The path to the file containing the data.
 * @param cpu_hist The array to store the calculated CPU history.
 * @param cpu_size The size of the `cpu_hist` array.
 * @param io_hist The array to store the calculated I/O history.
 * @param io_size The size of the `io_hist` array.
 */
void FakeOS_loadHistogram(const char *filename, BurstHistogram *cpu_hist, int *cpu_size, BurstHistogram *io_hist, int *io_size)
{
    char line[256];
    int is_cpu = 0, is_io = 0;
    static int cpu_count = 0, io_count = 0;
	static BurstHistogram cpu_hist_temp[100], io_hist_temp[100];
    
	if (cpu_count || io_count)
	{
		memcpy(cpu_hist, cpu_hist_temp, cpu_count * sizeof(BurstHistogram));
		memcpy(io_hist, io_hist_temp, io_count * sizeof(BurstHistogram));
		*cpu_size = cpu_count;
		*io_size = io_count;
		return;
	}

    FILE *file = fopen(filename, "r");
	assert(file && "file not found");

    while (fgets(line, sizeof(line), file))
	{
		if (line[0] == '#' || line[0] == '\n')
            continue;

        if (strncmp(line, "CPU_BURST", 9) == 0)
		{
            is_cpu = 1;
            is_io = 0;
            continue;
        }
        if (strncmp(line, "IO_BURST", 8) == 0)
		{
            is_io = 1;
            is_cpu = 0;
            continue;
        }

		// Find the position of comment if present
        char *comment_pos = strchr(line, '#');
        if (comment_pos) {
            *comment_pos = '\0'; // Truncate the line at the comment
        }

        // Parse burst time and probability
        int burst_time;
        double probability;
        if (sscanf(line, "%d %lf", &burst_time, &probability) == 2 || sscanf(line, "%d,%lf", &burst_time, &probability) == 2)
		{
            if (is_cpu)
			{
                cpu_hist_temp[cpu_count].burst_time = burst_time;
                cpu_hist_temp[cpu_count].probability = probability;
                cpu_count++;
            } 
			else if (is_io)
			{
                io_hist_temp[io_count].burst_time = burst_time;
                io_hist_temp[io_count].probability = probability;
                io_count++;
            }
        }
    }

    fclose(file);

    // Copy data from temp arrays
    memcpy(cpu_hist, cpu_hist_temp, cpu_count * sizeof(BurstHistogram));
    memcpy(io_hist, io_hist_temp, io_count * sizeof(BurstHistogram));
	*cpu_size = cpu_count;
	*io_size = io_count;
}

/**
 * @brief Set the scheduler function and arguments
 *
 * @param os
 * @param scheduler
 */
void FakeOS_setScheduler(FakeOS *os, SchedulerType scheduler)
{
    assert(os && "null pointer");

    void *args;  // Variabile generica per gestire i diversi tipi di args
	int quantum = weightedMeanQuantum(os->histogram_file); 

    switch (scheduler)
    {
    case FCFS:
        args = 0;
        os->schedule_fn = schedFCFS;
        break;
    case SJF_PREDICT:
    case SJF_PREDICT_PREEMPTIVE:
    case SJF_PURE:
	case SRTF:
		args = SJFArgs(quantum, scheduler);
        os->schedule_fn = schedSJF;
        break;
	case PRIORITY:
	case PRIORITY_PREEMPTIVE:
		args = PriorArgs(quantum, scheduler);
		os->schedule_fn = schedPriority;
		break;
	case RR:
		args = RRArgs(quantum, scheduler);
        os->schedule_fn = schedRR;
        break;
    case MLFQ:
		assert(0 && "MLFQ not implemented");
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
int FakeOS_createEventProc(FakeProcess *p, const char *filename)
{
	assert(p && "null pointer");

	// Definition of the histograms for CPU burst and I/O burst durations for the processes
	BurstHistogram cpu_burst_hist[100], io_burst_hist[100];
	int cpu_histogram_size, io_histogram_size;

	// Generate a random number of bursts for the process
	int num_bursts_per_process = MIN_BURST_PROCESS + rand() % (MAX_BURST_PROCESS - MIN_BURST_PROCESS + 1);

	// Load the histograms for CPU and I/O burst durations from the file
	FakeOS_loadHistogram(filename, cpu_burst_hist, &cpu_histogram_size, io_burst_hist, &io_histogram_size);

	// Create a number of bursts for the process based on the number of bursts per process
	// and the histograms for CPU and I/O burst
	// The process will have alternating CPU and I/O bursts
	for (int i = 0; i < num_bursts_per_process; i++)
	{
		// Generate a random number to decide if the next burst is a CPU or I/O burst
		double random_val = (double)rand() / RAND_MAX;

		if (random_val < 0.5)
		{
			// Generate a random burst duration based on the CPU burst histogram
			int cpu_burst = generateBurstDuration(cpu_burst_hist, cpu_histogram_size, 1);
			ProcessEvent *cpu_event = (ProcessEvent *)malloc(sizeof(ProcessEvent));
			cpu_event->list.prev = cpu_event->list.next = 0;
			cpu_event->type = CPU;
			cpu_event->duration = cpu_burst;
			List_pushBack(&p->events, (ListItem *)cpu_event);
		} else {
			// Generate a random burst duration based on the I/O burst histogram
			int io_burst = generateBurstDuration(io_burst_hist, io_histogram_size, 1);
			ProcessEvent *io_event = (ProcessEvent *)malloc(sizeof(ProcessEvent));
			io_event->list.prev = io_event->list.next = 0;
			io_event->type = IO;
			io_event->duration = io_burst;
			List_pushBack(&p->events, (ListItem *)io_event);
		}
	}

	// number of events for a single process, given by sum of CPU burst
	// and I/O burst.
	return num_bursts_per_process;
}

/**
 * @brief Create a process and put it in the list of processes to be created
 *
 * @param os The fake OS instance
 * @param proc_num The process number to create
 */
void FakeOS_createProcess(FakeOS *os, int proc_num)
{
	static unsigned int pid = 1;
	FakeProcess new_process = {0};

	new_process.pid = pid++;
	new_process.arrival_time = rand() % 10;
	new_process.priority = rand() % MAX_PRIORITY;
	List_init(&new_process.events);
	new_process.list.prev = new_process.list.next = 0;

	int num_events = FakeOS_createEventProc(&new_process, os->histogram_file);
	printf("created [Process: %2d], pid: %2d, events:%2d, arrival_time:%2d, priority_level: %2d\n",
		   proc_num, new_process.pid, num_events, new_process.arrival_time, new_process.priority);
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
	new_pcb->curr_priority = new_pcb->base_priority = p->priority;
	new_pcb->duration = 0;
	new_pcb->stats = FakeProcess_initiStats();
	FakeProcess_setArgs(new_pcb, os->scheduler);
	FakeOS_procUpdateStats(os, new_pcb, ARRIVAL_TIME); 

	assert(new_pcb->events.first && "process without events");

	// depending on the type of the first event
	// we put the process either in ready or in waiting
	ProcessEvent *e = (ProcessEvent *)new_pcb->events.first;
	switch (e->type)
	{
	case CPU:
		List_pushBack(&os->ready, (ListItem *)new_pcb);
		// FakeOS_procUpdateStats(os, new_pcb, READY_ENQUEUE);
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

void FakeOS_destroyPCB(FakePCB *pcb)
{
	if (pcb->args)
	{
		memset(pcb->args, 0, sizeof(ProcSJFArgs));
		free(pcb->args);
	}
	memset(pcb, 0, sizeof(FakePCB));
	free(pcb);	
}

/**
 * @brief Update the statistics of a process
 *
 * @param os
 * @param pcb
 * @param type
 */
void FakeOS_procUpdateStats(FakeOS *os, FakePCB *pcb, ProcStatsType type)
{
	switch (type)
	{
	case ARRIVAL_TIME:
		FakeProcess_arrivalTime(pcb, os->timer);
		break;
	case READY_ENQUEUE:
		FakeProcess_lastEnqueuedTime(pcb, os->timer);
		break;
	case COMPLETE_TIME:
		FakeProcess_completeTime(pcb, os->timer);
		FakeProcess_turnaroundTime(pcb, os->timer);
		break;
	case WAITING_TIME:
		FakeProcess_waitingTime(pcb, os->timer);
		FakeProcess_responseTime(pcb, os->timer);
		break;
	default:
		assert(0 && "illegal stat type");
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
					List_pushBack(&os->ready, (ListItem *)pcb);
					FakeOS_procUpdateStats(os, pcb, READY_ENQUEUE); 
					printf(ANSI_ORANGE "\t\t[!] end burst -> move to ready\n" ANSI_RESET);
					break;
				case IO:
					List_pushBack(&os->waiting, (ListItem *)pcb);
					printf(ANSI_YELLOW "\t\t[!] end burst -> move to waiting\n" ANSI_RESET);
					break;
				}
			}
			else
			{
				FakeOS_procUpdateStats(os, pcb, COMPLETE_TIME); 
				List_pushBack(&os->terminated_stats, (ListItem *)pcb->stats); 

				// kill process
				FakeOS_destroyPCB(pcb);
				printf(ANSI_RED "\t\t[-] end process\n" ANSI_RESET);
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
	int cpu_using = 0;
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
		if ((*running)->pid)
		{
			cpu_using = 1;
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
						FakeOS_procUpdateStats(os, *running, READY_ENQUEUE); 
						List_pushBack(&os->ready, (ListItem *)*running);
						printf(ANSI_ORANGE "\t\t[!] end burst -> move to ready\n" ANSI_RESET);
						break;
					case IO:
						// caso priority scheduling
						// se il processo ha completato un burst di CPU, resetta la priorità
						resetAging(*running);
						List_pushBack(&os->waiting, (ListItem *)*running);
						printf(ANSI_YELLOW "\t\t[!] end burst -> move to waiting\n" ANSI_RESET);
						break;
					}
				}
				else
				{
					FakeOS_procUpdateStats(os, *running, COMPLETE_TIME); 
					List_pushBack(&os->terminated_stats, (ListItem *)((*running)->stats)); 

					// kill process
					FakeOS_destroyPCB(*running);
					printf(ANSI_RED "\t\t[-] end process\n" ANSI_RESET);
				}
				// set running to 0 to signal that the core is free
				*running = 0;
			}
		}
	}

	// if at least one core is using the cpu, increment the cpu utilization
	if (cpu_using)
		os->cpu_busy_time++;

	/********************************* READY QUEUE *********************************/

	// scan ready list, and print the pid of the process in the ready list
	aux = os->ready.first;
	printf(ANSI_BLUE "\nREADY QUEUE:\n" ANSI_RESET);
	while (aux)
	{
		FakePCB *pcb = (FakePCB *)aux;
		ProcessEvent *e = (ProcessEvent *)pcb->events.first;
		assert(e->type == CPU);
		printf(ANSI_BLUE "\tPID: %2d - CPU_burst: %2d - Priority: %s\n" ANSI_RESET, pcb->pid, e->duration, print_priority(pcb->curr_priority));
		aux = aux->next;
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
	}

	// increase the duration of the running processes
	increaseDuration(os);
	++os->timer;
}

/**
 * @brief Calculate the statistics of the fake OS
 *
 * @param os
 */
void FakeOS_calculateStatistics(FakeOS *os) {
    int total_turnaround_time = 0;
    int total_waiting_time = 0;
    int total_response_time = 0;
    int total_processes = 0;

    ListItem *item = os->terminated_stats.first;

    // Itera su tutti i processi completati per raccogliere i dati
    while (item) {
        ProcessStats *stats = (ProcessStats *)item;
        
        // Incrementa i contatori con i dati del processo
        total_turnaround_time += stats->complete_time - stats->arrival_time;
        total_waiting_time += stats->waiting_time;
        total_response_time += stats->response_time;
        total_processes++;
        
        item = item->next;
    }

    if (total_processes == 0) {
        printf("Nessun processo completato.\n");
        return;
    }

    // Calcola le statistiche medie
    float avg_turnaround_time = (float)total_turnaround_time / total_processes;
    float avg_waiting_time = (float)total_waiting_time / total_processes;
    float avg_response_time = (float)total_response_time / total_processes;
    float cpu_utilization = (float)os->cpu_busy_time / os->timer * 100.0;
    float throughput = (float)total_processes / (float)os->timer;

    // Stampa le statistiche
    printf(ANSI_CYAN "\n\n------------------------------------STATISTICS------------------------------------\n" ANSI_RESET);
    printf(ANSI_CYAN "Turnaround time avrg: \t\t[%.3f ms]\n" ANSI_RESET, avg_turnaround_time);
    printf(ANSI_CYAN "Waiting time avrg: \t\t[%.3f ms]\n" ANSI_RESET, avg_waiting_time);
    printf(ANSI_CYAN "Response time avrg: \t\t[%.3f ms] \n" ANSI_RESET, avg_response_time);
    printf(ANSI_CYAN "Throughput: \t\t\t[%f]\n" ANSI_RESET, throughput);
    printf(ANSI_CYAN "CPU Used: \t\t\t[%.2f%%]\n" ANSI_RESET, cpu_utilization);
     
    // Fairness
    // for (int i = 0; i < MAX_PRIORITY; ++i) {
    //     printf("Tempo CPU per priorità %d: %d\n", i, os->priority_cpu_time[i]);
    // }
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
	ListItem *aux = os->terminated_stats.first;
	while (aux)
	{
		ProcessStats *stats = (ProcessStats *)aux;
		aux = aux->next;
		memset(stats, 0, sizeof(ProcessStats));
		free(stats);
		stats = 0;
	}
	os->running = 0;
	os->schedule_args = 0;
}




int main(int argc, char **argv)
{
	FakeOS os;

	if (argc < 4)
	{
		printf(usage_buffer, argv[0]);
		return 1;
	}

	srand(time(NULL));

	int num_cores = atoi(argv[1]);
	int num_processes = atoi(argv[2]);
	int scheduler = atoi(argv[3]) - 1;
	const char *histogram_file = argv[4];

	if (num_cores < 1 || num_processes < 1 || scheduler < 0 || scheduler >= MAX_SCHEDULERS)
	{
		printf(usage_buffer, argv[0]);
		return 1;
	}

	FakeOS_init(&os, num_cores, histogram_file);
	FakeOS_setScheduler(&os, scheduler);

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
	FakeOS_calculateStatistics(&os);
	FakeOS_destroy(&os);
}
