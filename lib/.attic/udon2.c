#define _XOPEN_SOURCE 700
#define _REENTRANT

#include <sys/stat.h>
#include <fcntl.h>
#include "udon.h"


char tmp_test[] = "some string or other...";

char blank_string[] = "";
char udon_global_error_msg[128];
UdonError udon_global_error = UDON_NO_ERROR;

UdonParser *udon_new_parser_from_file(char *filename) {
    ssize_t bytes_read;
    int     fd;
    struct  stat statbuf;
    UdonParser *p;

    if( (p = (UdonParser *) udon_malloc(sizeof(UdonParser))) == NULL) {
        udon_error(UDON_OUT_OF_MEMORY_ERROR, "Couldn't allocate memory for parser.");
        return NULL;
    }
    if( (fd = open(filename, O_RDONLY)) < 0) {
        udon_error(UDON_BAD_FILE_ERROR, "Couldn't open file '%s'.", filename);
        return NULL;
    }

    if( fstat(fd, &statbuf) == -1) {
        udon_error(UDON_BAD_FILE_ERROR, "Opened, but couldn't stat file '%s'.", filename);
        return NULL;
    }

    p->size     = statbuf.st_size;
    p->filename = filename;

    // padding to the right so that quickscan stuff can look in bigger chunks
    if( (p->buffer = (char *) udon_malloc(p->size+8)) == NULL)  {
        udon_error(UDON_OUT_OF_MEMORY_ERROR, "Couldn't allocate memory for file contents (file: '%s').", filename);
        return NULL;
    }

    if( (bytes_read = read(fd, p->buffer, p->size)) != p->size) {
        udon_error(UDON_READ_FILE_ERROR, "Only read %zd of %zd bytes from file '%s'.", bytes_read, p->size, filename);
        return NULL;
    }

    udon_reset_parser(p);
    close(fd);
    return p;
}

int udon_reset_parser(UdonParser *p) {
    p->curr          = p->buffer;
    p->end           = &(p->buffer[p->size - 1]);

    p->qcurr         = (uint64_t *) p->buffer;
    p->qsize         = p->size >> 3; // size in 64b chunks
    p->qend          = &(p->qcurr[p->qsize - 1]);

    p->curr[p->size] = 0; // last chance null terminator, just in case.

    p->column        = 1;
    p->line          = 1;
    p->error         = UDON_NO_ERROR;
    p->state         = UDON_NOT_STARTED;
}

int udon_free_parser(UdonParser *p) {

}

 /* warnings       ... accumulate in parser
 *  grammar errors ... specialized error message, propagate finish.
 *  system errors  ... shortcircuit and propagate
 *  eof            ... explicitly handled
 *  unexpected eof ... grammar error
 */

int udon_parse(UdonParser *p) {
    int retval = setjmp(p->err_jmpbuf);
    if(retval) return retval;

    p->result = gm__root(p);

    return 0;
}

static void *gm__root(UdonParser *p) {
    void *s = NULL;
    s = gm__node(p, 0);
    return s;
}

static inline void *gm__node(UdonParser *p, int skip_to_child_shortcut) {
    void *s = NULL;
    if(skip_to_child_shortcut) goto s_child_shortcut;
    int _local_inline = 1;
    uint64_t _local_ibase  = p->column;
    uint64_t _local_ipar   = p->column - 1;

s_initialize:
    // TODO: eof check
    switch(*(p->curr)) {
        case ' ':   // value
        case '\t':  // value
            ADVANCE_COLUMN();
            s = udon_new_value_node(p);
            goto s_value;
        case '\n':  // value
            ADVANCE_LINE();
            s = udon_new_value_node(p);
            goto s_value;
        case ':':   // attr
            ADVANCE_COLUMN();
            s = udon_new_full_node(p);
            goto s_attribute;
        case '[':
            s = udon_new_full_node(p);
            goto s_identity__id;
        case '.':   // class
            s = udon_new_full_node(p);
            goto s_identity__class;
        case '(':   // protected
            s = udon_new_full_node(p);
            ((UdonFullNode *)s)->name = (char *)gm__protected(p);
            goto s_identity;
        case '|':   // child
            s = udon_new_full_node(p);
            goto s_child__node;
        case '{':   // embed
            UDON_ERR("Embedded nodes not yet supported.");
        default:
            s = udon_new_full_node(p);
            ((UdonFullNode *)s)->name = (char *)gm__label(p);
            goto s_identity;
    }

s_identity:
    switch(*(p->curr)) {
        case ' ':   // leading
        case '\t':  // leading
            ADVANCE_COLUMN();
            goto s_identity;
        case '[':   // id
s_identity__id:
            ((UdonFullNode *)s)->id = (char *)gm__protected(p);
            goto s_identity;
        case '.':   // class
s_identity__class:
            ADVANCE_COLUMN();
            udon_list_append_gen(p, &(UDON_SFN->classes), &(UDON_SFN->first_class), gm__label(p));
            goto s_identity;
        default:   // child
            goto s_child;
    }

s_child_shortcut:
    s = udon_new_full_node(p);
    goto s_child;


s_child:
    switch(*(p->curr)) {
        case ' ':  // leading
        case '\t': // leading
            ADVANCE_COLUMN();
            goto s_child;
        case '\n': // newline
            ADVANCE_LINE();
            if(!_local_inline) {
                udon_list_append_gen(p, &(UDON_SFN->children), &(UDON_SFN->first_child), &blank_string);
            }
            _local_inline = 0;
            goto s_child;
        case '#': // comment
            udon_scanto1(p, '\n');
            // TODO: comment_node (?)
            goto s_child;
        default:
            if(p->column <= _local_ipar) { // not-ours
                return s;
            } else {
                switch(*(p->curr)) {
                    case '|':  // node
                        ADVANCE_COLUMN();
                        UDON_APPEND_CHILD(gm__node(p,0));
                        goto s_child;
                    case ':': // attribute
                        ADVANCE_COLUMN();
                        goto s_attribute;
                    default:
                        if(_local_inline) {
                            UDON_APPEND_CHILD(gm__value(p)); // TODO: define
                        } else {
                            UDON_APPEND_CHILD(gm__data(p));  // TODO: define
                        }
                        goto s_child;
                }
            }
    }
s_child__node:

s_attribute:
s_value:
    return s;
}

static inline void *udon_new_value_node(UdonParser *p) {
    // TODO: set source_line, source_col, node_type
    void *res;
    fprintf(stderr, "udon_new_value_node()");
    if( (res = (UdonFullNode *)udon_malloc(sizeof(UdonFullNode))) == NULL)
        UDON_SYSERR("Couldn't allocate memory for a value Udon node.");
    return res;
}

static inline void *udon_new_full_node(UdonParser *p) {
    // TODO: set source_line, source_col, node_type
    void *res;
    fprintf(stderr, "udon_new_full_node()");
    if( (res = (UdonFullNode *)udon_malloc(sizeof(UdonFullNode))) == NULL)
        UDON_SYSERR("Couldn't allocate memory for a full Udon node.");
    return res;
}

static inline void *gm__protected(UdonParser *p) {
    fprintf(stderr, "protected()");
    return &tmp_test;
}

static inline void *gm__label(UdonParser *p) {
    fprintf(stderr, "label()");
    return &tmp_test;
}


