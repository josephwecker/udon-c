#include <stdio.h>
#include <stdlib.h>
#include "udon.h"

int main (int argc, char *argv[]) {
    int i;
    int found = 0;
    pstate *state = init_from_file("../sjson-examples/big.txt");
    for(i=0; i<10000; i++) {
        found += parse(state);
        reset_state(state);
    }
    free_state(state);
    printf("%d\n", found);
}

