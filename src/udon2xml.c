#include <stdio.h>
#include <stdlib.h>
#include "udon.h"

int main (int argc, char *argv[]) {
    int i;
    int found = 0;
    if(argc < 2) return 1;
    pstate *state = init_from_file(argv[1]);
    for(i=0; i<10000; i++) {
        found += parse(state);
        reset_state(state);
    }
    free_state(state);
    printf("%d\n", found);
}

