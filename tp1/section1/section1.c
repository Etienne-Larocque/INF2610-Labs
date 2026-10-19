#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

void count_words(const char *input_path, const char *output_path)
{
    int fd = open(input_path, O_RDONLY);
    if (fd == -1) { perror("open input"); return; }

    char buffer[64];
    ssize_t n;
    int count = 0, in_word = 0;

    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
    {
        for (ssize_t i = 0; i < n; i++)
        {
            if (isspace((unsigned char)buffer[i]))
                in_word = 0;
            else if (!in_word)
            {
                in_word = 1;
                count++;
            }
        }
    }
    close(fd);

    fd = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) { perror("open output"); return; }

    char output[32];
    int len = snprintf(output, sizeof(output), "%d\n", count);
    write(fd, output, len);
    close(fd);

    printf("motCount: %d\n", count);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <mots_file>\n", argv[0]);
        return 1;
    }

    count_words(argv[1], "section2_2.txt");
    return 0;
}
