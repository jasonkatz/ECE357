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

    if (argc < 2) {
        fprintf(stderr, "Usage: ./a.out [X]\n");
        exit(1);
    }

    int i;

    // Set up signal handlers
    for (i = 1; i <= 31; ++i) {
        // Can't handle SIGKILL or SIGSTOP
        if (i != SIGKILL && i != SIGSTOP) {
            (void)signal(i, sig_handler);
        }
    }

    switch(argv[1][0]) {
        case 'A': {
            // Question A
            int fd;
            if ((fd = open("file1.txt", O_RDONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file1.txt: %s\n", strerror(errno));
                exit(1);
            }

            struct stat sb;
            if (fstat(fd, &sb) < 0) {
                fprintf(stderr, "Couldn't get stats of file: %s\n", strerror(errno));
                exit(1);
            }

            char * map;
            if ((map = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0)) < 0) {
                fprintf(stderr, "Couldn't map to file1.txt: %s\n", strerror(errno));
                exit(1);
            }

            if (close(fd) < 0) {
                fprintf(stderr, "Couldn't close file1.txt: %s\n", strerror(errno));
                exit(1);
            }

            printf("Signal generated for A is:\n");
            sprintf(map, "test");
            setjmp(int_jb);
            break;
        }
        case 'B': {
            // Question B
            int fd;
            if ((fd = open("file2.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file2.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (write(fd, "this is a test", 14) < 0) {
                fprintf(stderr, "Couldn't write to file2.txt: %s\n", strerror(errno));
                exit(1);
            }

            struct stat sb;
            if (fstat(fd, &sb) < 0) {
                fprintf(stderr, "Couldn't get stats of file: %s\n", strerror(errno));
                exit(1);
            }

            char * map;
            if ((map = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0) {
                fprintf(stderr, "Couldn't map to file2.txt: %s\n", strerror(errno));
                exit(1);
            }

            sprintf(map, "h");

            char buf[1];
            if (lseek(fd, 0, SEEK_SET) < 0) {
                fprintf(stderr, "Couldn't seek within file2.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (read(fd, buf, 1) < 0) {
                fprintf(stderr, "Couldn't read from file2.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (buf[0] == 'h') {
                printf("B: The update is visible\n");
            } else {
                printf("B: The update is not visible\n");
            }
            break;
        }
        case 'C': {
            // Question C
            int fd3;
            if ((fd3 = open("file3.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file3.txt: %s\n", strerror(errno));
                exit(1);
            }

            char * map;
            if ((map = mmap(0, 5, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd3, 0)) < 0) {
                fprintf(stderr, "Couldn't map to file3.txt: %s\n", strerror(errno));
                exit(1);
            }

            //sprintf(map, "hello");

            char buf[5];
            if (read(fd3, buf, 5) < 0) {
                fprintf(stderr, "Couldn't read from file3.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (strcmp(buf, "hello") == 0) {
                printf("C: The update is visible\n");
            } else {
                printf("C: The update is not visible\n");
            }
            break;
        }
        case 'D': {
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

            char * map;
            if ((map = mmap(0, numBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd4, 0)) == MAP_FAILED) {
                fprintf(stderr, "Couldn't map to file4.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Created mapping\n");

            map[8195] = 'x';
            break;
        }
    }

    return 0;
}
