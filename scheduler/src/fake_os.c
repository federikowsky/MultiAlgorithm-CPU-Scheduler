#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../include/fake_os.h"
#include <dirent.h>

char usage_buffer[1024] = "Usage: %s <num_cores> <scheduler> <quantum> <traces_folder> \n\
\n\
<num_cores>: Number of cores to simulate the processes on. \n\
<scheduler>: The scheduling algorithm to use: \n\
	1: First Come First Served (FCFS) \n\
	2: First Come First Served (FCFS) preemptive \n\
	3: Shortest Job First (SJF) with prediction \n\
	4: Shortest Job First (SJF) preemptive with prediction \n\
	5: Shortest Job First (SJF) no prediction \n\
	6: Shortest Remaining Time First (SRTF) (a sjf with preemptive)\n\
	7: Priority \n\
	8: Priority preemptive \n\
	9: Round Robin (RR) \n\
	10: Multi-Level Queue (MLQ) \n\
	11: Multi-Level Feedback Queue (MLFQ) \n\
<quantum>: The quantum to use for the scheduling algorithm. \n\
<traces_folder>: The path to the folder containing the traces. \n\
\n\
Example: %s 4 3 10 traces_folder \n\
\n\
This will simulate a 4 core system using the SJF with prediction scheduling algorithm and a quantum of 10 using the traces in the traces_folder. \
\n";

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
	printf("Priority: %s\n", print_priority(pcb->priority));
	printf("Duration: %d\n", pcb->duration);
	// List_print(&pcb->events, printProcessEvent);
}

void FakeOS_printReadyQueue(FakeOS *os)
{
	ListItem *aux;

	switch (os->scheduler)
	{
	case SJF_PREDICT:
	case SJF_PREDICT_PREEMPTIVE:
		SJF_printQueue(os);
		break;
	case PRIORITY:
	case PRIORITY_PREEMPTIVE:
		Prior_printQueue(os);
		break;
	case MLQ:
		MLQ_printQueue((SchedMLQArgs *)os->schedule_args);
		break;
	case MLFQ:
		MLFQ_printQueue((SchedMLFQArgs *)os->schedule_args);
		break;
	default:
		aux = os->ready.first;
		printf(ANSI_BLUE "\nREADY QUEUE:\n" ANSI_RESET);
		while (aux)
		{
			FakePCB *pcb = (FakePCB *)aux;
			ProcessEvent *e = (ProcessEvent *)pcb->events.first;
			assert(e->type == CPU);
			printf(ANSI_BLUE "\tPID: %2d - CPU_burst: %3d - Priority: %-8s\n" ANSI_RESET, 
				pcb->pid, e->duration, print_priority(pcb->priority));
			aux = aux->next;
		}
	}
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
 * @brief Check if all the cores are full
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
	if (!os->running)
		assert(0 && "malloc failed creating running array");
	List_init(&os->ready);
	List_init(&os->waiting);
	List_init(&os->processes);
	List_init(&os->terminated_stats);
	os->timer = 0;
	os->schedule_fn = 0;
	os->cores = cores;
	os->cpu_busy_time = 0;
}

/**
 * @brief Set the scheduler function and arguments
 *
 * @param os
 * @param scheduler
 */
void FakeOS_setScheduler(FakeOS *os, SchedulerType scheduler, int quantum)
{
    assert(os && "null pointer");

    void *args;  // Variabile generica per gestire i diversi tipi di args
	float aging_threshold = quantum * AGING_FACTOR;

    switch (scheduler)
    {
    case FCFS:
	case FCFS_PREEMPTIVE:
		args = FCFSArgs(quantum, scheduler);
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
		args = PriorArgs(quantum, aging_threshold, scheduler);
		os->schedule_fn = schedPriority;
		break;
	case RR:
		args = RRArgs(quantum, scheduler);
        os->schedule_fn = schedRR;
        break;
	case MLQ:
		args = MLQArgs(quantum);
		os->schedule_fn = schedMLQ;
		break;
    case MLFQ:
		args = MLFQArgs(quantum, aging_threshold);
		os->schedule_fn = schedMLFQ;
        break;

    default:
        assert(0 && "illegal scheduler");
    }

	// set the scheduler arguments and type
    os->scheduler = scheduler;
    os->schedule_args = args;
}

