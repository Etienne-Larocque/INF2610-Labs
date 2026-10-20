#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "timer.h"

#define SEQUENTIEL 0

#define BUFFER_SIZE 4096
#define TAIL_SIZE 11 // strlen("FAILED_LOGIN") - 1
static const char OUTPUT_FILE[] = "RESULT_PROCESS.txt";

typedef struct
{
    const char *log1;
    const char *log2;
    int n;
} process_args_t;

typedef struct
{
    int critical;
    int error;
    int failed_login;
} count_result_t;

static pid_t fork_or_exit() {
    pid_t child_pid = fork();
    if (child_pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return child_pid;
}

int count_word(const char *buf, const char *word)
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

void count_in_buffer(const char *buf, int *crit, int *err, int *fail)
{
    *crit += count_word(buf, "CRITICAL");
    *err += count_word(buf, "ERROR");
    *fail += count_word(buf, "FAILED_LOGIN");
}

// Le buffer contient [tail][données lues][\0]. On compte sur tout le buffer,
// puis on retire ce qui est entièrement dans le tail (déjà compté avant) :
// un mot appartient au bloc / à la lecture où il se termine.
void count_keywords_block(const char *filename, off_t start, off_t end,
                           const char *prev_tail, int prev_len,
                           int *crit, int *err, int *fail)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1) { perror(filename); return; }

    if (lseek(fd, start, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return;
    }

    char buffer[BUFFER_SIZE];
    int tail_len = prev_len;
    for (int i = 0; i < tail_len; i++) {
        buffer[i] = prev_tail[i];
    }

    off_t remaining = end - start;

    while (remaining > 0) {
        size_t space = BUFFER_SIZE - 1 - tail_len;
        size_t to_read = remaining < (off_t)space ? (size_t)remaining : space;

        ssize_t bytes_read = read(fd, buffer + tail_len, to_read);

        if (bytes_read == -1) {
            perror("read");
            close(fd);
            return;
        }
        if (bytes_read == 0) { break; }

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

    close(fd);
}

void process_file(const char *filename, int n, const int result_fd)
{
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) { perror(filename); _exit(EXIT_FAILURE); }

    off_t size = lseek(file_fd, 0, SEEK_END);

    for (int i = 0; i < n; i++)
    {
        off_t start = i * size / n;
        off_t end = (i + 1) * size / n;

        char prev_tail[TAIL_SIZE];
        int prev_len = start < TAIL_SIZE ? (int)start : TAIL_SIZE;
        lseek(file_fd, start - prev_len, SEEK_SET);
        if (read(file_fd, prev_tail, prev_len) != prev_len) { prev_len = 0; }

        pid_t pid = fork_or_exit();
        if (pid == 0)
        {
            close(file_fd);
            count_result_t result = {0};
            count_keywords_block(filename, start, end, prev_tail, prev_len,
                                 &result.critical, &result.error,
                                 &result.failed_login);
            write(result_fd, &result, sizeof(result));
            _exit(EXIT_SUCCESS);
        }
        #if SEQUENTIEL == 1
            wait(NULL);
        #endif
    }

    close(file_fd);

    #if SEQUENTIEL != 1
        for (int i = 0; i < n; i++)
        {
            wait(NULL);
        }
    #endif
}


void process_task_wrapper(void *arg_ptr)
{
    process_args_t *arg = arg_ptr;

    int result_pipe[2];
    if (pipe(result_pipe) == -1) { perror("pipe"); exit(EXIT_FAILURE); }

    pid_t process_log1 = fork_or_exit();
    if (process_log1 == 0) {
        close(result_pipe[0]);
        process_file(arg->log1, arg->n, result_pipe[1]);
        close(result_pipe[1]);
        _exit(EXIT_SUCCESS);
    }
    #if SEQUENTIEL == 1
        wait(NULL);
    #endif

    pid_t process_log2 = fork_or_exit();
    if (process_log2 == 0) {
        close(result_pipe[0]);
        process_file(arg->log2, arg->n, result_pipe[1]);
        close(result_pipe[1]);
        _exit(EXIT_SUCCESS);
    }
    #if SEQUENTIEL == 1
        wait(NULL);
    #endif

    close(result_pipe[1]);

    count_result_t total = {0};
    count_result_t result;

    for (;;)
    {
        ssize_t status = read(result_pipe[0], &result, sizeof(result));

        if (status == 0) { break; }
        if (status != sizeof(result))
        {
            perror("read");
            break; // Le reste peut être corrompu
        }

        total.critical += result.critical;
        total.error += result.error;
        total.failed_login += result.failed_login;
    }

    close(result_pipe[0]);

    #if SEQUENTIEL != 1
        if (waitpid(process_log1, NULL, 0) == -1) { perror("waitpid log1"); }
        if (waitpid(process_log2, NULL, 0) == -1) { perror("waitpid log2"); }
    #endif

    int output_fd = open(
        OUTPUT_FILE,
        O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    );

    if (output_fd == -1) { perror("open"); exit(EXIT_FAILURE); }

    char output[64];
    int output_length = snprintf(
        output,
        sizeof(output),
        "CRITICAL: %d\n"
        "ERROR: %d\n"
        "FAILED LOGIN: %d\n",
        total.critical,
        total.error,
        total.failed_login
    );
    
    write(
        output_fd,
        output,
        (size_t)output_length
    );

    close(output_fd);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <log1> <log2> <N>\n", argv[0]);
        return 1;
    }

    process_args_t args;
    args.log1 = argv[1];
    args.log2 = argv[2];
    args.n = atoi(argv[3]);

    printf("Logs 1: %s\n", args.log1);
    printf("Logs 2: %s\n", args.log2);
    printf("n: %d\n", args.n);
    fflush(stdout); 

    system("mkdir -p tmp bin");

    double secondes = measure_time(process_task_wrapper, &args);
    printf("Temps: %.4f s\n", secondes);

    return 0;
}
