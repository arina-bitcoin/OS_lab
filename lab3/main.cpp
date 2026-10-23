#define _GNU_SOURCE
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static int hold_sec = 0;
typedef struct {
    int *a;
    size_t n;          // степень двойки
    int tid, nthreads;
    pthread_barrier_t *bar;
} ctx_t;
static void *worker(void *arg) {
    ctx_t *c = arg;
    if (hold_sec > 0) sleep(hold_sec);   
    int *a = c->a;
    size_t n = c->n;
    size_t chunk = n / c->nthreads;
    size_t lo = chunk * c->tid;
    size_t hi = (c->tid == c->nthreads - 1) ? n : lo + chunk;
    for (size_t k = 2; k <= n; k <<= 1) {
        for (size_t j = k >> 1; j > 0; j >>= 1) {
            for (size_t i = lo; i < hi; i++) {
                size_t l = i ^ j;
                if (l > i) {
                    int up = ((i & k) == 0);
                    if ((up && a[i] > a[l]) || (!up && a[i] < a[l])) {
                        int t = a[i]; a[i] = a[l]; a[l] = t;
                    }
                }
            }
            (c->bar);
        }
    }
    return NULL;
}
static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
int main(int argc, char **argv) {
    size_t n = 1 << 20;
    int T = 1;
    int opt;
    while ((opt = getopt(argc, argv, "n:t:h:")) != -1) {
        switch (opt) {
        case 'n': n = strtoull(optarg, NULL, 10); break;
        case 't': T = atoi(optarg); break;
        case 'h': hold_sec = atoi(optarg); break;
        default:
            fprintf(stderr, "Usage: %s -n size -t threads [-h pause_sec]\n", argv[0]);
            return 1;
        }
    }
    if (n == 0 || T < 1) { fprintf(stderr, "bad args\n"); return 1; }
    size_t m = 1;
    while (m < n) m <<= 1;
    if ((size_t)T > m) T = (int)m;
    int *a = malloc(m * sizeof(int));
    if (!a) { perror("malloc"); return 1; }
    srand(42);
    for (size_t i = 0; i < n; i++) a[i] = rand() - RAND_MAX / 2;
    for (size_t i = n; i < m; i++) a[i] = INT_MAX;   // дополнение до 2^k
    pthread_barrier_t bar;
    pthread_barrier_init(&bar, NULL, T);
    pthread_t *th = malloc(T * sizeof(pthread_t));
    ctx_t *cx = malloc(T * sizeof(ctx_t));
    fprintf(stderr, "PID=%d, n=%zu (padded %zu), threads=%d\n", getpid(), n, m, T);
    double t0 = now();
    for (int i = 0; i < T; i++) {
        cx[i] = (ctx_t){a, m, i, T, &bar};
        if (pthread_create(&th[i], NULL, worker, &cx[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    for (int i = 0; i < T; i++) pthread_join(th[i], NULL);
    double t1 = now();
    int ok = 1;
    for (size_t i = 1; i < n; i++)
        if (a[i - 1] > a[i]) { ok = 0; break; }
    printf("sorted=%s time=%.6f s\n", ok ? "yes" : "NO", t1 - t0);
    pthread_barrier_destroy(&bar);
    free(a); free(th); free(cx);
    return ok ? 0 : 2;
}