/**
 * @brief Create a process and put it in the list of processes to be created
 *
 * @param os The fake OS instance
 * @param proc_num The process number to create
 */
void FakeOS_createProcess(FakeOS *os, const char *proc_file)
{
    static unsigned int pid = 1;
    char line[256];

    // Read the process file
    FILE *file = fopen(proc_file, "r");
    assert(file && "file not found");

    // Allocate memory for the new process
    FakeProcess *new_process = (FakeProcess *)malloc(sizeof(FakeProcess));
    if (!new_process)
        assert(0 && "malloc failed creating process");

    new_process->pid = pid++;

    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        // Find the position of comment if present
        char *comment_pos = strchr(line, '#');
        if (comment_pos)
        {
            *comment_pos = '\0'; // Truncate the line at the comment
        }

        // Parse the line
        if (strncmp(line, "Arrival", 7) == 0)
            sscanf(line, "Arrival %d", &new_process->arrival_time);

        else if (strncmp(line, "Priority", 8) == 0)
            sscanf(line, "Priority %d", &new_process->priority);

        else if (strncmp(line, "CPU", 3) == 0 || strncmp(line, "IO", 2) == 0)
        {
            ProcessEvent *new_event = (ProcessEvent *)malloc(sizeof(ProcessEvent));
            if (!new_event)
                assert(0 && "malloc failed creating event");

            // Determina il tipo di evento e salva il valore
            if (strncmp(line, "CPU", 3) == 0)
            {
                new_event->type = CPU;
                sscanf(line, "CPU %d", &new_event->duration);
            }
            else
            {
                new_event->type = IO;
                sscanf(line, "IO %d", &new_event->duration);
            }
            
            // Inizializza i puntatori della lista
            new_event->list.next = new_event->list.prev = 0;
            List_pushBack(&new_process->events, (ListItem *)new_event);
        }
    }

    fclose(file);
    List_pushBack(&os->processes, (ListItem *)new_process);

    printf(ANSI_GREEN "\t[+] process created\n" ANSI_RESET);
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

	// all fine, no such pcb exists, we can create it
	FakePCB *new_pcb = (FakePCB *)malloc(sizeof(FakePCB));
	if (!new_pcb)
		assert(0 && "malloc failed creating pcb");
	new_pcb->list.next = new_pcb->list.prev = 0;
	new_pcb->pid = p->pid;
	new_pcb->events = p->events;
	new_pcb->priority = p->priority;
	new_pcb->duration = 0;
	new_pcb->quantum_used = 0;
	new_pcb->stats = FakeProcess_initiStats();
	FakeProcess_setArgs(new_pcb, os->scheduler);
	FakeOS_procUpdateStats(os, new_pcb, ARRIVAL_TIME); 
     
	assert(new_pcb->events.first && "process without events");

	FakeOS_enqueueProcess(os, new_pcb);
}

/**
 * @brief Destroy a PCB and free its memory
 *
 * @param pcb
 */
void FakeOS_destroyPCB(FakePCB *pcb)
{
	if (pcb->args)
		free(pcb->args);
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
	case TURNAROUND_TIME:
		FakeProcess_turnaroundTime(pcb, os->timer);
		break;
	case WAITING_TIME:
		FakeProcess_waitingTime(pcb, os->timer);
	case RESPONSE_TIME:
		FakeProcess_responseTime(pcb, os->timer);
		break;
	default:
		assert(0 && "illegal stat type");
	}
}

/**
 * @brief Enqueue a process in the ready or waiting list
 *
 * @param os
 * @param pcb
 * @param old_event_type
 */
