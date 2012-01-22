/*---- udon parser implementation - automatically generated by udonachine ----*/


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
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* --- Error handling --- */
struct UdonError udon_global_error = {
    .code            = UDON_OK,
    .message         = "",

    .parser_file     = __FILE__,
    .parser_line     = __LINE__,
    .parser_function = "",

    .data_file       = "",
    .data_line       = 0,
    .data_column     = 0
};


#define _udon_err_set_global(ecode, msg, ...) {\
    udon_global_error.code = (ecode);\
    udon_global_error.parser_file = __FILE__;\
    udon_global_error.parser_line = __LINE__;\
    udon_global_error.parser_function = __FUNCTION__;\
    snprintf(udon_global_error.message, 255, (msg), ##__VA_ARGS__);\
}

#define _udon_err_set_parser(ecode, msg, ...) {\
    udon_global_error.data_file = p->_public.error.data_file = p->_public.source_origin;\
    udon_global_error.data_line = p->_public.error.data_line = p->line;\
    udon_global_error.data_column = p->_public.error.data_column = p->column;\
    p->_public.error.code = (ecode);\
    p->_public.error.parser_file = __FILE__;\
    p->_public.error.parser_line = __LINE__;\
    p->_public.error.parser_function = __FUNCTION__;\
    snprintf(p->_public.error.message, 255, (msg), ##__VA_ARGS__);\
}

#define udon_err(ecode,msg,...) {\
    _udon_err_set_global(ecode, msg, ##__VA_ARGS__);\
    return NULL;\
}

#define udon_data_err(msg,...) {\
    _udon_err_set_global(UDON_DATA_ERR, msg, ##__VA_ARGS__);\
    _udon_err_set_parser(UDON_DATA_ERR, msg, ##__VA_ARGS__);\
    longjmp(p->err_jmpbuf, UDON_DATA_ERR);\
}

#define udon_memory_err(msg,...) {\
    _udon_err_set_global(UDON_MEMORY_ERR, msg, ##__VA_ARGS__);\
    _udon_err_set_parser(UDON_MEMORY_ERR, msg, ##__VA_ARGS__);\
    if(p->jmpbuf_set) longjmp(p->err_jmpbuf, UDON_MEMORY_ERR);\
    else return NULL;\
}


/* --- Scanning --- */
#define _UDON_EOF p->curr >= p->end
#define _UDON_ADVANCE_COL()  {p->column ++; p->curr ++;}
#define _UDON_ADVANCE_LINE() {p->column=1; p->line ++; p->curr ++;}

#define udon_q_haszero(v)   ((v) - UINT64_C(0x0101010101010101)) & ~(v) & UINT64_C(0x8080808080808080)
#define udon_q_hasval(v,n)  (udon_q_haszero((v) ^ (~UINT64_C(0)/255 * (n))))

#define _UDON_QSCAN_TO1(c1) {\
    uint64_t *qcurr = (uint64_t *)p->curr;\
    while((qcurr <= p->qend) && !udon_q_hasval(*qcurr,(c1))) qcurr++;\
    p->curr = (char *)qcurr;\
    while(1) {\
        if(p->curr >= p->end) goto _eof;\
        if(*(p->curr) == (c1)) break;\
        p->curr ++;\
    }\
}

#define _UDON_QSCAN_TO3(c1,c2,c3) {\
    uint64_t *qcurr = (uint64_t *)p->curr;\
    while((qcurr <= p->qend) && !(\
                udon_q_hasval(*qcurr,(c1)) || \
                udon_q_hasval(*qcurr,(c2)) || \
                udon_q_hasval(*qcurr,(c3))))\
        qcurr++;\
    p->curr = (char *)qcurr;\
    while(1) {\
        if(p->curr >= p->end) goto _eof;\
        if(*(p->curr) == (c1) || *(p->curr) == (c2) || *(p->curr) == (c3)) break;\
        p->curr ++;\
    }\
}

#define _UDON_QSCAN_PAST1(c1) {\
    _UDON_QSCAN_TO1((c1));\
    _UDON_ADVANCE_COL();\
}

