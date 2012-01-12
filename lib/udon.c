/*---- udon parser implementation - automatically generated by genmachine ----*/


/*----------------- PRIVATE HEADER ---------------------*/
#define _XOPEN_SOURCE 700
#define _REENTRANT

#include "udon.h"
#include <stddef.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

#define _UDON_EOF p->curr == p->end

int udon_global_error = UDON_OK;
char udon_global_error_msg[128];

/* --- Dict --- */
struct UdonDictEntry {
    UdonString *key;
    void *value;
    unsigned int used;
};
typedef struct UdonDictEntry UdonDictEntry;

struct UdonDict {
    UdonDictEntry *table;
    uint64_t size;
    uint64_t num_allocated;
    UdonList *keys;
    UdonList *_keys__tail;
};


/* --- ParseState --- */
struct _UdonParseState {
    UdonParseState _public;
    jmp_buf          err_jmpbuf; /* Where to go on error. */

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

static inline UdonFullNode * _udon_node(_UdonParseState *p);
static inline UdonString * _udon_label(_UdonParseState *p);
static inline UdonFullNode * _udon_node__s_child_shortcut(_UdonParseState *p);
static inline UdonString * _udon_label__s_delim(_UdonParseState *p);
static inline void * _udon_id(_UdonParseState *p);
static inline void * _udon_comment(_UdonParseState *p);
static inline UdonFullNode * _new_udon_full_node(_UdonParseState *p);
static inline UdonList * _new_udon_list(_UdonParseState *p);
static inline UdonString * _new_udon_string(_UdonParseState *p);


/*----------------- IMPLEMENTATION ---------------------*/

// TODO:
//  * ParseState --> _ParseState public function
//  * Alternate hash routines for language bindings (e.g., natively create ruby
//    hashes immediately, etc.)

inline UdonParseState *udon_state(_UdonParseState *p) { return &(p->_public); }

_UdonParseState *udon_init_from_file(char *filename) {
    size_t  bytes_read;
    int     fd;
    struct  stat statbuf;
    _UdonParseState *state;

    if( (state = (_UdonParseState *) udon_malloc(sizeof(_UdonParseState))) == NULL)
        err(UDON_MEMORY_ERR, "Couldn't allocate memory for parser state.");
    if( (fd = open(filename, O_RDONLY)) < 0)
        err(UDON_FILE_OPEN_ERR, "Couldn't open %s.", filename);

    if( fstat(fd, &statbuf) == -1)
        err(UDON_FILE_OPEN_ERR, "Opened, but couldn't stat %s.", filename);

    state->_public.source_size = statbuf.st_size;
    state->qsize    = state->_public.source_size >> 3; // size in uint64_t chunks
    state->_public.source_origin = filename;

    // padding to the right so that quickscan stuff can look in bigger chunks
    if( (state->_public.source_buffer = (char *) udon_malloc(state->_public.source_size+8)) == NULL)
        err(UDON_MEMORY_ERR, "Couldn't allocate memory for file contents (%s).", filename);

    if( (bytes_read = read(fd, state->_public.source_buffer, state->_public.source_size)) != state->_public.source_size)
        err(UDON_FILE_READ_ERR, "Only read %zd of %zd bytes from %s.", bytes_read, state->_public.source_size, filename);

    reset_state(state);
    state->end    = &(state->_public.source_buffer[state->_public.source_size - 1]);
    state->qend   = &(state->qcurr[state->qsize - 1]);
    state->curr[state->_public.source_size] = 0; // Null terminate the whole thing just in case
    close(fd);
    return state;
}

/* --- Dict generic code --- */
static inline int is_prime(unsigned int n) {
    /* Rarely run, ok if slow */
    unsigned int div = 3;
    while(div * div < n && n % div != 0) div += 2;
    return n % div != 0;
}

static inline int string_eq(UdonString *a, UdonString *b) {
    if(a==NULL || b==NULL) return 0;
    if(a->length != b->length) return 0;
    return (memcmp(a->start, b->start, a->length) == 0);
}

UdonDict *udon_dict_create(void) {
    UdonDict *dict = malloc(sizeof(UdonDict));
    if(!dict) return NULL;
    dict->num_allocated = 19;
    dict->size = 0;
    dict->table = (UdonDictEntry *) calloc(dict->num_allocated + 1, sizeof(UdonDictEntry));
    if(!dict->table) return NULL;
    return dict;
}

UdonDict *udon_dict_create_sized(size_t s) {
    UdonDict *dict = malloc(sizeof(UdonDict));
    if(!dict) return NULL;
    if(s < 3) s = 3;
    s |= 1; /* ensure odd */
    while(!is_prime(s)) s += 2;
    dict->num_allocated = s;
    dict->size = 0;
    dict->table = (UdonDictEntry *) calloc(dict->num_allocated + 1, sizeof(UdonDictEntry));
    if(!dict->table) return NULL;
    return dict;
}

void udon_dict_destroy(UdonDict *dict) {
    if(dict == NULL) return;
    if(dict->table == NULL) return;
    free(dict->table);
    dict->table = NULL;
}

/* Returns a pointer to the old value */
/* Right now doesn't replace key w/ pointer to textually identical one. Change
 * if for some reason later we need to clean up the old key or something... */
void * udon_dict_add_or_update(UdonDict *dict, UdonString *key, void *new_value) {
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

void * udon_dict_value_for(UdonDict *dict, UdonString *key) {
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



int udon_parse(_UdonParseState *p) {
    int errval                   = setjmp(p->err_jmpbuf);
    if(errval) return errval;
    p->_public.result            = (void *)_udon_node__s_child_shortcut(p);
    return 0;
}


static inline UdonFullNode * _udon_node(_UdonParseState *p) {
    UdonFullNode * self_res      = _new_udon_full_node(p);
    uint64_t inl                 = 1;
    uint64_t ibase               = p->column;
    uint64_t ipar                = p->column-1;
    s_init:
        if(_UDON_EOF) {
            ((UdonNode *)self_res)->node_type = UDON_BLANK;
            return self_res;
        } else {
          _inner_s_init:
            switch(*(p->curr)) {
                case '\n':  /*-- init.value1 ---*/
                    _UDON_ADVANCE_LINE();
                    ((UdonNode *)self_res)->node_type = UDON_VALUE;
                    goto s_value;
                case ' ':
                case '\t':  /*-- init.value2 ---*/
                    _UDON_ADVANCE_COL();
                    ((UdonNode *)self_res)->node_type = UDON_VALUE;
                    goto s_value;
                case ':':   /*-- init.attr -----*/
                    _UDON_ADVANCE_COL();
                    goto s_attribute;
                case '[':   /*-- init.id -------*/   goto s_identity__id;
                case '.':   /*-- init.class ----*/   goto s_identity__class;
                case '(':   /*-- init.delim ----*/
                    self_res->name = _udon_label__s_delim(p);
                    goto s_identity;
                case '|':   /*-- init.child ----*/   goto s_child__node;
                case '{':   /*-- init.embed ----*/   _UDON_ERR("Embedded nodes are not yet supported");
                default:    /*-- init.name -----*/
                    self_res->name = _udon_label(p);
                    goto s_identity;
            }
        }
    s_identity:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_identity:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- identity.lead -*/
                    _UDON_ADVANCE_COL();
                    goto s_identity;
                case '[':   /*-- identity.id ---*/
                  s_identity__id:
                    self_res->id = _udon_id(p);
                    goto s_identity;
                case '.':   /*-- identity.class */
                  s_identity__class:
                    _UDON_ADVANCE_COL();
                    {
                        UdonList *_item = _new_udon_list(p);
                        _item->v = (void *)_udon_label(p);
                        _item->next = NULL;
                        if(self_res->_classes__tail == NULL) {
                            self_res->classes = self_res->_classes__tail = _item;
                        } else {
                            self_res->_classes__tail->next = _item;
                            self_res->_classes__tail = self_res->_classes__tail->next;
                        }
                    }
                    goto s_identity;
                default:    /*-- identity.child */   goto _inner_s_child;
            }
        }
    s_child_shortcut:
        ((UdonNode *)self_res)->node_type = UDON_ROOT;
        goto _inner_s_child;
    s_child:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_child:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- child.lead ----*/
                    _UDON_ADVANCE_COL();
                    goto s_child;
                case '\n':  /*-- child.nl ------*/
                    if(!inl) {
                        {
                            UdonList * _item  = (UdonList *)(_udon_node(p));
                            UdonList * *_acc_head = (UdonList **) &(self_res->children);
                            UdonList * *_acc_tail = (UdonList **) &(self_res->_children__tail);
                            if(*_acc_tail == NULL) {
                                *_acc_head = *_acc_tail = _item;
                            } else {
                                (*_acc_tail)->next = _item;
                                *_acc_tail = _item;
                            }
                        }
                    }
                    inl          = 0;
                    goto s_child;
                case '#':   /*-- child.comment -*/
                    _UDON_ADVANCE_COL();
                    _udon_comment(p);
                    goto s_child;
            }
        }
    s_value:
        _UDON_ERR("Parser for 'value' in 'node' not yet implemented.");
    s_attribute:
        _UDON_ERR("Parser for 'attribute' in 'node' not yet implemented.");
    s_child__node:
        _UDON_ERR("Parser for 'child' in 'node' (substate 'node') not yet implemented.");
    _eof:
        return self_res;
}


