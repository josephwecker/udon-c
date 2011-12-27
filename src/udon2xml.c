#include <stdio.h>
#include <stdlib.h>
#include "udon.h"

int main (int argc, char *argv[]) {
    int i;
    if(argc < 2) return 1;
    UdonParser *udon = udon_new_parser_from_file(argv[1]);
    if(udon == NULL) {
        udon_emit_error(stderr);
        return udon_error_value();
    }
    int res = udon_parse(udon);
    if(res) udon_emit_error(stderr);
    udon_reset_parser(udon);
    udon_free_parser(udon);
    return res;
}

