#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "../utils/timer.h"

#define BUFFER_SIZE 4096

typedef struct {
    int critical;
    int error;
    int failed;
} Result;

typedef struct {
    int fd;
    off_t start;
    off_t end;
    Result *res;
} ThreadArgs;

typedef struct {
    const char *log1;
    const char *log2;
    int n;
} thread_args_t;

/* TODO*/
void *thread_count(void *arg) {
    ThreadArgs *targ = (ThreadArgs *)arg;
    (void)targ;
    return NULL;
}

/* TODO*/
void process_file_threads(const char *filename, int n, int *crit_total, int *err_total, int *fail_total) {
    (void)filename; (void)n; (void)crit_total; (void)err_total; (void)fail_total;
}

/* TODO */
void thread_task_wrapper(void *arg) {
    (void)arg;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <log1> <log2> <N>\n", argv[0]);
        return 1;
    }

    thread_args_t args;
    args.log1 = argv[1];
    args.log2 = argv[2];
    args.n = atoi(argv[3]);

    double elapsed = measure_time(thread_task_wrapper, &args);
    printf("Temps d'exécution (threads) = %.6f secondes\n", elapsed);

    return 0;
}