static inline UdonString * _udon_label(_UdonParseState *p) {
    UdonString * self_res        = _new_udon_string(p);
    uint64_t lvl                 = 0;
    self_res->start              = p->curr;
    s_init:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_init:
            switch(*(p->curr)) {
                case '(':   /*-- init.delim ----*/   goto s_delim__nest;
                case ' ':
                case '\t':
                case '\n':
                case '[':
                case '|':
                case '.':
                case '!':   /*-- init.done -----*/
                    self_res->length = p->curr - self_res->start;
                    return self_res;
                default:    /*-- init.collect --*/
                    _UDON_ADVANCE_COL();
                    goto s_init;
            }
        }
    s_delim:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_delim:
            switch(*(p->curr)) {
                case '(':   /*-- delim.nest ----*/
                  s_delim__nest:
                    _UDON_ADVANCE_COL();
                    lvl         += 1;
                    goto s_delim;
                case ')':   /*-- delim.unnest --*/
                    _UDON_ADVANCE_COL();
                    lvl         -= 1;
                    if(lvl==0) return self_res;
                    else goto s_delim;
                case '\n':  /*-- delim.collect1 */
                    _UDON_ADVANCE_LINE();
                    goto s_delim;
                default:    /*-- delim.collect2 */
                    _UDON_ADVANCE_COL();
                    goto s_delim;
            }
        }
    _eof:
        self_res->length         = p->curr - self_res->start;
        return self_res;
}


