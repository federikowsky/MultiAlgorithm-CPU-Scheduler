#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../include/fake_os.h"

// void promote_process(FakeOS *os, FakePCB *pcb) {
//     // Se il processo è già nella coda con priorità massima, non promuovere
//     if (pcb->priority > 0) {
//         pcb->priority--;
//         List_detach(&os->ready[pcb->priority + 1], &pcb->list);
//         List_pushBack(&os->ready[pcb->priority], &pcb->list);
//     }
// }

// void demote_process(FakeOS *os, FakePCB *pcb) {
//     // Se il processo è già nella coda con priorità minima, non degradare
//     if (pcb->priority < NUM_QUEUES - 1) {
//         pcb->priority++;
//         List_detach(&os->ready[pcb->priority - 1], &pcb->list);
//         List_pushBack(&os->ready[pcb->priority], &pcb->list);
//     }
// }