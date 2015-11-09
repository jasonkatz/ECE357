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
    if ((fd = open("testfile.txt", O_RDWR | O_CREAT | O_TRUNC, 0777) < 0)) {
        fprintf(stderr, "Couldn't open testfile.txt: %s\n", strerror(errno));
        exit(1);
    }
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        fprintf(stderr, "Couldn't get stats of file: %s\n", strerror(errno));
        exit(1);
    }

    char * map;
    if ((map = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) < 0)) {
        fprintf(stderr, "Couldn't map to file testfile.txt: %s\n", strerror(errno));
        exit(1);
    }

    if (close(fd) < 0) {
        fprintf(stderr, "Couldn't close testfile.txt: %s\n", strerror(errno));
        exit(1);
    }

    return 0;
}
