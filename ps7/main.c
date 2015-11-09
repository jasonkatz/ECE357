#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

int N_PROC = 64;

int tas(volatile char *lock);

int main() {

    int fd;
    if ((fd = open("testfile.txt", O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE) < 0)) {
        fprintf(stderr, "Couldn't open testfile.txt: %s\n", strerror(errno));
        exit(1);
    }
    char * buf = "some text";
    if (write(fd, buf, 10) < 0) {
        fprintf(stderr, "Couldn't write to testfile.txt: %s\n", strerror(errno));
        exit(1);
    }
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        fprintf(stderr, "Couldn't get stats of file: %s\n", strerror(errno));
        exit(1);
    }

    int * map;
    if ((map = (int *)mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) < 0)) {
        fprintf(stderr, "Couldn't map to file testfile.txt: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }

    map[0] = 0;

    int i;
    for (i = 0; i < N_PROC; ++i) {
        pid_t pid;
        switch(pid = fork()) {
            case 0:
                break;
            case -1:
                fprintf(stderr, "Couldn't fork process: %s\n", strerror(errno));
                exit(1);
                break;
            default: {
                int j;
                for (j = 0; j < 1000; ++j) {
                    map[0] += 1;
                }
                break;
            }
        }
    }

    if (close(fd) < 0) {
        fprintf(stderr, "Couldn't close testfile.txt: %s\n", strerror(errno));
        exit(1);
    }

    return 0;
}
