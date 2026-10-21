#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include "../utils/timer.h"

#define BUFFER_SIZE 4096
#define TAIL_SIZE 11 // strlen("FAILED_LOGIN") - 1
static const char OUTPUT_FILE[] = "RESULT_PROCESS.txt";

typedef struct {
    int critical;
    int error;
    int failed;
} Result;

typedef struct {
    const char *filename;
    off_t start;
    off_t end;
} ThreadArgs;

typedef struct {
    const char *log1;
    const char *log2;
    int n;
} thread_args_t;

int count_word(const char *buf, const char *word)
{
    int count = 0;

    for (int i = 0; buf[i] != '\0'; i++) {
        int j = 0;
        while (word[j] != '\0' && buf[i + j] != '\0' && buf[i + j] == word[j]) {
            j++;
        }
        if (word[j] == '\0') {
            count++;
        }
    }

    return count;
}

void count_in_buffer(const char *buf, int *crit, int *err, int *fail)
{
    *crit += count_word(buf, "CRITICAL");
    *err += count_word(buf, "ERROR");
    *fail += count_word(buf, "FAILED_LOGIN");
}

void count_keywords_block(const int fd, off_t start, off_t end,
                           int *crit, int *err, int *fail)
{
    char buffer[BUFFER_SIZE];
    int tail_len = start < TAIL_SIZE ? start : TAIL_SIZE;
    ssize_t bytes_read = pread(fd, buffer, tail_len, start - tail_len);

    off_t remaining = end - start;
    off_t file_offset = start;

    while (remaining > 0) {
        size_t space = BUFFER_SIZE - 1 - tail_len;
        size_t to_read = remaining < (off_t)space ? (size_t)remaining : space;

        ssize_t bytes_read = pread(fd, buffer + tail_len, to_read, file_offset);
        if (bytes_read == -1) {
            perror("read");
            return;
        }
        if (bytes_read == 0) { break; }
        file_offset += bytes_read;

        int total = tail_len + (int)bytes_read;
        buffer[total] = '\0';
        count_in_buffer(buffer, crit, err, fail);

        // Retirer les mots entièrement dans le tail (car sont deja compté par le block ou le buffer d'avant)
        char tail[TAIL_SIZE + 1];
        for (int i = 0; i < tail_len; i++) {
            tail[i] = buffer[i];
        }
        tail[tail_len] = '\0';
        int tail_crit = 0, tail_err = 0, tail_fail = 0;
        count_in_buffer(tail, &tail_crit, &tail_err, &tail_fail);
        *crit -= tail_crit;
        *err -= tail_err;
        *fail -= tail_fail;

        // Les derniers octets deviennent le tail de la prochaine lecture
        tail_len = total < TAIL_SIZE ? total : TAIL_SIZE;
        for (int i = 0; i < tail_len; i++) {
            buffer[i] = buffer[total - tail_len + i];
        }

        remaining -= bytes_read;
    }
}

/* TODO*/
void *thread_count(void *arg) {
    ThreadArgs *targ = (ThreadArgs *)arg;
    Result* res = calloc(1, sizeof(Result));
    if (!res) {
        perror("malloc result");
        free(targ);
        return NULL;
    }

    int fd = open(targ->filename, O_RDONLY);
    if (fd == -1) {
        perror(targ->filename);
        free(targ);
        return res; // Renvoie 0 partout en cas d'échec d'ouverture
    }

    count_keywords_block(fd, targ->start, targ->end,
                         &res->critical, &res->error, &res->failed);

    close(fd);
    free(targ);
    return (void *)res;
}

/* TODO*/
void process_file_threads(const char *filename, int n, pthread_t *threads, size_t *total_threads) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) { perror(filename); _exit(EXIT_FAILURE); }
    off_t size = lseek(file_fd, 0, SEEK_END);
    close(file_fd);
    int j;
    for (j = 0; j < n; j++) {
        off_t start = j * size / n;
        off_t end = (j + 1) * size / n;

        ThreadArgs *targs = malloc(sizeof(ThreadArgs));
        if (!targs) { perror("malloc targs"); _exit(EXIT_FAILURE); }
        targs->filename = filename;
        targs->start = start;
        targs->end = end;

        int tcode = pthread_create(&threads[*total_threads], NULL, thread_count, targs);
        if (tcode != 0) {
            perror("pthread_create");
            free(targs);
        } else {
            (*total_threads)++;
        }
    }
}

/* TODO */
void thread_task_wrapper(void *arg) {
    thread_args_t *targ = (thread_args_t *)arg;
    pthread_t *threads = malloc(targ->n * 2 * sizeof(pthread_t));
    if (!threads) { perror("malloc threads"); return; }

    size_t total_threads = 0;

    process_file_threads(targ->log1, targ->n, threads, &total_threads);
    process_file_threads(targ->log2, targ->n, threads, &total_threads);

    Result total = {0};
    for (int j = 0; j < total_threads; j++) {
        Result *result_thread;
        pthread_join(threads[j], (void **)&result_thread);
        if (result_thread != NULL) {
            total.critical += result_thread->critical;
            total.error += result_thread->error;
            total.failed += result_thread->failed;
            free(result_thread);
        }
    }
    free(threads);

    int output_fd = open(
        OUTPUT_FILE,
        O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    );
    if (output_fd == -1) { perror("open"); return; }
    char output[64];
    int output_length = snprintf(
        output,
        sizeof(output),
        "CRITICAL: %d\n"
        "ERROR: %d\n"
        "FAILED LOGIN: %d\n",
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