#define _UDON_QSCAN_PAST1_NL(c1) {\
    _UDON_QSCAN_TO1((c1));\
    _UDON_ADVANCE_LINE();\
}


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
    unsigned int     jmpbuf_set;

    uint64_t         line;
    uint64_t         column;

    char             *curr;
    char             *end;
    uint64_t         *qend;
    char             *alpha;    /* Used for accumulating. possibly depricated... */
};

/* --- Private prototypes --- */

static inline UdonNode *     _udon_node(_UdonParseState *p);
static inline UdonString *   _udon_data(_UdonParseState *p);
static inline UdonString *   _udon_value(_UdonParseState *p);
static inline UdonString *   _udon_label(_UdonParseState *p);
static inline UdonString *   _udon_id(_UdonParseState *p);
static inline void           _udon_block_comment(_UdonParseState *p);
static inline UdonNode *     _udon_node__s_child_shortcut(_UdonParseState *p);
static inline UdonString *   _udon_label__s_delim(_UdonParseState *p);
static inline UdonNode *     _new_udon_node(_UdonParseState *p);
static inline UdonString *   _new_udon_string(_UdonParseState *p);


/*----------------- IMPLEMENTATION ---------------------
 * TODO:
 *  - ParseState --> _ParseState public function
 *  - Alternate hash routines for language bindings (e.g., natively create ruby
 *    hashes immediately, etc.)
 */

inline UdonParseState *udon_state(_UdonParseState *p) { return &(p->_public); }

_UdonParseState *udon_init_from_file(char *filename) {
    size_t bytes_read;
    int     fd;
    struct  stat statbuf;
    _UdonParseState *p;

    if( (p=(_UdonParseState *)udon_malloc(sizeof(_UdonParseState))) == NULL)
        udon_err(UDON_MEMORY_ERR, "Couldn't allocate memory for parser state.");
    if( (fd=open(filename, O_RDONLY)) < 0)
        udon_err(UDON_FILE_OPEN_ERR, "Couldn't open file '%s'.", filename);
    if( fstat(fd, &statbuf) == -1)
        udon_err(UDON_FILE_OPEN_ERR, "Couldn't stat file '%s'.", filename);

    p->_public.source_size   = statbuf.st_size;
    p->_public.source_origin = filename;

    /* The padding is so that quickscan stuff can look in bigger chunks without overflowing */
    if( (p->_public.source_buffer = (char *) udon_malloc(p->_public.source_size+8)) == NULL)
        udon_err(UDON_MEMORY_ERR, "Couldn't allocate memory for file contents (file: '%s').",
                filename);

    if( (bytes_read = read(fd, p->_public.source_buffer, p->_public.source_size)) != p->_public.source_size)
        udon_err(UDON_FILE_READ_ERR, "Only read %zd of %zd bytes from file '%s'.",
                bytes_read, p->_public.source_size, filename);

    udon_reset_parser(p);
    close(fd);
    return p;
}

void udon_reset_parser(_UdonParseState *p) {
    p->jmpbuf_set                    = 0;
    p->_public.result                = NULL;
    p->_public.error.code            = UDON_OK;
    p->_public.error.message[0]      = 0;
    p->_public.error.parser_file     = __FILE__;
    p->_public.error.parser_line     = __LINE__;
    p->_public.error.parser_function = __FUNCTION__;
    p->_public.error.data_file       = "";
    p->_public.error.data_line       = 0;
    p->_public.error.data_column     = 0;
    p->_public.warnings              = NULL;
    p->_public._warnings__tail       = NULL;

    p->line                          = 1;
    p->column                        = 1;
    p->curr                          = p->_public.source_buffer;
    p->end                           = &(p->_public.source_buffer[p->_public.source_size - 1]);
    p->qend                          = (uint64_t *)(p->end);

    p->curr[p->_public.source_size]  = 0; /* last chance null terminator, just in case. */
}

int udon_free_parser(_UdonParseState *p) {
    udon_free(p->_public.source_buffer, p->_public.source_size);
    return 0;
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
    p->jmpbuf_set                = 1;
    if(errval) return errval;
    p->_public.result            = (void *)_udon_node__s_child_shortcut(p);
    return 0;
}


