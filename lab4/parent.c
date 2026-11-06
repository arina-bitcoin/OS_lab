#define _POSIX_C_SOURCE 200809L
#include "shared.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void fatal(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static int wait_sem(sem_t *sem)
{
    while (sem_wait(sem) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return 0;
}

static int stop_child(SharedSlot *slot)
{
    slot->stop = 1;
    return sem_post(&slot->request);
}

static int wait_child(pid_t pid, int *status)
{
    while (waitpid(pid, status, 0) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return 0;
}

int main(void)
{
    int fd = open(SHM_FILE, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd == -1)
        fatal("open shared file");

    if (ftruncate(fd, (off_t)sizeof(SharedData)) == -1)
        fatal("ftruncate");

    SharedData *shared = mmap(NULL, sizeof(*shared),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED)
        fatal("mmap parent");

    memset(shared, 0, sizeof(*shared));

    /* pshared=1: семафоры разделяются между процессами. */
    if (sem_init(&shared->child1.request, 1, 0) == -1)
        fatal("sem_init child1 request");
    if (sem_init(&shared->child1.response, 1, 0) == -1)
        fatal("sem_init child1 response");
    if (sem_init(&shared->child2.request, 1, 0) == -1)
        fatal("sem_init child2 request");
    if (sem_init(&shared->child2.response, 1, 0) == -1)
        fatal("sem_init child2 response");

    pid_t pid1 = fork();
    if (pid1 == -1)
        fatal("fork child1");

    if (pid1 == 0) {
        munmap(shared, sizeof(*shared));
        close(fd);
        execl("./child1", "child1", SHM_FILE, (char *)NULL);
        perror("exec child1");
        _exit(127);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        stop_child(&shared->child1);
        wait_child(pid1, NULL);
        fatal("fork child2");
    }

    if (pid2 == 0) {
        munmap(shared, sizeof(*shared));
        close(fd);
        execl("./child2", "child2", SHM_FILE, (char *)NULL);
        perror("exec child2");
        _exit(127);
    }

    fprintf(stderr, "[parent PID=%ld] child1 PID=%ld, child2 PID=%ld\n",
            (long)getpid(), (long)pid1, (long)pid2);

    srand((unsigned int)(time(NULL) ^ (unsigned int)getpid()));
    printf("Введите строки (максимум %d байт). Ctrl+D — завершение.\n",
           MAX_LINE - 1);

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    uint64_t sent1 = 0;
    uint64_t sent2 = 0;
    int input_error = 0;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        if ((size_t)length >= MAX_LINE) {
            fprintf(stderr, "Строка слишком длинная (максимум %d байт), пропущена.\n",
                    MAX_LINE - 1);
            continue;
        }

        int target = (rand() % 100 < 80) ? 1 : 2;
        SharedSlot *slot = (target == 1) ? &shared->child1 : &shared->child2;

        memcpy(slot->input, line, (size_t)length);
        slot->input[length] = '\0';
        slot->input_len = (size_t)length;

        if (sem_post(&slot->request) == -1) {
            perror("parent: sem_post(request)");
            input_error = 1;
            break;
        }

        if (wait_sem(&slot->response) == -1) {
            perror("parent: sem_wait(response)");
            input_error = 1;
            break;
        }

        printf("[parent PID=%ld] результат child%d: ",
               (long)getpid(), target);
        if (slot->output_len > 0)
            fwrite(slot->output, 1, slot->output_len, stdout);
        if (slot->output_len == 0 ||
            slot->output[slot->output_len - 1] != '\n')
            putchar('\n');

        if (target == 1)
            ++sent1;
        else
            ++sent2;
    }

    if (ferror(stdin)) {
        perror("getline");
        input_error = 1;
    }
    free(line);

    if (stop_child(&shared->child1) == -1)
        perror("stop child1");
    if (stop_child(&shared->child2) == -1)
        perror("stop child2");

    int status1 = 0;
    int status2 = 0;
    if (wait_child(pid1, &status1) == -1)
        perror("waitpid child1");
    if (wait_child(pid2, &status2) == -1)
        perror("waitpid child2");

    printf("\nИтоговая статистика:\n");
    printf("child1: направлено=%llu, обработано=%llu\n",
           (unsigned long long)sent1,
           (unsigned long long)shared->child1.processed);
    printf("child2: направлено=%llu, обработано=%llu\n",
           (unsigned long long)sent2,
           (unsigned long long)shared->child2.processed);

    sem_destroy(&shared->child1.request);
    sem_destroy(&shared->child1.response);
    sem_destroy(&shared->child2.request);
    sem_destroy(&shared->child2.response);

    if (munmap(shared, sizeof(*shared)) == -1)
        perror("parent: munmap");
    if (close(fd) == -1)
        perror("parent: close");
    if (unlink(SHM_FILE) == -1)
        perror("parent: unlink");

    int children_ok =
        WIFEXITED(status1) && WEXITSTATUS(status1) == 0 &&
        WIFEXITED(status2) && WEXITSTATUS(status2) == 0;

    return (!input_error && children_ok) ? EXIT_SUCCESS : EXIT_FAILURE;
}
