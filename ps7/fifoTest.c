#include <stdio.h>

#include "fifo.h"

int main() {

    struct fifo f;
    fifo_init(&f);

    void * pf;
    pf = (void *)&f;
    printf("%d\n", f.count);

    return 0;
}