static inline UdonNode * _udon_node(_UdonParseState *p) {
    UdonNode * self_res          = _new_udon_node(p);
    uint64_t inl                 = 1;
    uint64_t ibase               = p->column;
    uint64_t ipar                = p->column-1;
    UdonNode * g;
    self_res->node_type          = UDON_NORMAL;
    s_init:
        if(_UDON_EOF) {
            self_res->node_type  = UDON_BLANK;
            return self_res;
        } else {
          _inner_s_init:
            switch(*(p->curr)) {
                case '\n':  /*-- init.value1 ---*/
                    _UDON_ADVANCE_LINE();
                    self_res->node_type = UDON_BLANK;
                    goto s_child;
                case ' ':
                case '\t':  /*-- init.value2 ---*/
                    goto s_child__lead;
                case ':':   /*-- init.attr -----*/
                    _UDON_ADVANCE_COL();
                    goto s_attribute;
                case '[':   /*-- init.id -------*/   goto s_identity__id;
                case '.':   /*-- init.class ----*/   goto s_identity__class;
                case '(':   /*-- init.delim ----*/
                    self_res->name = _udon_label__s_delim(p);
                    goto s_identity;
                case '|':   /*-- init.child ----*/   goto s_child2__node;
                case '{':   /*-- init.embed ----*/   udon_data_err("Embedded nodes are not yet supported");
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
                        UdonList * _item  = (UdonList *)(_udon_label(p));
                        UdonList * *_acc_head = (UdonList **) &(self_res->classes);
                        UdonList * *_acc_tail = (UdonList **) &(self_res->_classes__tail);
                        if(*_acc_tail == NULL) {
                            *_acc_head = *_acc_tail = _item;
                        } else {
                            (*_acc_tail)->next = _item;
                            *_acc_tail = _item;
                        }
                    }
                    goto s_identity;
                default:    /*-- identity.child */   goto _inner_s_child;
            }
        }
    s_child_shortcut:
        self_res->node_type      = UDON_ROOT;
        inl                      = 0;
        goto _inner_s_child;
    s_child:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_child:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- child.lead ----*/
                  s_child__lead:
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
                case '#':   /*-- child.bcomment */
                    _UDON_ADVANCE_COL();
                    _udon_block_comment(p);
                    goto s_child;
                default:    /*-- child.text ----*/
                    if(p->column<=ipar) return self_res;
                    goto _inner_s_child2;
            }
        }
    s_child2:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_child2:
            switch(*(p->curr)) {
                case '|':   /*-- child2.node ---*/
                  s_child2__node:
                    _UDON_ADVANCE_COL();
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
                    goto s_child;
                case ':':   /*-- child2.attribute */
                    _UDON_ADVANCE_COL();
                    goto s_attribute;
                default:    /*-- child2.value --*/
                    if(inl) {
                        {
                            UdonList * _item  = (UdonList *)(_udon_value(p));
                            if(_item && ((UdonString *)_item)->start) {
                                if(!((UdonString *)_item)->length) ((UdonString *)_item)->length = p->curr - ((UdonString *)_item)->start;
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
                        goto s_child;
                    } else {
                        {
                            UdonList * _item  = (UdonList *)(_udon_data(p));
                            if(_item && ((UdonString *)_item)->start) {
                                if(!((UdonString *)_item)->length) ((UdonString *)_item)->length = p->curr - ((UdonString *)_item)->start;
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
                        goto s_child;
                    }
            }
        }
    s_attribute:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_attribute:
            switch(*(p->curr)) {
                case '|':   /*-- attribute.grim */
                    _UDON_ADVANCE_COL();
                    g            = _udon_node(p);
                    udon_dict_add_or_update(self_res->attributes,g->name,g);
                    goto s_child;
                case '[':   /*-- attribute.grim2 */
                    g            = _udon_node(p);
                    udon_dict_add_or_update(self_res->attributes,g->name,g);
                    goto s_child;
                default:    /*-- attribute.normal */
                    udon_dict_add_or_update(self_res->attributes,_udon_label(p),_udon_value(p));
                    goto s_child;
            }
        }
    _eof:
        return self_res;
}


static inline UdonString * _udon_data(_UdonParseState *p) {
    UdonString * self_res        = _new_udon_string(p);
    self_res->start              = p->curr;
    s_main:
        _UDON_QSCAN_PAST1_NL('\n');
        self_res->length         = p->curr - self_res->start;
        return self_res;
    _eof:
        self_res->length         = p->curr - self_res->start;
        return self_res;
}


static inline UdonString * _udon_value(_UdonParseState *p) {
    UdonString * self_res        = _new_udon_string(p);
    self_res->start              = p->curr;
    s_main:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_main:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- main.space ----*/
                    self_res->length = p->curr - self_res->start;
                    _UDON_ADVANCE_COL();
                    goto s_disamb;
                case '\n':  /*-- main.done1 ----*/
                    self_res->length = p->curr - self_res->start;
                    return self_res;
                case '#':   /*-- main.comment --*/
                    self_res->length = p->curr - self_res->start;
                    _UDON_QSCAN_TO1('\n');
                    return self_res;
                default:    /*-- main.collect --*/
                  s_main__collect:
                    _UDON_ADVANCE_COL();
                    goto s_main;
            }
        }
    s_disamb:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_disamb:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- disamb.space --*/
                    _UDON_ADVANCE_COL();
                    goto s_disamb;
                case '\n':  /*-- disamb.done1 --*/
                    self_res->length = p->curr - self_res->start;
                    return self_res;
                case '#':   /*-- disamb.comment */
                    _UDON_QSCAN_TO1('\n');
                    return self_res;
                case '|':
                case '.':
                case '!':
                case ':':   /*-- disamb.done2 --*/
                    return self_res;
                default:    /*-- disamb.nevermind */   goto s_main__collect;
            }
        }
    _eof:
        self_res->length         = p->curr - self_res->start;
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
        if(_UDON_EOF) {
            udon_data_err("Unexpected end of file - missing closing ')'");
        } else {
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
                    if(lvl==0) {
                        self_res->length = p->curr - self_res->start;
                        return self_res;
                    } else {
                        goto s_delim;
                    }
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


static inline UdonString * _udon_id(_UdonParseState *p) {
    UdonString * self_res        = _new_udon_string(p);
    uint64_t lvl                 = 0;
    self_res->start              = p->curr;
    s_delim:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_delim:
            switch(*(p->curr)) {
                case '[':   /*-- delim.nest ----*/
                    _UDON_ADVANCE_COL();
                    lvl         += 1;
                    goto s_delim;
                case ']':   /*-- delim.unnest --*/
                    _UDON_ADVANCE_COL();
                    lvl         -= 1;
                    if(lvl==0) {
                        self_res->length = p->curr - self_res->start;
                        return self_res;
                    } else {
                        goto s_delim;
                    }
                case '\n':  /*-- delim.collectnl */
                    _UDON_ADVANCE_LINE();
                    goto s_delim;
                default:    /*-- delim.collect -*/
                    _UDON_ADVANCE_COL();
                    goto s_delim;
            }
        }
    _eof:
        udon_data_err("Unexpected end of file - missing closing ']'");
}


