#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

int executeCommand(char * line, char ** argv, int argc);
char ** getArguments(char * line, int * count);
int getCommandArgCount(char ** args, int count);
int getRedirectArgCount(char ** args, int count);

int main(int argc, char ** argv) {
    FILE * fp = stdin;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        executeCommand(line, argv, argc);
    }

    free(line);
    return 0;
}

// Handles argument parsing and command execution
int executeCommand(char * line, char ** argv, int argc) {
    int count = 0;
    char ** args = getArguments(line, &count);

    // Get command arg count and build command arg array
    int commandArgCount = getCommandArgCount(args, count);
    char ** commandArgs = malloc(commandArgCount * sizeof(char *));
    if (commandArgCount > 0) {
        int argi;
        for (argi = 0; argi <= commandArgCount; ++argi) {
            // Add add arg to commandArgs
            commandArgs[argi] = args[argi];
        }
        // Add 0 to last arg element to comply with standard
        commandArgs = realloc(commandArgs, (commandArgCount + 1) * sizeof(char *));
        commandArgs[commandArgCount] = 0;
    }

    // Get redirect arg count and build redirect arg array
    int redirectArgCount = getRedirectArgCount(args, count);
    char ** redirectArgs = malloc(redirectArgCount * sizeof(char *));
    if (redirectArgCount > 0) {
        int argi;
        for (argi = 0; argi <= redirectArgCount; ++argi) {
            // Add add arg to redirectArgs BUT make sure to offset by commandArgsCount to ignore command args
            redirectArgs[argi] = args[argi +  commandArgCount];
        }
    }

    // Display information about command to be executed
    printf("Executing command %s with arguments \"", args[0]);
    if (commandArgCount > 0) {
        int argi;
        // Start at 1 to skip command string
        for (argi = 1; argi < commandArgCount; ++argi) {
            printf("%s", commandArgs[argi]);
            if (commandArgCount - argi > 1 || redirectArgCount > 0) {
                printf(", ");
            }
        }
    }
    if (redirectArgCount > 0) {
        int argi;
        for (argi = 0; argi < redirectArgCount; ++argi) {
            printf("%s", redirectArgs[argi]);
            if (redirectArgCount - argi > 1) {
                printf(", ");
            }
        }
    }
    printf("\"\n");

    // Fork process
    pid_t pid;
    struct rusage ru;
    int status;
    switch(pid = fork()) {
        case 0: {
            // Resolve I/O redirection
            int redi, fd, fileNum;
            for (redi = 0; redi < redirectArgCount; ++redi) {
                char * fileName = NULL;
                if (redirectArgs[redi][0] == '<') {
                    // Redirect stdin
                    fileName = redirectArgs[redi];
                    ++fileName;
                    fd = open(fileName, O_RDONLY);
                    fileNum = STDIN_FILENO;
                } else if (redirectArgs[redi][0] == '>' && redirectArgs[redi][1] != '>') {
                    // Redirect stdout (Open/Create/Truncate)
                    fileName = redirectArgs[redi];
                    ++fileName;
                    fd = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE);
                    fileNum = STDOUT_FILENO;
                } else if (redirectArgs[redi][0] == '2' && redirectArgs[redi][1] == '>') {
                    // Redirect stderr (Open/Create/Truncate)
                    fileName = redirectArgs[redi];
                    fileName += 2;
                    fd = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE);
                    fileNum = STDERR_FILENO;
                } else if (redirectArgs[redi][0] == '>' && redirectArgs[redi][1] == '>') {
                    // Redirect stdout (Open/Create/Append)
                    fileName = redirectArgs[redi];
                    fileName += 2;
                    fd = open(fileName, O_WRONLY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE);
                    fileNum = STDOUT_FILENO;
                } else if (redirectArgs[redi][0] == '2' && redirectArgs[redi][1] == '>' && redirectArgs[redi][2] == '>') {
                    // Redirect stderr (Open/Create/Append)
                    fileName = redirectArgs[redi];
                    fileName += 3;
                    fd = open(fileName, O_WRONLY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE);
                    fileNum = STDERR_FILENO;
                }
                if (fd < 0) {
                    fprintf(stderr, "Can't open file %s: %s\n", fileName, strerror(errno));
                    exit(1);
                }

                // Dup files and close dangling descriptors
                if (dup2(fd, fileNum) < 0) {
                    fprintf(stderr, "Can't dup2 %s to fd %d: %s\n", fileName, fileNum, strerror(errno));
                    exit(1);
                }
                close(fd);
            }

            execvp(args[0], commandArgs);
            break;
        }
        case -1:
            fprintf(stderr, "Fork failed: %s\n", strerror(errno));
            exit(1);
            break;
        default:
            //printf("In parent, pid is %d\n", (int)pid);

            if (wait3(&status, 0, &ru) < 0) {
                fprintf(stderr, "Wait3 error %s", strerror(errno));
                exit(1);
            }

            break;
    }

    free(args);
    free(commandArgs);
    free(redirectArgs);
    return 0;
}

// Splits up command into multiple arguments
char ** getArguments(char * line, int * count) {
    char ** args = NULL;
    char * delimiters = "\t ";

    line[strlen(line) - 1] = '\0';

    char * arg = strtok(line, delimiters);
    while (arg != NULL) {
        args = realloc(args, ++(*count) * sizeof(char *));
        if (args == NULL) {
            fprintf(stderr, "Error reallocating args memory.\n");
            exit(1);
        }

        args[*count-1] = arg;

        arg = strtok(NULL, delimiters);
    }

    return args;
}

// Returns amount of arguments excluding redirections
int getCommandArgCount(char ** args, int count) {
    int i;
    int commandArgCount = 0;
    for (i = 0; i < count; ++i) {
        char * a = args[i];
        if (!(a[0] == '<' ||
              a[0] == '>' ||
             (a[0] == '2' && a[1] == '>'))) {
            ++commandArgCount;
        }
    }

    return commandArgCount;
}

// Returns amount of redirection arguments
int getRedirectArgCount(char ** args, int count) {
    int i;
    int redirectArgCount = 0;
    for (i = 0; i < count; ++i) {
        char * a = args[i];
        if ((a[0] == '<' ||
             a[0] == '>' ||
            (a[0] == '2' && a[1] == '>'))) {
            ++redirectArgCount;
        }
    }

    return redirectArgCount;
}
