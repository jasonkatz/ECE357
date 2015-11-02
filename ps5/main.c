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

jmp_buf int_jb;

void sig_handler(int sn) {
    printf("Signal generated: %d\t%s\n", sn, strsignal(sn));
    // Make sure we can exit the program
    if (sn == SIGINT) {
        exit(1);
    }
    longjmp(int_jb, 1);
}

int main(int argc, char ** argv) {

    int i;

    // Set up signal handlers
    for (i = 1; i <= 31; ++i) {
        // Can't handle SIGKILL or SIGSTOP
        if (i != SIGKILL && i != SIGSTOP) {
            (void)signal(i, sig_handler);
        }
    }

    // Question A
    int fd1;
    if ((fd1 = open("file1.txt", O_RDONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
        fprintf(stderr, "Couldn't open file1.txt: %s\n", strerror(errno));
        exit(1);
    }

    char * map;
    if ((map = (char *)mmap(0, 6, PROT_READ, MAP_SHARED, fd1, 0)) < 0) {
        fprintf(stderr, "Couldn't map to file1.txt: %s\n", strerror(errno));
        exit(1);
    }

    if (close(fd1) < 0) {
        fprintf(stderr, "Couldn't close file1.txt: %s\n", strerror(errno));
        exit(1);
    }

    //map[0] = 'h';
    setjmp(int_jb);

    // Question B
    int fd2;
    if ((fd2 = open("file2.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
        fprintf(stderr, "Couldn't open file2.txt: %s\n", strerror(errno));
        exit(1);
    }

    if ((map = mmap(0, 5, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0)) < 0) {
        fprintf(stderr, "Couldn't map to file2.txt: %s\n", strerror(errno));
        exit(1);
    }

    //sprintf(map, "hello");

    char buf[5];
    if (read(fd2, buf, 5) < 0) {
        fprintf(stderr, "Couldn't read from file2.txt: %s\n", strerror(errno));
        exit(1);
    }
    if (strcmp(buf, "hello") == 0) {
        printf("B: The update is visible\n");
    } else {
        printf("B: The update is not visible\n");
    }

    // Question C
    int fd3;
    if ((fd3 = open("file3.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
        fprintf(stderr, "Couldn't open file3.txt: %s\n", strerror(errno));
        exit(1);
    }

    if ((map = mmap(0, 5, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd3, 0)) < 0) {
        fprintf(stderr, "Couldn't map to file3.txt: %s\n", strerror(errno));
        exit(1);
    }

    //sprintf(map, "hello");

    if (read(fd3, buf, 5) < 0) {
        fprintf(stderr, "Couldn't read from file3.txt: %s\n", strerror(errno));
        exit(1);
    }
    if (strcmp(buf, "hello") == 0) {
        printf("C: The update is visible\n");
    } else {
        printf("C: The update is not visible\n");
    }

    // Question D
    int numBytes = 4100;
    // Create test file
    int fd4;
    if ((fd4 = open("file4.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
        fprintf(stderr, "Couldn't open file2.txt: %s\n", strerror(errno));
        exit(1);
    }
    for (i = 0; i < numBytes; ++i) {
        char randomChar = 'A' + (random() % 26);
        if (write(fd4, &randomChar, 1) < 0) {
            fprintf(stderr, "Couldn't write to file4.txt: %s\n", strerror(errno));
            exit(1);
        }
    }
    printf("Created random test file (file4.txt) with %d character bytes\n", numBytes);

    struct stat sb;
    if (fstat(fd4, &sb) < 0) {
        fprintf(stderr, "Couldn't stat file4.txt: %s\n", strerror(errno));
        exit(1);
    }
    printf("Size of file4.txt (from fstat) is %d\n", (int)sb.st_size);

    if ((map = mmap(0, numBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd4, 0)) == MAP_FAILED) {
        fprintf(stderr, "Couldn't map to file4.txt: %s\n", strerror(errno));
        exit(1);
    }
    printf("Created mapping\n");

    map[8195] = 'x';

    return 0;
}
