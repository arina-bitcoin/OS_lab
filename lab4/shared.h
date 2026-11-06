#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>

#define SHM_FILE "lr4_shared.dat"
#define MAX_LINE 65536

typedef struct {
    sem_t request;       /* Родитель положил новую строку */
    sem_t response;      /* Ребёнок записал результат */
    int stop;            /* Команда завершения */
    size_t input_len;
    size_t output_len;
    uint64_t processed;
    char input[MAX_LINE];
    char output[MAX_LINE];
} SharedSlot;

typedef struct {
    SharedSlot child1;
    SharedSlot child2;
} SharedData;

#endif