static inline void _udon_block_comment(_UdonParseState *p) {
    uint64_t ipar                = p->column - 1;
    s_main:
        _UDON_QSCAN_PAST1_NL('\n');
    s_next:
        _UDON_QSCAN_TO3('^',' ','\t');
        if(p->column<=ipar) return;
    _eof:
        return;
}


static inline UdonNode * _udon_node__s_child_shortcut(_UdonParseState *p) {
    UdonNode * self_res          = _new_udon_node(p);
    uint64_t inl                 = 1;
    uint64_t ibase               = p->column;
    uint64_t ipar                = p->column-1;
    UdonNode * g;
    self_res->node_type          = UDON_NORMAL;
    s_child_shortcut:
        self_res->node_type      = UDON_ROOT;
        inl                      = 0;
        goto _inner_s_child;
    s_child:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_child:
            switch(*(p->curr)) {
                case ' ':
                case '\t':  /*-- child.lead ----*/
                  s_child__lead:
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
                case '#':   /*-- child.bcomment */
                    _UDON_ADVANCE_COL();
                    _udon_block_comment(p);
                    goto s_child;
                default:    /*-- child.text ----*/
                    if(p->column<=ipar) return self_res;
                    goto _inner_s_child2;
            }
        }
    s_child2:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_child2:
            switch(*(p->curr)) {
                case '|':   /*-- child2.node ---*/
                  s_child2__node:
                    _UDON_ADVANCE_COL();
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
                    goto s_child;
                case ':':   /*-- child2.attribute */
                    _UDON_ADVANCE_COL();
                    goto s_attribute;
                default:    /*-- child2.value --*/
                    if(inl) {
                        {
                            UdonList * _item  = (UdonList *)(_udon_value(p));
                            if(_item && ((UdonString *)_item)->start) {
                                if(!((UdonString *)_item)->length) ((UdonString *)_item)->length = p->curr - ((UdonString *)_item)->start;
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
                        goto s_child;
                    } else {
                        {
                            UdonList * _item  = (UdonList *)(_udon_data(p));
                            if(_item && ((UdonString *)_item)->start) {
                                if(!((UdonString *)_item)->length) ((UdonString *)_item)->length = p->curr - ((UdonString *)_item)->start;
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
                        goto s_child;
                    }
            }
        }
    s_attribute:
        if(_UDON_EOF) goto _eof;
        else {
          _inner_s_attribute:
            switch(*(p->curr)) {
                case '|':   /*-- attribute.grim */
                    _UDON_ADVANCE_COL();
                    g            = _udon_node(p);
                    udon_dict_add_or_update(self_res->attributes,g->name,g);
                    goto s_child;
                case '[':   /*-- attribute.grim2 */
                    g            = _udon_node(p);
                    udon_dict_add_or_update(self_res->attributes,g->name,g);
                    goto s_child;
                default:    /*-- attribute.normal */
                    udon_dict_add_or_update(self_res->attributes,_udon_label(p),_udon_value(p));
                    goto s_child;
            }
        }
    s_init:
        if(_UDON_EOF) {
            self_res->node_type  = UDON_BLANK;
            return self_res;
        } else {
          _inner_s_init:
            switch(*(p->curr)) {
                case '\n':  /*-- init.value1 ---*/
                    _UDON_ADVANCE_LINE();
                    self_res->node_type = UDON_BLANK;
                    goto s_child;
                case ' ':
                case '\t':  /*-- init.value2 ---*/
                    goto s_child__lead;
                case ':':   /*-- init.attr -----*/
                    _UDON_ADVANCE_COL();
                    goto s_attribute;
                case '[':   /*-- init.id -------*/   goto s_identity__id;
                case '.':   /*-- init.class ----*/   goto s_identity__class;
                case '(':   /*-- init.delim ----*/
                    self_res->name = _udon_label__s_delim(p);
                    goto s_identity;
                case '|':   /*-- init.child ----*/   goto s_child2__node;
                case '{':   /*-- init.embed ----*/   udon_data_err("Embedded nodes are not yet supported");
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
                        UdonList * _item  = (UdonList *)(_udon_label(p));
                        UdonList * *_acc_head = (UdonList **) &(self_res->classes);
                        UdonList * *_acc_tail = (UdonList **) &(self_res->_classes__tail);
                        if(*_acc_tail == NULL) {
                            *_acc_head = *_acc_tail = _item;
                        } else {
                            (*_acc_tail)->next = _item;
                            *_acc_tail = _item;
                        }
                    }
                    goto s_identity;
                default:    /*-- identity.child */   goto _inner_s_child;
            }
        }
    _eof:
        return self_res;
}


static inline UdonString * _udon_label__s_delim(_UdonParseState *p) {
    UdonString * self_res        = _new_udon_string(p);
    uint64_t lvl                 = 0;
    self_res->start              = p->curr;
    s_delim:
        if(_UDON_EOF) {
            udon_data_err("Unexpected end of file - missing closing ')'");
        } else {
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
                    if(lvl==0) {
                        self_res->length = p->curr - self_res->start;
                        return self_res;
                    } else {
                        goto s_delim;
                    }
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


static inline UdonNode * _new_udon_node(_UdonParseState *p) {
    UdonNode * res               = (UdonNode *)udon_malloc(sizeof(UdonNode));
    if(!res) udon_memory_err("Memory allocation failed for Node.");
    memset(res, 0, sizeof(UdonNode));
    res->ll.listable_type        = UDON_NODE_TYPE;
    return res;
}


static inline UdonString * _new_udon_string(_UdonParseState *p) {
    UdonString * res             = (UdonString *)udon_malloc(sizeof(UdonString));
    if(!res) udon_memory_err("Memory allocation failed for STRING.");
    memset(res, 0, sizeof(UdonString));
    res->ll.listable_type        = UDON_STRING_TYPE;
    return res;
}


