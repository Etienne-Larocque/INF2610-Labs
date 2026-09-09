#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

const int BUF_SIZE = 64;
const char outputFile[] = "section2_2.txt";

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <mots_file>\n", argv[0]);
        return 1;
    }
    int motFileFd = open(argv[1], O_RDONLY);
    char buf[BUF_SIZE];

    int motCount = 0;
    bool espace = false;
    while (read(motFileFd, buf, BUF_SIZE) != 0) {
        int i = 0;
        while (buf[i]) {
            if (isspace(buf[i])) {
                espace = true;
            } else if (espace) {
                espace = false;
                motCount++;
            }
            i++;
        }
    }
    close(motFileFd);

    printf("motCount: %d\n", motCount);
    int outputFd = open(outputFile, O_CREAT | O_WRONLY);
    char str[BUF_SIZE];
    sprintf(str, "%d", motCount);
    if (write(outputFd, str, BUF_SIZE) == -1) {
        perror("write");
        return 1;
    }
    
    close(outputFd);
    return 0;
}

