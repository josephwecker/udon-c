/* UDON
 * Universal Document Object Notation
 * Parser... scratch... don't know how this will evolve exactly...
 *
 * TODO: rewrite error logic now that it's a library to use return values etc.
 */

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sysexits.h>
#include <err.h>
//#include <malloc.h>

#include "udon.h"
#include "udon_parsing.h"
#include "hsearch.h"

void reset_state(pstate *state) {
    state->p_curr   = state->p_start;
    state->p_quick  = (uint64_t *) state->p_start;
}

pstate *init_from_file(char *filename) {
    ssize_t bytes_read;
    int     fd;
    struct  stat statbuf;
    pstate *state;

    if( (state = (pstate *) malloc(sizeof(pstate))) == NULL)
        err(EX_OSERR, "Couldn't allocate memory for parser state.");
    if( (fd = open(filename, O_RDONLY)) < 0)
        err(EX_NOINPUT, "Couldn't open %s.", filename);

    if( fstat(fd, &statbuf) == -1)
        err(EX_NOINPUT, "Opened, but couldn't stat %s.", filename);

    state->size     = statbuf.st_size;
    state->qsize    = state->size >> 3; // size in uint64_t chunks
    state->filename = filename;
    
    // padding to the right so that quickscan stuff can look in bigger chunks
    if( (state->p_start = (char *) malloc(state->size+8)) == NULL)
        err(EX_OSERR, "Couldn't allocate memory for file contents (%s).", filename);

    if( (bytes_read = read(fd, state->p_start, state->size)) != state->size)
        err(EX_DATAERR, "Only read %zd of %zd bytes from %s.", bytes_read, state->size, filename);

    reset_state(state);
    state->p_end    = &(state->p_start[state->size - 1]);
    state->p_qend   = &(state->p_quick[state->qsize - 1]);
    state->p_curr[state->size] = 0; // Null terminate the whole thing just in case
    close(fd);
    return state;
}

void free_state(pstate *state) {
    if(state != NULL) {
        if(state->p_start != NULL) free(state->p_start);
        state->p_start = NULL;
        free(state);
    }
}

int parse(pstate *state) {
    //mallopt(
    UNPACK_STATE();
    return toplevel(state);
}

int toplevel(pstate *state) {
    UNPACK_STATE();
    int found = 0;
    while(curr <= eof) {
        qscan3('{','[','#');
        switch(*curr) {
            case '{': discard(); found += new_dict(state); break;
            case '[': discard(); found += new_list(state); break;
            case '#': discard(); found ++; break;
            default:  discard(); // EOF
        }
    }
    return found;
}

int new_dict(pstate *state) {

    //printf("\n-->DICT\n%s\n", state->p_curr);
    return 1;
}

int new_list(pstate *state) {
    //printf("\n-->LIST\n%s\n", state->p_curr);
    // - create structure
    // - next_value* until ']'
    return 1;
}

int next_value(pstate *state) {
    // - skipwhites
    // - could be: explicit-string, list, dict, scalar, nothing(comma),
    //             nothing(end-bracket)
    UNPACK_STATE();
    //skipwhites();

}
