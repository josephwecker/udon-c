/*---- genm parser implementation - automatically generated by genmachine ----*/


/*----------------- PRIVATE HEADER ---------------------*/
#define _XOPEN_SOURCE 700
#define _REENTRANT

#include "genm.h"
#include <stddef.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <unistd.h>
{% if use_gmdict %}#include <string.h>{% endif %}

#define _GENM_EOF p->curr == p->end
#define _GENM_ADVANCE_COL()  {p->column ++; p->curr ++;}
#define _GENM_ADVANCE_LINE() {p->column=1; p->line ++; p->curr ++;}

/* --- Error handling --- */
struct GenmError genm_global_error = {
    .code            = GENM_OK,
    .message         = "",

    .parser_file     = __FILE__,
    .parser_line     = __LINE__,
    .parser_function = "",

    .data_file       = "",
    .data_line       = 0,
    .data_column     = 0
};


#define _genm_err_set_global(ecode, msg, ...) {\
    genm_global_error.code = (ecode);\
    genm_global_error.parser_file = __FILE__;\
    genm_global_error.parser_line = __LINE__;\
    genm_global_error.parser_function = __FUNCTION__;\
    snprintf(genm_global_error.message, 255, (msg), ##__VA_ARGS__);\
}

#define _genm_err_set_parser(ecode, msg, ...) {\
    genm_global_error.data_file = p->_public.error.data_file = p->_public.source_origin;\
    genm_global_error.data_line = p->_public.error.data_line = p->line;\
    genm_global_error.data_column = p->_public.error.data_column = p->column;\
    p->_public.error.code = (ecode);\
    p->_public.error.parser_file = __FILE__;\
    p->_public.error.parser_line = __LINE__;\
    p->_public.error.parser_function = __FUNCTION__;\
    snprintf(p->_public.error.message, 255, (msg), ##__VA_ARGS__);\
}

#define genm_err(ecode,msg,...) {\
    _genm_err_set_global(ecode, msg, ##__VA_ARGS__);\
    return NULL;\
}