void FakeOS_enqueueProcess(FakeOS *os, FakePCB *pcb)
{
	if (pcb->events.first)
	{
		ProcessEvent *e = (ProcessEvent *)pcb->events.first;
		switch (e->type)
		{
		case CPU:
			if (os->scheduler == MLFQ)
				MLFQ_enqueue(os, pcb);
			else if (os->scheduler == MLQ)
				MLQ_enqueue(os, pcb);
			else
				List_pushBack(&os->ready, (ListItem *)pcb);
			FakeOS_procUpdateStats(os, pcb, READY_ENQUEUE);
			printf(ANSI_ORANGE "\t\t[!] move to ready\n" ANSI_RESET);
			break;
		case IO:
			// if the process requires I/O, we put it in the waiting list and increment the priority
			List_pushBack(&os->waiting, (ListItem *)pcb);
			printf(ANSI_YELLOW "\t\t[!] move to waiting\n" ANSI_RESET);
			break;
		default:
			assert(0 && "illegal resource");
		}
	}
	else
	{
		List_pushBack(&os->terminated_stats, (ListItem *)pcb->stats);
		FakeOS_procUpdateStats(os, pcb, COMPLETE_TIME);
		FakeOS_destroyPCB(pcb);
		printf(ANSI_RED "\t\t[-] end process\n" ANSI_RESET);
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
			free(new_process);
			new_process = 0;
		}
	}

	/********************************* WAITING QUEUE *********************************/
	
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
			free(e);
			List_detach(&os->waiting, (ListItem *)pcb);

			FakeOS_enqueueProcess(os, pcb);
		}
	}

	/********************************* RUNNING QUEUE *********************************/
	
	FakePCB **running;
	int i = -1;
	int cpu_using = 0;
	printf(ANSI_GREEN "\nRUNNING QUEUE:\n" ANSI_RESET);
	while (++i < os->cores)
	{
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
				free(e);
				FakeOS_enqueueProcess(os, *running);
				
				// set running to 0 to signal that the core is free
				*running = 0;
			}
		}
	}

	// if at least one core is using the cpu, increment the cpu utilization
	if (cpu_using)
		os->cpu_busy_time++;

	/********************************* READY QUEUE *********************************/

	FakeOS_printReadyQueue(os);

	/********************************* SCHEDULING *********************************/

	i = -1;
	while (++i < os->cores)
	{
		if (!cpuFull(os->running, os->cores))
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
    while (item)
	{
        ProcessStats *stats = (ProcessStats *)item;
        
        // Incrementa i contatori con i dati del processo
        total_turnaround_time += stats->complete_time - stats->arrival_time;
        total_waiting_time += stats->waiting_time;
        total_response_time += stats->response_time;
        total_processes++;
        
        item = item->next;
    }

    if (total_processes == 0)
	{
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
	free(os->running);
	if (os->schedule_args)
	{
		switch (os->scheduler)
		{
			case MLFQ:
				MLFQ_destroyArgs(os->schedule_args);
				break;
			case MLQ:
				MLQ_destroyArgs(os->schedule_args);
				break;
			default:
				free(os->schedule_args);
				break;
		}
	}

	ListItem *aux = os->terminated_stats.first;
	while (aux)
	{
		ProcessStats *stats = (ProcessStats *)aux;
		aux = aux->next;
		free(stats);
	}
	os->running = 0;
	os->schedule_args = 0;
}




int main(int argc, char **argv)
{
	FakeOS os;

	if (argc != 5)
	{
		printf(usage_buffer, argv[0]);
		return 1;
	}

	srand(time(NULL));

	int num_cores = atoi(argv[1]);
	int scheduler = atoi(argv[2]) - 1;
	int quantum = atoi(argv[3]);
	const char *traces_folder = argv[4];

	if (num_cores < 1 || scheduler < 0 || scheduler >= MAX_SCHEDULERS)
	{
		printf(usage_buffer, argv[0]);
		return 1;
	}

	FakeOS_init(&os, num_cores);
	FakeOS_setScheduler(&os, scheduler, quantum);

	// read from traces folder the process and create them
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(traces_folder)) == NULL)
	{
		perror("Could not open directory");
		return 1;
	}

	while ((ent = readdir(dir)) != NULL)
	{
		if (ent->d_type == DT_REG)
		{
			char filename[256];
			sprintf(filename, "%s/%s", traces_folder, ent->d_name);
			printf("Reading from file: %s\n", filename);
			FakeOS_createProcess(&os, filename);	
		}
	}
	closedir(dir);

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
	free(temp);
	temp = 0;
	FakeOS_calculateStatistics(&os);
	FakeOS_destroy(&os);
	
	return 0;
}
