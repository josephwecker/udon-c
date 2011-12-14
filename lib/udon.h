#ifndef UDON_H
#define UDON_H

#include <stdint.h>
#include <sys/types.h>
#include "hsearch.h"


enum UdonType {
    NODE,
    TEXT,
    ATTR_VALUE,
    CLASS,
    PARENT,
    COMMENT,
    FULL
};

typedef struct _UdonList {
    void                      *v;
    struct _UdonList          *next;
} UdonList;

typedef struct _UdonNode {
    UdonList                  _base;           // Acts like a linked list
    enum UdonType             node_type;       // Node type, from enum above.
    uint64_t                  source_line;     // Line where the parser encountered it.
    uint64_t                  source_col;      // Column on the line where the parser encountered it.
} UdonNode;

typedef struct _UdonComment {
    UdonNode                  _base;           // Extends simple node
    UdonNode                  *children;       // Linked list of children
} UdonComment;

typedef struct _UdonAttributes {
    UdonList                  *keys;           // Linked list of keys
    struct hsearch_data       *hash_table;     // Actual hash table
    uint64_t                  size;            // How many attributes defined so far
    uint64_t                  num_allocated;   // If size surpasses this, a new hash table must be created.
} UdonAttributes;

struct UdonFullNode {
    struct UdonNode           _base;           // Extend basic node
    UdonNode                  *children;       // (same spot as UdonComment) - children linked-list
    char                      *type;           // Not to be confused w/ node_type- this is the "tag"
    char                      *id;             // If specified, the ID string
    UdonList                  *classes;        // Linked list of classes
    UdonAttributes            attributes;      // See structure above
};

typedef struct _UdonParseState {
    char                      *filename;
    int                       fd;
    ssize_t                   size;
    ssize_t                   qsize;

    char                      *p_start;
    uint64_t                  *p_quick;
    char                      *p_curr;

    char                      *p_end;
    uint64_t                  *p_qend;

    UdonNode                  *root_node;
} UdonParseState;


// Set up, reset, and free parser state object.
UdonParseState *udon_init_from_file(char *filename);
void            udon_reset_state   (UdonParseState *state);
void            udon_free_state    (UdonParseState *state);

// Actual parsing
int             udon_parse         (UdonParseState *state);

// TODO: functions for navigating, fetching attributes, etc.
// TODO: interface for genmachine

#endif
