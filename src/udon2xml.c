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

int main (int argc, char *argv[]) {
    int i;
    if(argc < 2) return 1;
    UdonParseState *udon = udon_new_parser_from_file(argv[1]);
    if(udon == NULL) {
        return 1;
        //udon_emit_error(stderr);
        //return udon_error_value();
    }
    int res = udon_parse(udon);
    if(res) {
        return 1;
        //udon_emit_error(stderr);
    } else {
        emit_xml(udon->result);
    }
    //udon_reset_parser(udon);
    //udon_free_parser(udon);
    return res;
}

