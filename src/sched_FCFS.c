#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"


void schedFCFS(struct FakeOS *os, void *args_)
{
    // look for the first process in ready
    // if none, return
    if (!os->ready.first)
        return;

    FakePCB *pcb = (FakePCB *)List_popFront(&os->ready);
    pcb->duration = 0;

    // put it in running list (first empty slot)
    int i = 0;
    FakePCB **running = os->running;
    while (running[i])
        ++i;
    running[i] = pcb;
};