#define genm_data_err(msg,...) {\
    _genm_err_set_global(GENM_DATA_ERR, msg, ##__VA_ARGS__);\
    _genm_err_set_parser(GENM_DATA_ERR, msg, ##__VA_ARGS__);\
    longjmp(p->err_jmpbuf, GENM_DATA_ERR);\
}

#define genm_memory_err(msg,...) {\
    _genm_err_set_global(GENM_MEMORY_ERR, msg, ##__VA_ARGS__);\
    _genm_err_set_parser(GENM_MEMORY_ERR, msg, ##__VA_ARGS__);\
    if(p->jmpbuf_set) longjmp(p->err_jmpbuf, GENM_MEMORY_ERR);\
    else return NULL;\
}


{% if use_gmdict %}/* --- Dict --- */
struct GenmDictEntry {
    GenmString *key;
    void *value;
    unsigned int used;
};
typedef struct GenmDictEntry GenmDictEntry;

struct GenmDict {
    GenmDictEntry *table;
    uint64_t size;
    uint64_t num_allocated;
    GenmList *keys;
    GenmList *_keys__tail;
};
{% endif %}

/* --- ParseState --- */
struct _GenmParseState {
    GenmParseState _public;
    jmp_buf          err_jmpbuf; /* Where to go on error. */
    unsigned int     jmpbuf_set;

    uint64_t         line;
    uint64_t         column;

    char             *curr;
    uint64_t         *qcurr;
    char             *end;
    uint64_t         *qend;
    size_t           qsize;     /* Automatically calculated, for quickscans. */
    char             *alpha;    /* Used for accumulating. possibly depricated... */
};

/* --- Private prototypes --- */
{% for p in priv_protos %}
{{p}}{% endfor %}


/*----------------- IMPLEMENTATION ---------------------*/

// TODO:
//  * ParseState --> _ParseState public function
//  * Alternate hash routines for language bindings (e.g., natively create ruby
//    hashes immediately, etc.)

inline GenmParseState *genm_state(_GenmParseState *p) { return &(p->_public); }


_GenmParseState *genm_init_from_file(char *filename) {
    size_t bytes_read;
    int     fd;
    struct  stat statbuf;
    _GenmParseState *p;

    if( (p=(_GenmParseState *)genm_malloc(sizeof(_GenmParseState))) == NULL)
        genm_err(GENM_MEMORY_ERR, "Couldn't allocate memory for parser state.");
    if( (fd=open(filename, O_RDONLY)) < 0)
        genm_err(GENM_FILE_OPEN_ERR, "Couldn't open file '%s'.", filename);
    if( fstat(fd, &statbuf) == -1)
        genm_err(GENM_FILE_OPEN_ERR, "Couldn't stat file '%s'.", filename);

    p->_public.source_size   = statbuf.st_size;
    p->_public.source_origin = filename;

    /* The padding is so that quickscan stuff can look in bigger chunks without overflowing */
    if( (p->_public.source_buffer = (char *) genm_malloc(p->_public.source_size+8)) == NULL)
        genm_err(GENM_MEMORY_ERR, "Couldn't allocate memory for file contents (file: '%s').",
                filename);

    if( (bytes_read = read(fd, p->_public.source_buffer, p->_public.source_size)) != p->_public.source_size)
        genm_err(GENM_FILE_READ_ERR, "Only read %zd of %zd bytes from file '%s'.",
                bytes_read, p->_public.source_size, filename);

    genm_reset_state(p);
    close(fd);
    return p;
}

void genm_reset_state(_GenmParseState *p) {
    p->jmpbuf_set                    = 0;
    p->_public.result                = NULL;
    p->_public.error.code            = GENM_OK;
    p->_public.error.message[0]      = 0;
    p->_public.error.parser_file     = __FILE__;
    p->_public.error.parser_line     = __LINE__;
    p->_public.error.parser_function = __FUNCTION__;
    p->_public.error.data_file       = "";
    p->_public.error.data_line       = 0;
    p->_public.error.data_column     = 0;
    p->_public.warnings              = _new_genm_list(p);

    p->line                          = 1;
    p->column                        = 1;
    p->curr                          = p->_public.source_buffer;
    p->qcurr                         = (uint64_t *)p->_public.source_buffer;
    p->end                           = &(p->_public.source_buffer[p->_public.source_size - 1]);
    p->qsize                         = p->_public.source_size >> 3; /* size in 64bit chunks */
    p->qend                          = &(p->qcurr[p->qsize - 1]);

    p->curr[p->_public.source_size]  = 0; /* last chance null terminator, just in case. */
}

int genm_free_parser(_GenmParseState *p) {
    genm_free(p->_public.source_buffer, p->_public.source_size);
    return 0;
}
/*    size_t  bytes_read;
    int     fd;
    struct  stat statbuf;
    _GenmParseState *state;

    if( (state = (_GenmParseState *) genm_malloc(sizeof(_GenmParseState))) == NULL)
        err(GENM_MEMORY_ERR, "Couldn't allocate memory for parser state.");
    if( (fd = open(filename, O_RDONLY)) < 0)
        err(GENM_FILE_OPEN_ERR, "Couldn't open %s.", filename);

    if( fstat(fd, &statbuf) == -1)
        err(GENM_FILE_OPEN_ERR, "Opened, but couldn't stat %s.", filename);

    state->_public.source_size = statbuf.st_size;
    state->qsize    = state->_public.source_size >> 3; // size in uint64_t chunks
    state->_public.source_origin = filename;

    // padding to the right so that quickscan stuff can look in bigger chunks
    if( (state->_public.source_buffer = (char *) genm_malloc(state->_public.source_size+8)) == NULL)
        err(GENM_MEMORY_ERR, "Couldn't allocate memory for file contents (%s).", filename);

    if( (bytes_read = read(fd, state->_public.source_buffer, state->_public.source_size)) != state->_public.source_size)
        err(GENM_FILE_READ_ERR, "Only read %zd of %zd bytes from %s.", bytes_read, state->_public.source_size, filename);

    genm_reset_state(state);
    state->end    = &(state->_public.source_buffer[state->_public.source_size - 1]);
    state->qend   = &(state->qcurr[state->qsize - 1]);
    state->curr[state->_public.source_size] = 0; // Null terminate the whole thing just in case
    close(fd);
    return state;
}

void genm_reset_state(_GenmParseState *p) {
    p->
}
*/

{% if use_gmdict %}/* --- Dict generic code --- */
static inline int is_prime(unsigned int n) {
    /* Rarely run, ok if slow */
    unsigned int div = 3;
    while(div * div < n && n % div != 0) div += 2;
    return n % div != 0;
}

static inline int string_eq(GenmString *a, GenmString *b) {
    if(a==NULL || b==NULL) return 0;
    if(a->length != b->length) return 0;
    return (memcmp(a->start, b->start, a->length) == 0);
}

GenmDict *genm_dict_create(void) {
    GenmDict *dict = malloc(sizeof(GenmDict));
    if(!dict) return NULL;
    dict->num_allocated = 19;
    dict->size = 0;
    dict->table = (GenmDictEntry *) calloc(dict->num_allocated + 1, sizeof(GenmDictEntry));
    if(!dict->table) return NULL;
    return dict;
}

GenmDict *genm_dict_create_sized(size_t s) {
    GenmDict *dict = malloc(sizeof(GenmDict));
    if(!dict) return NULL;
    if(s < 3) s = 3;
    s |= 1; /* ensure odd */
    while(!is_prime(s)) s += 2;
    dict->num_allocated = s;
    dict->size = 0;
    dict->table = (GenmDictEntry *) calloc(dict->num_allocated + 1, sizeof(GenmDictEntry));
    if(!dict->table) return NULL;
    return dict;
}

void genm_dict_destroy(GenmDict *dict) {
    if(dict == NULL) return;
    if(dict->table == NULL) return;
    free(dict->table);
    dict->table = NULL;
}

/* Returns a pointer to the old value */
/* Right now doesn't replace key w/ pointer to textually identical one. Change
 * if for some reason later we need to clean up the old key or something... */
void * genm_dict_add_or_update(GenmDict *dict, GenmString *key, void *new_value) {
    uint64_t idx;
    uint64_t hval = key->length;
    uint64_t count = hval;
    void *   retval;
    while(count-- > 0) {
        hval <<= 4;
        hval += key->start[count];
    }
    if(hval == 0) ++hval;

    idx = hval % dict->num_allocated + 1; /* First simple hash function: modulus (except 0) */
    if(dict->table[idx].used) {
        if(dict->table[idx].used == hval && string_eq(key,dict->table[idx].key)) {
            retval = dict->table[idx].value;
            dict->table[idx].value = new_value;
            return retval;
        }
        uint64_t hval2 = 1 + hval % (dict->num_allocated - 2);
        uint64_t first_idx = idx;
        do {
            if(idx <= hval2) idx = dict->num_allocated + idx - hval2;
            else idx -= hval2;
            if(idx == first_idx) break;
            if(dict->table[idx].used == hval && string_eq(key, dict->table[idx].key)) {
                retval = dict->table[idx].value;
                dict->table[idx].value = new_value;
                return retval;
            }
        } while(dict->table[idx].used);
    }

    /* Not found */
    if(dict->size == dict->num_allocated) {
        /* TODO: realloc (and zero out all new)*/
        return NULL;
    }
    dict->table[idx].used = hval;
    dict->table[idx].key = key;
    dict->table[idx].value = new_value;
    ++dict->size;
    return NULL;
}

void * genm_dict_value_for(GenmDict *dict, GenmString *key) {
    uint64_t idx;
    uint64_t hval = key->length;
    uint64_t count = hval;
    while(count-- > 0) {
        hval <<= 4;
        hval += key->start[count];
    }
    if(hval == 0) ++hval;

    idx = hval % dict->num_allocated + 1; /* First simple hash function: modulus (except 0) */
    if(dict->table[idx].used) {
        if(dict->table[idx].used == hval && string_eq(key,dict->table[idx].key)) {
            return dict->table[idx].value;
        }
        uint64_t hval2 = 1 + hval % (dict->num_allocated - 2);
        uint64_t first_idx = idx;
        do {
            /* Steps through all available indices because size is prime */
            if(idx <= hval2) idx = dict->num_allocated + idx - hval2;
            else idx -= hval2;
            if(idx == first_idx) return NULL;
            if(dict->table[idx].used == hval && string_eq(key, dict->table[idx].key)) {
                return dict->table[idx].value;
            }
        } while(dict->table[idx].used);
    }
    return NULL;
}
{% endif %}

{% for f in functions %}
{{f}}
{% endfor %}
