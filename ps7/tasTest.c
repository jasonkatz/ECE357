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
#include <sys/resource.h>
#include <sys/wait.h>

int N_PROC = 64;
int * map;

int tas(volatile char *lock);

void readFile();

int main(int argc, char ** argv) {

    if (argc > 1) {
        readFile();
        return 0;
    }

    int fd;
    if ((fd = open("testfile.txt", O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) < 0) {
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

    if ((map = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0) {
        fprintf(stderr, "Couldn't map to file testfile.txt: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }

    map[0] = 0;

    int i;
    int inFork = 0;
    int status;
    char spinlock = 0;
    struct rusage ru;
    for (i = 0; i < N_PROC; ++i) {
        pid_t pid;
        switch(pid = fork()) {
            case 0: {
                inFork = 1;
                int j;
                for (j = 0; j < 10000000; ++j) {
                    while (tas(&spinlock) != 0)
                        ;
                    map[0] += 1;
                    spinlock = 0;
                }
                break;
            }
            case -1:
                fprintf(stderr, "Couldn't fork process: %s\n", strerror(errno));
                exit(1);
                break;
            default: {
                break;
            }
        }
        if (inFork) {
            break;
        }
    }

    if (close(fd) < 0) {
        fprintf(stderr, "Couldn't close testfile.txt: %s\n", strerror(errno));
        exit(1);
    }

    return 0;
}

void readFile() {
    int fd;
    if ((fd = open("testfile.txt", O_RDONLY, S_IREAD)) < 0) {
        fprintf(stderr, "Couldn't open testfile.txt: %s\n", strerror(errno));
        exit(1);
    }

    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        fprintf(stderr, "Couldn't get stats of file: %s\n", strerror(errno));
        exit(1);
    }

    if ((map = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0)) < 0) {
        fprintf(stderr, "Couldn't map to file testfile.txt: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }

    printf("Final value is %d\n", map[0]);
}