static inline UdonFullNode * _udon_node__s_child_shortcut(_UdonParseState *p) {
    UdonFullNode * self_res      = _new_udon_full_node(p);
    uint64_t inl                 = 1;
    uint64_t ibase               = p->column;
    uint64_t ipar                = p->column-1;
    s_child_shortcut:
        ((UdonNode *)self_res)->node_type = UDON_ROOT;
        goto _inner_s_child;
    s_child:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_child:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- child.lead ----*/
                    _UDON_ADVANCE_COL();
                    goto s_child;
                case '\n':  /*-- child.nl ------*/
                    if(!inl) {
                        {
                            UdonList * _item  = (UdonList *)(_udon_node(p));
                            UdonList * *_acc_head = (UdonList **) &(self_res->children);
                            UdonList * *_acc_tail = (UdonList **) &(self_res->_children__tail);
                            if(*_acc_tail == NULL) {
                                *_acc_head = *_acc_tail = _item;
                            } else {
                                (*_acc_tail)->next = _item;
                                *_acc_tail = _item;
                            }
                        }
                    }
                    inl          = 0;
                    goto s_child;
                case '#':   /*-- child.comment -*/
                    _UDON_ADVANCE_COL();
                    _udon_comment(p);
                    goto s_child;
            }
        }
    s_init:
        if(_UDON_EOF) {
            ((UdonNode *)self_res)->node_type = UDON_BLANK;
            return self_res;
        } else {
          _inner_s_init:
            switch(*(p->curr)) {
                case '\n':  /*-- init.value1 ---*/
                    _UDON_ADVANCE_LINE();
                    ((UdonNode *)self_res)->node_type = UDON_VALUE;
                    goto s_value;
                case ' ':
                case '\t':  /*-- init.value2 ---*/
                    _UDON_ADVANCE_COL();
                    ((UdonNode *)self_res)->node_type = UDON_VALUE;
                    goto s_value;
                case ':':   /*-- init.attr -----*/
                    _UDON_ADVANCE_COL();
                    goto s_attribute;
                case '[':   /*-- init.id -------*/   goto s_identity__id;
                case '.':   /*-- init.class ----*/   goto s_identity__class;
                case '(':   /*-- init.delim ----*/
                    self_res->name = _udon_label__s_delim(p);
                    goto s_identity;
                case '|':   /*-- init.child ----*/   goto s_child__node;
                case '{':   /*-- init.embed ----*/   _UDON_ERR("Embedded nodes are not yet supported");
                default:    /*-- init.name -----*/
                    self_res->name = _udon_label(p);
                    goto s_identity;
            }
        }
    s_identity:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_identity:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- identity.lead -*/
                    _UDON_ADVANCE_COL();
                    goto s_identity;
                case '[':   /*-- identity.id ---*/
                  s_identity__id:
                    self_res->id = _udon_id(p);
                    goto s_identity;
                case '.':   /*-- identity.class */
                  s_identity__class:
                    _UDON_ADVANCE_COL();
                    {
                        UdonList *_item = _new_udon_list(p);
                        _item->v = (void *)_udon_label(p);
                        _item->next = NULL;
                        if(self_res->_classes__tail == NULL) {
                            self_res->classes = self_res->_classes__tail = _item;
                        } else {
                            self_res->_classes__tail->next = _item;
                            self_res->_classes__tail = self_res->_classes__tail->next;
                        }
                    }
                    goto s_identity;
                default:    /*-- identity.child */   goto _inner_s_child;
            }
        }
    s_value:
        _UDON_ERR("Parser for 'value' in 'node__s_child_shortcut' not yet implemented.");
    s_attribute:
        _UDON_ERR("Parser for 'attribute' in 'node__s_child_shortcut' not yet implemented.");
    s_child__node:
        _UDON_ERR("Parser for 'child' in 'node__s_child_shortcut' (substate 'node') not yet implemented.");
    _eof:
        return self_res;
}


