#include <stdio.h>
#include <unistd.h>

int main(int argc, char ** argv) {

    write(STDOUT_FILENO, "Hello, world!\n", 15);

    return 0;
}
