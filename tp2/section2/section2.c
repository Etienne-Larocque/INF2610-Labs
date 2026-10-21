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
    int thread_id;
    int nthreads;
    int n_blocks;
    off_t block_size;
    off_t filesize;
    Result total;
} PoolArgs;

/* TODO */
void *pool_worker(void *arg) {
    PoolArgs *a = (PoolArgs *)arg;
    (void)a;
    return NULL;
}

/* TODO  */
void process_file_pool(const char *filename, int n_blocks, int nthreads,
                        int *crit_total, int *err_total, int *fail_total) {
    (void)filename; (void)n_blocks; (void)nthreads;
    (void)crit_total; (void)err_total; (void)fail_total;
}

typedef struct {
    const char *log1;
    const char *log2;
    int n_blocks;
    int nthreads;
} pool_args_t;

/* TODO */
void pool_task_wrapper(void *arg) {
    (void)arg;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <log1> <log2> <N_BLOCS> <N_THREADS>\n", argv[0]);
        return 1;
    }

    pool_args_t args;
    args.log1 = argv[1];
    args.log2 = argv[2];
    args.n_blocks = atoi(argv[3]);
    args.nthreads = atoi(argv[4]);

    if (args.nthreads < 1) {
        fprintf(stderr, "N_THREADS doit être >= 1\n");
        return 1;
    }
    if (args.n_blocks < 1) {
        fprintf(stderr, "N_BLOCS doit être >= 1\n");
        return 1;
    }

    system("mkdir -p bin");

    double elapsed = measure_time(pool_task_wrapper, &args);
    printf("Temps d'exécution (pool de %d threads, %d blocs) = %.6f secondes\n",
           args.nthreads, args.n_blocks, elapsed);

    return 0;
}
