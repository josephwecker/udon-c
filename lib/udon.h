#ifndef UDON_H
#define UDON_H

#define _XOPEN_SOURCE 700
#define _REENTRANT

#include <stdint.h>
#include <sys/types.h>
#include <sysexits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>

// (for c++ inclusion)
#ifdef __cplusplus
extern "C" {
#endif

// --- Allow setting different malloc/free routines ---
#ifdef _UDON_NONSTANDARD_MEMORY
    void *udon_calloc(size_t count, size_t size);
    void *udon_malloc(size_t size);
    void  udon_free(void *ptr, size_t size);
#else

#define udon_calloc(count,size) calloc(count,size)
#define udon_malloc(size)       malloc(size)
#define udon_free(ptr,size)     free(ptr)
#endif



// --- Errors ---
    enum UdonError {
        UDON_NO_ERROR            = 0,
        UDON_OUT_OF_MEMORY_ERROR = EX_OSERR,
        UDON_BAD_FILE_ERROR      = EX_NOINPUT,
        UDON_READ_FILE_ERROR     = EX_DATAERR,
        UDON_SYSTEM_ERROR,
        UDON_GRAMMAR_ERROR
    };
    typedef enum UdonError UdonError;

    // Global error state - declared here but defined in udon.c
    extern UdonError udon_global_error;
    extern char udon_global_error_msg[128];

    static inline void udon_error(UdonError type, char *msg, ...) {
        va_list argp;
        va_start(argp, msg);
        vsnprintf(udon_global_error_msg, 128, msg, argp);
        udon_global_error = type;
    }


    void udon_emit_error(FILE *outf) {
        fprintf(outf, "%s\n", udon_global_error_msg, &udon_global_error_msg);
    }

    int udon_error_value() {
        return (int)udon_global_error;
    }



// --- Structures for holding results etc. ---

    struct UdonList {
        void             *v;
        struct UdonList  *next;
    }; typedef struct UdonList UdonList;

    enum UdonType {
        NODE,
        TEXT,
        ATTR_VALUE,
        CLASS,
        PARENT,
        COMMENT,
        FULL
    };

    struct UdonNode {
        UdonList         _base;         // Acts like a linked list
        enum UdonType    node_type;     // Node type, from enum above.
        uint64_t         source_line;   // Line where the parser encountered it.
        uint64_t         source_col;    // Column on the line where the parser encountered it.
    }; typedef struct UdonNode UdonNode;

    /*struct UdonComment {
        UdonNode         _base;         // Extends simple node
        UdonNode         *children;     // Linked list of children
    }; typedef struct UdonComment UdonComment;*/

    typedef struct _UdonAttributes {
        UdonList         *keys;         // Linked list of keys
        struct hsearch_data *hash_table;   // Actual hash table
        uint64_t         size;          // How many attributes defined so far
        uint64_t         num_allocated; // If size surpasses this, a new hash table must be created.
    } UdonAttributes;

    struct UdonFullNode {
        struct UdonNode  _base;         // Extend basic node
        UdonNode         *children;     // (same spot as UdonComment) - children linked-list
        UdonNode         *first_child;  // Points to head of children list

        char             *name;         // This is the "tag"
        char             *id;           // If specified, the ID string
        UdonList         *classes;      // Linked list of classes
        UdonList         *first_class;  // Points to head of class list
        UdonAttributes   attributes;    // See structure above
    };
    typedef struct UdonFullNode UdonFullNode;


// --- Overall state of the parser ---
    enum UdonParseState {
        UDON_NOT_STARTED,
        UDON_FINISHED_OK,
        UDON_FINISHED_WARNINGS,
        UDON_ABORTED_ERROR
    };
    typedef enum UdonParseState UdonParseState;

// --- Parser struct, mostly opaque, holding state etc. ---
    struct UdonParser {
        // --- Set these ---
        char             *buffer;
        ssize_t          size;
        char             *filename;  // optional, of course
        jmp_buf          err_jmpbuf; // Use setjmp(...)

        // --- Result State ---
        void             *result;
        UdonError        error;
        char             error_message[256];
        UdonList         warnings;

        // --- Current State ---
        UdonParseState   state;
        uint64_t         line;
        uint64_t         column;

        // --- Generally Opaque ---
        //void             *local_result;
        char             *curr;
        uint64_t         *qcurr;
        char             *end;
        uint64_t         *qend;
        ssize_t          qsize;  // Automatically calculated, for quickscans.
        char             *alpha; // Used for accumulating
    };
    typedef struct UdonParser UdonParser;


    // --- USED INTERNALLY ---
#define ADVANCE_COLUMN() p->column ++; p->curr ++;
#define ADVANCE_LINE()   p->column = 1; p->line ++; p->curr ++;

    static inline void udon_error_p(UdonParser *p, UdonError type, char *filename, int lineno, char *msg, ...) {
        va_list argp;
        va_start(argp, msg);
        // TODO: Get source line / filename for system errors, and those plus
        // udon-source line / filename for grammar errors.
        vsnprintf(udon_global_error_msg, 128, msg, argp);
        vsnprintf(p->error_message, 256, msg, argp);
        udon_global_error = type;
        p->error = type;
        p->state = UDON_ABORTED_ERROR;
    }

#define UDON_SYSERR(msg, ...) { udon_error_p(p, UDON_SYSTEM_ERROR, __FILE__, __LINE__, msg, ##__VA_ARGS__); longjmp(p->err_jmpbuf, (int)UDON_SYSTEM_ERROR); }

#define UDON_ERR(msg, ...) { udon_error_p(p, UDON_GRAMMAR_ERROR, __FILE__, __LINE__, msg, ##__VA_ARGS__); longjmp(p->err_jmpbuf, (int)UDON_GRAMMAR_ERROR); }

#define UDON_SFN ((UdonFullNode *)s)

    static inline void udon_list_append_gen(UdonParser *p, UdonList **list, UdonList **first, void *val) {
        UdonList *newtail = (UdonList *)udon_malloc(sizeof(UdonList));
        if(newtail == NULL)
            UDON_SYSERR("Couldn't allocate memory for a simple list.");
        newtail->v = val;
        udon_list_append(p, list, first, newtail);
    }

    static inline void udon_list_append(UdonParser *p, UdonList **list, UdonList **first, UdonList *newtail) {
        if((* list) == NULL) {
            (* list) = newtail;
            (* first) = newtail;
        } else {
            (* list)->next = newtail;
            (* list) = newtail;
        }
    }

#define UDON_APPEND_CHILD(newnode) udon_list_append(p, &(UDON_SFN->children), &(UDON_SFN->first_child), (UdonList *)newnode)

    // --- MAIN API INTERFACE ---
    UdonParser *udon_new_parser_from_buffer(char *buffer, ssize_t length);
    UdonParser *udon_new_parser_from_file(char *filename);
    int udon_parse(UdonParser *parser);

    // --- udon_reset_parser(UdonParser *) - assumes parser is allocated and
    // that it has a buffer and size specified at the very least. Doesn't free
    // any results or anything at the moment...
    int udon_reset_parser(UdonParser *parser);
    int udon_free_parser(UdonParser *parser);

    // --- Opaque ---
    static        void *gm__root     (UdonParser *p);
    static inline void *gm__node     (UdonParser *p, int skip_to_child_shortcut);
    static inline void *gm__protected(UdonParser *p);
    static inline void *gm__label    (UdonParser *p);

    static inline void *udon_new_value_node(UdonParser *p);
    static inline void *udon_new_full_node (UdonParser *p);

// Modified from http://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
//             really just:  (v - 0x0101...) & ~v & 0x8080...
#define q_haszero(v)    ((v) - UINT64_C(0x0101010101010101)) & ~(v) & UINT64_C(0x8080808080808080)
#define q_hasval(v,n)   (q_haszero((v) ^ (~UINT64_C(0)/255 * (n))))

// Search for one of two characters- first big chunks at a time and then get
// the actual byte. (First statement resets qcurr to whatever curr is so curr
// is authoritative).

    static inline void udon_scanto1(UdonParser *p, c1) {
        p->qcurr=(uint64_t *)(p->curr);
        while((p->qcurr <= p->qeof) && !q_hasval(*(p->qcurr),(c1)))
            p->qcurr++;
        p->curr=(char *)(p->qcurr);
        while((p->curr <= p->eof) && *(p->curr) != (c1))
            p->curr++;
    }

    /*
#define qscan2(c1,c2)   p->qcurr=(uint64_t *)(p->curr); \
                        while((p->qcurr <= qeof) && !q_hasval(*qcurr,(c1)) && !q_hasval(*qcurr,(c2)))\
                            qcurr++; \
                        curr=(char *)qcurr;     \
                        while((curr <= eof) && (*curr != (c1)) && (*curr != (c2)))\
                            curr++;

#define qscan3(c1,c2,c3) qcurr=(uint64_t *)curr; \
                        while((qcurr <= qeof) && !q_hasval(*qcurr,(c1)) && \
                                !q_hasval(*qcurr,(c2)) && !q_hasval(*qcurr,(c3)))\
                            qcurr++; \
                        curr=(char *)qcurr;     \
                        while((curr <= eof) && (*curr != (c1)) && (*curr != (c2)) && (*curr != (c3)))\
                            curr++;
*/


// (for c++ inclusion)
#ifdef __cplusplus
}
#endif

#endif
