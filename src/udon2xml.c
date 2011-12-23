#include <stdio.h>
#include <stdlib.h>
#include "udon.h"

int main (int argc, char *argv[]) {
    int i;
    int found = 0;
    if(argc < 2) return 1;
    UdonParser *udon = udon_new_parser_from_file(argv[1]);
    if(udon == NULL) {
        udon_emit_error(stderr);
        return udon_error_value();
    }
    for(i=0; i<10000; i++) {
        found += udon_parse(udon);
        udon_reset_parser(udon);
    }
    udon_free_parser(udon);
    printf("%d\n", found);
}

