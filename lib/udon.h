#ifndef UDON_H
#define UDON_H

#include <stdint.h>
#include <sys/types.h>
#include "hsearch.h"

// enum of different child types for identifying them in the linked-list

enum UDON_NODE_TYPE {
  UDON_TEXT_NODE,
  UDON_FULL_NODE
};

typedef struct LLIST_ {
  void                *value;
  struct LLIST_       *next;
} llist;

typedef struct UDON_NODE_ {
    /* Basic data / properties */
    enum UDON_NODE_TYPE node_type;
                                             // (most of these are null if it's a text node.)
    char               *type;                // tag/object type
    char               *id;                  // id text if specified
    char               *text_value;          // inline text for nodes, main value for text
    llist              classes;              // classes if defined
    llist              attribute_keys;

    /* For navigating */
    // NOTE: at some point it would be easy to add backwards links or links to
    // parents if someone needs them...
    enum UDON_NODE_TYPE first_child_type;     
    struct UDON_NODE_  *first_child;         // will contain links to additional children (its siblings)

    enum UDON_NODE_TYPE next_sibling_type;
    struct UDON_NODE_  *next_sibling;        // essentially a linked list of parent's children

    /* Misc internally used */
    struct hsearch_data *attributes;          // allows for fetching attr. values
    uint64_t           _num_attributes;
    uint64_t           _max_attributes;      // spaces currently allocated
    //llist            _values_to_free;      // uncomment when/if needed
} udon_node;

typedef struct {
    char              *filename;
    int               fd;
    ssize_t           size;
    ssize_t           qsize;

    char              *p_start;
    uint64_t          *p_quick;
    char              *p_curr;

    char              *p_end;
    uint64_t          *p_qend;

    udon_node         *first_child;          // like a linked list for all children
    // TODO: ref_start for current "word" start...
} pstate;


// Set up, reset, and free parser state object.
pstate *init_from_file(char *filename);
void    reset_state   (pstate *state);
void    free_state    (pstate *state);

// Actual parsing
int     parse         (pstate *state);

// TODO: functions for navigating, fetching attributes, etc.


#endif
