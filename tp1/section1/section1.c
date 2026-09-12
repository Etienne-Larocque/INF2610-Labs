#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static const size_t BUF_SIZE = 64;
static const char OUTPUT_FILE[] = "section2_2.txt";
static const mode_t OUTPUT_MODE =
    S_IRUSR | S_IWUSR |
    S_IRGRP |
    S_IROTH;

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

int count_words_or_exit(char *file_path) {
    int input_fd = open_or_exit(file_path, O_RDONLY, 0);

    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    int word_count = 0;
    int is_inside_word = 0;

    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0)
    {
        for (ssize_t i = 0; i < bytes_read; i++) {
            unsigned char character = (unsigned char)buffer[i];
            if (isspace(character))
            {
                is_inside_word = 0;
            }
            else if (!is_inside_word)
            {
                is_inside_word = 1;
                word_count++;
            }
        }
    }
    if (bytes_read == -1)
    {
        perror("read input file");
        close(input_fd);
        exit(EXIT_FAILURE);
    }

    close_or_exit(input_fd, "output file");

    printf("motCount: %d\n", word_count);
    return word_count;
}


int main(int argc, char *argv[]) 
{
    if (argc != 2)
    {
        printf("Usage: %s <mots_file>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    int word_count = count_words_or_exit(argv[1]); 

    int output_fd = open_or_exit(
        OUTPUT_FILE,
        O_CREAT | O_WRONLY | O_TRUNC,
        OUTPUT_MODE
    );

    char output[BUF_SIZE];
    int output_length = snprintf(
        output,
        sizeof(output),
        "%d\n",
        word_count
    );
    
    write_all_or_exit(
        output_fd,
        output,
        (size_t)output_length,
        OUTPUT_FILE
    );

    close_or_exit(output_fd, "close output file");

    return EXIT_SUCCESS;
}


