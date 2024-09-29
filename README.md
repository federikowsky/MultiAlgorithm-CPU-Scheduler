# CPU_Scheduling

## 1. Introduction
- CPU Scheduling is a process which allows one process to use the CPU while the execution of another process is on hold(in waiting state) due to unavailability of any resource like I/O etc, thereby making full use of CPU. The aim of CPU scheduling is to make the system efficient, fast and fair.

## 2. Types of CPU Scheduling
- **Non-Preemptive Scheduling**: Once a process has been allocated the CPU, it cannot be preempted until it completes its CPU burst.
- **Preemptive Scheduling**: In this type of Scheduling, the tasks are usually assigned with priorities. At times it is necessary to run a certain task that has a higher priority before another task although it is running. Therefore, the running task is interrupted for some time and resumed later when the priority task has finished its execution.

## 3. Scheduling Algorithms
- **First Come First Serve(FCFS)**: In this type of Scheduling, the process that arrives first, gets executed first. It is simple and easy to understand.
- **Shortest Job Next(SJN)**: This is the process of scheduling such that the process with the smallest execution time is chosen for the next execution.
- **Priority Scheduling**: In this type of Scheduling, the process with the highest priority is selected for execution.
- **Round Robin(RR)**: Each process is assigned a fixed time in cyclic order.
- **Multilevel Queue Scheduling**: In this type of Scheduling, the ready queue is partitioned into several separate queues.
- **Multilevel Feedback Queue Scheduling**: In this type of Scheduling, a process can move between the various queues.

## 4. Conclusion
- CPU Scheduling is an essential part of a Multiprogramming operating systems. It allows the operating system to maximize the utilization of CPU. It is an essential operating system concept that helps in the execution of processes in an efficient manner.
