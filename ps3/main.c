#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char ** argv, char ** envp) {

    char * args[] = { "", "-l" };

    if (execvp("ls", args) == -1) {
        printf("Could not run execve: %s\n", strerror(errno));
    }

    return 0;
}
