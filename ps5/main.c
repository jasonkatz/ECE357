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
                fprintf(stderr, "Couldn't get stats of file2.txt: %s\n", strerror(errno));
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
            if (buf[0] == map[0]) {
                printf("B: The update is visible\n");
            } else {
                printf("B: The update is not visible\n");
            }

            if (close(fd) < 0) {
                fprintf(stderr, "Couldn't close file2.txt: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
        case 'C': {
            // Question C
            int fd;
            if ((fd = open("file3.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file3.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (write(fd, "this is a test", 14) < 0) {
                fprintf(stderr, "Couldn't write to file3.txt: %s\n", strerror(errno));
                exit(1);
            }

            struct stat sb;
            if (fstat(fd, &sb) < 0) {
                fprintf(stderr, "Couldn't get stats of file3.txt: %s\n", strerror(errno));
                exit(1);
            }

            char * map;
            if ((map = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) < 0) {
                fprintf(stderr, "Couldn't map to file3.txt: %s\n", strerror(errno));
                exit(1);
            }

            sprintf(map, "h");

            char buf[1];
            if (lseek(fd, 0, SEEK_SET) < 0) {
                fprintf(stderr, "Couldn't seek within file3.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (read(fd, buf, 1) < 0) {
                fprintf(stderr, "Couldn't read from file3.txt: %s\n", strerror(errno));
                exit(1);
            }
            if (buf[0] == map[0]) {
                printf("The update is visible\n");
            } else {
                printf("The update is not visible\n");
            }

            if (close(fd) < 0) {
                fprintf(stderr, "Couldn't close file3.txt: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
        case 'D': {
            // Question D
            int numBytes = 4100;
            // Create test file
            int fd;
            if ((fd = open("file4.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file4.txt: %s\n", strerror(errno));
                exit(1);
            }
            for (i = 0; i < numBytes; ++i) {
                char randomChar = 'A' + (random() % 26);
                if (write(fd, &randomChar, 1) < 0) {
                    fprintf(stderr, "Couldn't write to file4.txt: %s\n", strerror(errno));
                    exit(1);
                }
            }
            printf("Created random test file (file4.txt) with %d character bytes\n", numBytes);

            struct stat sb;
            if (fstat(fd, &sb) < 0) {
                fprintf(stderr, "Couldn't stat file4.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Size of file4.txt (from fstat) is %d\n", (int)sb.st_size);

            char * map;
            if ((map = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
                fprintf(stderr, "Couldn't map to file4.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Created mapping\n");

            map[(int)sb.st_size + 1] = 'T';
            map[(int)sb.st_size + 2] = 'E';
            map[(int)sb.st_size + 3] = 'S';
            map[(int)sb.st_size + 4] = 'T';
            printf("Added 4 bytes\n");

            if (fstat(fd, &sb) < 0) {
                fprintf(stderr, "Couldn't stat file4.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Size of file4.txt (from fstat) after addition is %d\n", (int)sb.st_size);
            printf("This size doesn't change because the inode entry doesn't get updated. Stat gets it's data from the inode table, so there is no change\n");

            if (close(fd) < 0) {
                fprintf(stderr, "Couldn't close file4.txt: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
        case 'E': {
            // Question E
            int numBytes = 4100;
            // Create test file
            int fd;
            if ((fd = open("file5.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file5.txt: %s\n", strerror(errno));
                exit(1);
            }
            for (i = 0; i < numBytes; ++i) {
                char randomChar = 'A' + (random() % 26);
                if (write(fd, &randomChar, 1) < 0) {
                    fprintf(stderr, "Couldn't write to file5.txt: %s\n", strerror(errno));
                    exit(1);
                }
            }
            printf("Created random test file (file5.txt) with %d character bytes\n", numBytes);

            struct stat sb;
            if (fstat(fd, &sb) < 0) {
                fprintf(stderr, "Couldn't stat file5.txt: %s\n", strerror(errno));
                exit(1);
            }

            char * map;
            if ((map = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
                fprintf(stderr, "Couldn't map to file5.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Created mapping\n");

            map[(int)sb.st_size + 1] = 'T';
            map[(int)sb.st_size + 2] = 'E';
            map[(int)sb.st_size + 3] = 'S';
            map[(int)sb.st_size + 4] = 'T';
            printf("Added 4 bytes\n");

            if (lseek(fd, 100, SEEK_END) < 0) {
                fprintf(stderr, "Couldn't seek in file5.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Created hole in file5.txt with lseek\n");

            if (map[(int)sb.st_size + 4] == 'T') {
                printf("The inserted data is still there after the creation of a hole\n");
            } else {
                printf("The inserted data is overwritten after the creation of a hole\n");
            }

            if (close(fd) < 0) {
                fprintf(stderr, "Couldn't close file5.txt: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
        case 'F': {
            // Question F

            // Create test file
            int fd;
            int numBytes = 10;
            if ((fd = open("file6.txt", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
                fprintf(stderr, "Couldn't open file6.txt: %s\n", strerror(errno));
                exit(1);
            }
            for (i = 0; i < numBytes; ++i) {
                char randomChar = 'A' + (random() % 26);
                if (write(fd, &randomChar, 1) < 0) {
                    fprintf(stderr, "Couldn't write to file6.txt: %s\n", strerror(errno));
                    exit(1);
                }
            }
            printf("Created random test file (file6.txt) with %d character bytes\n", numBytes);

            char * map;
            if ((map = mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0) {
                fprintf(stderr, "Couldn't map to file6.txt: %s\n", strerror(errno));
                exit(1);
            }
            printf("Mapped small test file (file6.txt) to mapping of 8192 bytes\n");
            //printf("Accessing second page: %c\nSignal:\n", map[5000]);
            printf("Accessing first page: %c\nSignal:\n", map[1000]);

            if (close(fd) < 0) {
                fprintf(stderr, "Couldn't close file6.txt: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
    }

    return 0;
}
