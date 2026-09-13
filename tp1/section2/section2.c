#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096
static const char OUTPUT_FILE[] = "RESULTAT_PROCESS.txt";
static const mode_t OUTPUT_MODE =
    S_IRUSR | S_IWUSR |
    S_IRGRP |
    S_IROTH;

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

static void pipe_or_exit(int pipe_fds[2]) {
    if (pipe(pipe_fds) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}

static pid_t fork_or_exit() {
    pid_t child_pid = fork();
    if (child_pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return child_pid;
}

static int open_or_exit(
    const char *path,
    int flags,
    mode_t mode
)
{
    int fd = open(path, flags, mode);

    if (fd == -1)
    {
        perror(path);
        exit(EXIT_FAILURE);
    }

    return fd;
}

static int read_result(
    int fd,
    count_result_t *result
)
{
    char *bytes = (char *)result;
    size_t total_read = 0;

    while (total_read < sizeof(*result))
    {
        ssize_t bytes_read = read(
            fd,
            bytes + total_read,
            sizeof(*result) - total_read
        );

        if (bytes_read == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }

        if (bytes_read == 0)
        {
            if (total_read == 0)
            {
                return 0;
            }
            return -1;
        }

        total_read += (size_t)bytes_read;
    }
    return 1;
}

static void close_or_exit(int fd, const char *description)
{
    if (close(fd) == -1)
    {
        perror(description);
        exit(EXIT_FAILURE);
    }
}

static void write_all_or_exit(
    int fd,
    const void *buffer,
    size_t length,
    const char *description
)
{
    const char *bytes = buffer;
    size_t total_written = 0;

    while (total_written < length)
    {
        ssize_t bytes_written = write(
            fd,
            bytes + total_written,
            length - total_written
        );

        if (bytes_written == -1)
        {
            perror(description);
            exit(EXIT_FAILURE);
        }

        total_written += (size_t)bytes_written;
    }
}

/* TODO */
void count_in_buffer(const char *buf, int *crit, int *err, int *fail)
{
    (void)buf; (void)crit; (void)err; (void)fail;
}

/* TODO */
void count_keywords_block(const char *filename, off_t start, off_t end,
                           const char *prev_tail, int prev_len,
                           int *crit, int *err, int *fail)
{
    (void)filename; (void)start; (void)end;
    (void)prev_tail; (void)prev_len; (void)crit; (void)err; (void)fail;
}

/* TODO */
void process_file(const char *filename, int n, const int result_fd)
{
    (void)filename; (void)n; (void)result_fd;
}

/* TODO */
void process_task_wrapper(process_args_t *arg)
{
    int result_pipe[2];
    pipe_or_exit(result_pipe);

    pid_t process_log1 = fork_or_exit();
    if (process_log1 == 0) {
        close_or_exit(result_pipe[0], "log1 read end");
        process_file(arg->log1, arg->n, result_pipe[1]);
        close_or_exit(result_pipe[1], "log2 write end");
        _exit(EXIT_SUCCESS);
    }

    pid_t process_log2 = fork_or_exit();
    if (process_log2 == 0) {
        close_or_exit(result_pipe[0], "log1 read end");
        process_file(arg->log2, arg->n, result_pipe[1]);
        close_or_exit(result_pipe[1], "log2 write end");
        _exit(EXIT_SUCCESS);
    }

    close_or_exit(result_pipe[1], "parent write end");

    count_result_t total = {0};
    count_result_t result;

    for (;;)
    {
        int status = read_result(result_pipe[0], &result);

        if (status == 0)
        {
            break;
        }

        if (status == -1)
        {
            fprintf(stderr, "Could not read a complete result\n");
            break; // We stop reading cause the rest could be corrupted
        }

        total.critical += result.critical;
        total.error += result.error;
        total.failed_login += result.failed_login;
    }

    close_or_exit(result_pipe[0], "parent read end");

    if (waitpid(process_log1, NULL, 0) == -1)
    {
        perror("waitpid log1");
    }

    if (waitpid(process_log2, NULL, 0) == -1)
    {
        perror("waitpid log2");
    }

    int output_fd = open_or_exit(
        OUTPUT_FILE,
        O_CREAT | O_WRONLY | O_TRUNC,
        OUTPUT_MODE
    );

    char output[BUF_SIZE];
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
    
    write_all_or_exit(
        output_fd,
        output,
        (size_t)output_length,
        OUTPUT_FILE
    );

    close_or_exit(output_fd, "close output file");
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

    process_task_wrapper(&args);

    return 0;
}
