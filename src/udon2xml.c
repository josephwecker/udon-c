#include <stdio.h>
#include <stdlib.h>
#include "udon.h"

void emit_xml(void *u) {
    printf("<?xml version=\"1.0\"?>");
    UdonFullNode *root = (UdonFullNode *)u;
    UdonNode *curr_child = root->children;
    UdonList *curr_class = root->classes;
    // Temporary- just testing some basic parser functionality
    while(curr_class != NULL) {
        UdonString *class = (UdonString *)(curr_class->v);
        printf("--- ");
        fwrite(class->start, 1, class->length, stdout);
        printf("\n");
        curr_class = curr_class->next;
    }
    //emit_tag(u);
}

void print_err(UdonError *e) {
    fprintf(stderr, "ERROR: %s\n  Line: %ld\n  Column: %ld\n",
            e->message, e->data_line, e->data_column);
}

int main (int argc, char *argv[]) {
    int i;
    fprintf(stderr, "starting...\n");
    if(argc < 2) {
        fprintf(stderr, "bad argc.\n");
        return 1;
    }
    _UdonParseState *udon = udon_init_from_file(argv[1]);
    if(udon == NULL) {
        print_err(&udon_global_error);
        return udon_global_error.code;
    }
    int res = udon_parse(udon);
    if(res) {
        print_err(&(udon_state(udon)->error));
        return udon_state(udon)->error.code;
    } else {
        emit_xml(udon_state(udon)->result);
    }
    udon_reset_parser(udon);
    udon_free_parser(udon);
    return res;
}