static inline UdonString * _udon_label__s_delim(_UdonParseState *p) {
    UdonString * self_res        = _new_udon_string(p);
    uint64_t lvl                 = 0;
    self_res->start              = p->curr;
    s_delim:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_delim:
            switch(*(p->curr)) {
                case '(':   /*-- delim.nest ----*/
                  s_delim__nest:
                    _UDON_ADVANCE_COL();
                    lvl         += 1;
                    goto s_delim;
                case ')':   /*-- delim.unnest --*/
                    _UDON_ADVANCE_COL();
                    lvl         -= 1;
                    if(lvl==0) return self_res;
                    else goto s_delim;
                case '\n':  /*-- delim.collect1 */
                    _UDON_ADVANCE_LINE();
                    goto s_delim;
                default:    /*-- delim.collect2 */
                    _UDON_ADVANCE_COL();
                    goto s_delim;
            }
        }
    s_init:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_init:
            switch(*(p->curr)) {
                case '(':   /*-- init.delim ----*/   goto s_delim__nest;
                case ' ':
                case '\t':
                case '\n':
                case '[':
                case '|':
                case '.':
                case '!':   /*-- init.done -----*/
                    self_res->length = p->curr - self_res->start;
                    return self_res;
                default:    /*-- init.collect --*/
                    _UDON_ADVANCE_COL();
                    goto s_init;
            }
        }
    _eof:
        self_res->length         = p->curr - self_res->start;
        return self_res;
}


static inline void * _udon_id(_UdonParseState *p) {
    _UDON_ERR("Parser for 'id' not yet implemented.");
    return NULL;
}


static inline void * _udon_comment(_UdonParseState *p) {
    _UDON_ERR("Parser for 'comment' not yet implemented.");
    return NULL;
}


static inline UdonFullNode * _new_udon_full_node(_UdonParseState *p) {
    UdonFullNode * res           = (UdonFullNode *)udon_malloc(sizeof(UdonFullNode));
    if(!res) _UDON_MEM_ERR("Memory allocation failed for FullNode.");
    return res;
}


static inline UdonList * _new_udon_list(_UdonParseState *p) {
    UdonList * res               = (UdonList *)udon_malloc(sizeof(UdonList));
    if(!res) _UDON_MEM_ERR("Memory allocation failed for LIST.");
    return res;
}


static inline UdonString * _new_udon_string(_UdonParseState *p) {
    UdonString * res             = (UdonString *)udon_malloc(sizeof(UdonString));
    if(!res) _UDON_MEM_ERR("Memory allocation failed for STRING.");
    return res;
}


