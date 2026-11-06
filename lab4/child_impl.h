#ifndef CHILD_IMPL_H
#define CHILD_IMPL_H

#define _POSIX_C_SOURCE 200809L
#include "shared.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int wait_sem(sem_t *sem)
{
    while (sem_wait(sem) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return 0;
}

static int reverse_utf8(const char *src, size_t len,
                        char *dst, size_t *dst_len)
{
    size_t end = len;

    if (end > 0 && src[end - 1] == '\n')
        --end;
    if (end > 0 && src[end - 1] == '\r')
        --end;

    size_t *starts = malloc((end + 1) * sizeof(*starts));
    if (starts == NULL)
        return -1;

    size_t count = 0;
    for (size_t i = 0; i < end; ) {
        starts[count++] = i;

        unsigned char c = (unsigned char)src[i];
        size_t step;

        if (c < 0x80)       step = 1;
        else if ((c & 0xE0) == 0xC0) step = 2;
        else if ((c & 0xF0) == 0xE0) step = 3;
        else if ((c & 0xF8) == 0xF0) step = 4;
        else                step = 1;

        if (i + step > end)
            step = 1;

        i += step;
    }

    size_t out = 0;
    for (size_t j = count; j > 0; --j) {
        size_t begin = starts[j - 1];
        size_t finish = (j == count) ? end : starts[j];
        size_t bytes = finish - begin;

        memcpy(dst + out, src + begin, bytes);
        out += bytes;
    }

    if (len > end) {
        memcpy(dst + out, src + end, len - end);
        out += len - end;
    }

    dst[out] = '\0';
    *dst_len = out;
    free(starts);
    return 0;
}

static int child_process(int number, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <shared-file>\n", argv[0]);
        return 2;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("child: open shared file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("child: fstat");
        close(fd);
        return 1;
    }

    if ((size_t)st.st_size < sizeof(SharedData)) {
        fprintf(stderr, "child%d: размер общего файла меньше ожидаемого\n", number);
        close(fd);
        return 1;
    }

    SharedData *shared = mmap(NULL, sizeof(*shared),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("child: mmap");
        close(fd);
        return 1;
    }

    SharedSlot *slot = (number == 1) ? &shared->child1 : &shared->child2;

    fprintf(stderr, "[child%d PID=%ld] запущен\n",
            number, (long)getpid());

    for (;;) {
        if (wait_sem(&slot->request) == -1) {
            perror("child: sem_wait(request)");
            break;
        }

        if (slot->stop)
            break;

        if (reverse_utf8(slot->input, slot->input_len,
                         slot->output, &slot->output_len) == -1) {
            perror("child: reverse_utf8");
            slot->output[0] = '\0';
            slot->output_len = 0;
        } else {
            ++slot->processed;
        }

        if (sem_post(&slot->response) == -1) {
            perror("child: sem_post(response)");
            break;
        }
    }

    fprintf(stderr, "[child%d PID=%ld] завершён; обработано=%llu\n",
            number, (long)getpid(),
            (unsigned long long)slot->processed);

    if (munmap(shared, sizeof(*shared)) == -1)
        perror("child: munmap");
    if (close(fd) == -1)
        perror("child: close");

    return 0;
}

#endif
