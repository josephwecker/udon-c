#include <stdio.h>
#include <stdlib.h>
#include "udon.h"

#define P(m) printf((m))
#define NL printf("\n")
#define LIST(x) ((UdonList *)x)
#define PSTR(s) if(s) fwrite(((UdonString *)s)->start, 1, ((UdonString *)s)->length, stdout)

static void pp(void *n, int ilvl);

static void print_err(UdonError *e) {
    fprintf(stderr, "ERROR: %s\n  Line: %ld\n  Column: %ld\n", e->message, (unsigned long int)e->data_line, (unsigned long int)e->data_column);
}

static void p_ind(int ilvl) {int i; for(i=0; i < ilvl; i++) printf("  "); }

static void pp_node(UdonNode *n, int ilvl) {
    // |name.classes[id]
    //   :attr-key  value
    //   ...
    //   children...

    UdonList *klass = n->classes;
    UdonList *child = n->children;

    p_ind(ilvl);
    P("|"); PSTR(n->name);
    while(klass != NULL) {
        P("."); PSTR(klass);
        klass = klass->next;
    }
    // TODO: id
    NL;
    // TODO: attributes

    while(child != NULL) {
        pp(child, ilvl+1);
        child = child->next;
    }
}

static void pp(void *n, int ilvl) {
    if(LIST(n)->listable_type == UDON_STRING_TYPE) {
        p_ind(ilvl); PSTR(n); NL;
    } else if(LIST(n)->listable_type == UDON_NODE_TYPE) {
        pp_node((UdonNode *)n, ilvl);
    } else {
        fprintf(stderr, "Error: Huhh???\n");
        return;
    }
}

int main (int argc, char *argv[]) {
    fprintf(stderr, "starting...\n");
    if(argc < 2) {
        fprintf(stderr, "Please specify at least one file to parse.\n");
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
        pp(udon_state(udon)->result, 0);
    }
    udon_reset_parser(udon);
    udon_free_parser(udon);
    return res;
}

