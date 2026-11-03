#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "../utils/timer.h"

#define BUFFER_SIZE 4096
#define TAIL_SIZE 11 // strlen("FAILED_LOGIN") - 1
static const char OUTPUT_FILE[] = "RESULT_THR_POOL.txt";

typedef struct {
    int critical;
    int error;
    int failed;
} Result;

typedef struct {
    int fd;
    int thread_id;
    int n_blocks;
    size_t file_size;
    atomic_int* present_n;
    Result res;
} PoolArgs;


int count_word(const char* buf, const char* word)
{
    int count = 0;
    for (int i = 0; buf[i] != '\0'; i++) {
        int j = 0;
        while (word[j] != '\0' && buf[i + j] == word[j]) {
            j++;
        }
        if (word[j] == '\0') {
            count++;
        }
    }
    return count;
}

void count_in_buffer(const char* buf, Result* res)
{
    res->critical += count_word(buf, "CRITICAL");
    res->error += count_word(buf, "ERROR");
    res->failed += count_word(buf, "FAILED_LOGIN");
}

// Même logique qu'en section 1 : le buffer contient [tail][données lues][\0].
// On compte sur tout le buffer, puis on retire ce qui est entièrement dans le tail
// (déjà compté par le bloc ou la lecture d'avant).
void count_keywords_block(int fd, off_t start, off_t end, Result* res)
{
    char buffer[BUFFER_SIZE];
    int tail_len = start < TAIL_SIZE ? start : TAIL_SIZE;
    if (pread(fd, buffer, tail_len, start - tail_len) != tail_len) {
        tail_len = 0;
    }

    off_t remaining = end - start;
    off_t file_offset = start;

    while (remaining > 0) {
        size_t space = BUFFER_SIZE - 1 - tail_len;
        size_t to_read = remaining < (off_t)space ? (size_t)remaining : space;

        ssize_t bytes_read = pread(fd, buffer + tail_len, to_read, file_offset);
        if (bytes_read == -1) { perror("couldnt read right"); exit(EXIT_FAILURE); }
        if (bytes_read == 0) { break; }
        file_offset += bytes_read;

        int total = tail_len + (int)bytes_read;
        buffer[total] = '\0';
        count_in_buffer(buffer, res);

        char tail[TAIL_SIZE + 1];
        for (int i = 0; i < tail_len; i++) {
            tail[i] = buffer[i];
        }
        tail[tail_len] = '\0';
        Result in_tail = { 0 };
        count_in_buffer(tail, &in_tail);
        res->critical -= in_tail.critical;
        res->error -= in_tail.error;
        res->failed -= in_tail.failed;

        // Les derniers octets deviennent le tail de la prochaine lecture
        tail_len = total < TAIL_SIZE ? total : TAIL_SIZE;
        for (int i = 0; i < tail_len; i++) {
            buffer[i] = buffer[total - tail_len + i];
        }

        remaining -= bytes_read;
    }
}

void* pool_worker(void* arg) {
    PoolArgs* a = (PoolArgs*)arg;

    while (1) {
        int task_n = atomic_fetch_add(a->present_n, 1);
        if (task_n >= a->n_blocks) break;

        //  ----- The task -----
        off_t start = task_n * a->file_size / a->n_blocks;
        off_t end = (task_n + 1) * a->file_size / a->n_blocks;

        printf("  thread %d -> bloc %d\n", a->thread_id, task_n);
        count_keywords_block(a->fd, start, end, &a->res);
    }

    return NULL;
}

void process_file_pool(const char* filename, int n_blocks, int nthreads, Result* r_total) {

    int fd = open(filename, O_RDONLY);
    if (fd == -1) { perror(filename); return; }

    off_t file_size = lseek(fd, 0, SEEK_END);
    printf("%s : %d blocs, %d threads\n", filename, n_blocks, nthreads);

    pthread_t th[nthreads];
    PoolArgs* pool_args = malloc(nthreads * sizeof(PoolArgs));
    atomic_int present_n = 0;

    for (int i = 0; i < nthreads; i++) {
        pool_args[i] = (PoolArgs)
        {
            .fd = fd,
            .thread_id = i,
            .n_blocks = n_blocks,
            .file_size = file_size,
            .present_n = &present_n,
            .res = {0}
        };

        if (pthread_create(&th[i], NULL, &pool_worker, &pool_args[i]) != 0) {
            perror("failed to pthread_create");
        }
    }
    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("failed to pthread_join");
        }
        r_total->critical += pool_args[i].res.critical;
        r_total->error += pool_args[i].res.error;
        r_total->failed += pool_args[i].res.failed;
    }
    free(pool_args);

    close(fd);
}

typedef struct {
    const char* log1;
    const char* log2;
    int n_blocks;
    int nthreads;
} pool_args_t;

void pool_task_wrapper(void* arg) {
    pool_args_t* args = (pool_args_t*)arg;

    Result total = { 0 };

    process_file_pool(args->log1, args->n_blocks, args->nthreads, &total);
    process_file_pool(args->log2, args->n_blocks, args->nthreads, &total);

    int output_fd = open(
        OUTPUT_FILE,
        O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    );

    if (output_fd == -1) { perror("cant open exit file"); exit(EXIT_FAILURE); }

    char output[128];
    int output_length = snprintf(
        output,
        sizeof(output),
        "---------- resultats ----------\n"
        "   critical       : %d\n"
        "   error          : %d\n"
        "   failed login   : %d\n",
        total.critical,
        total.error,
        total.failed
    );

    write(
        output_fd,
        output,
        (size_t)output_length
    );

    close(output_fd);
}

int main(int argc, char* argv[]) {
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
