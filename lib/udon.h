/*---- udon parser header/api - automatically generated by udonachine ----*/
#ifndef __UDON_H__
#define __UDON_H__ 1
#ifdef __cplusplus
extern "C" {
#endif
#include <sysexits.h>
#include <stdint.h>
#include <stddef.h>


/* --- Memory Management ---
 * Allows you to set these to something else if you need to use custom memory
 * allocators etc. (for example, when doing a language binding).
 */
#ifdef _UDON_NONSTANDARD_MEMORY
    void *udon_calloc(size_t count, size_t size);
    void *udon_malloc(size_t size);
    void  udon_free(void *ptr, size_t size);
#else
    #define udon_calloc(count,size) calloc(count,size)
    #define udon_malloc(size)       malloc(size)
    #define udon_free(ptr,size)     free(ptr)
#endif

/* --- Error values ---
 * (Tied to sysexits standard exit values so you can exit a program with them
 * and have it be somewhat meaningful).
 */
#define UDON_OK             EX_OK
#define UDON_MEMORY_ERR     EX_OSERR
#define UDON_FILE_OPEN_ERR  EX_NOINPUT
#define UDON_FILE_READ_ERR  EX_IOERR
#define UDON_DATA_ERR       EX_DATAERR


enum UdonListableTypes {
    UDON_LIST_TYPE,
    UDON_STRING_TYPE,
    UDON_NODE_TYPE,
    UDON_DATA_TYPE,
    
};

// TODO: YOU ARE HERE: allow random structs to be link-listed together - give
// them a byte at the top of the struct that indicates the type. Allow strings
// to be link-listed- (so instead of v * in the linked list, it's just the
// string fields - get rid of v for all linked stuff).

/* --- Global parser error state ---
 * These get created in the parser implementation file as a backup. Feel free
 * to ignore in multithreaded environments etc. and use the error members of
 * the UdonParseState struct instead.
 */

struct UdonError {
    unsigned int code;
    char         message[256];

    const char * parser_file;
    uint64_t     parser_line;
    const char * parser_function;

    const char * data_file;
    uint64_t     data_line;
    uint64_t     data_column;
};
typedef struct UdonError UdonError;
extern UdonError udon_global_error;


/* --- Linked List base --- */
struct UdonList {
    enum UdonListableTypes listable_type;
    struct UdonList * next;
};
typedef struct UdonList UdonList;

/* --- String ---
 * Not null-terminated and by default simply a pointer into the original data,
 * so you may want to allocate a copy of it and null-terminate it depending on
 * how you want to use it.
 */
struct UdonString {
    UdonList ll;
    char *   start;
    uint64_t length;
};
typedef struct UdonString UdonString;


/* --- Dict / Hash table --- */
/* Opaque dictionary instance */
struct UdonDict;
typedef struct UdonDict UdonDict;

/* Iterating and querying dictionaries */
extern UdonList *udon_dict_keys(UdonDict *dict);
extern void * udon_dict_value_for(UdonDict *dict, UdonString *key);

/* Mostly internally used interface, exposed in case it's customized */
extern void * udon_dict_add_or_update(UdonDict *dict, UdonString *key, void *new_value);
extern UdonDict *udon_dict_create(void); /* default size */
extern UdonDict *udon_dict_create_sized(size_t size);
extern void udon_dict_destroy(UdonDict *dict);



/* --- Return structure constants / enums --- */
enum UdonNodeType {
    UDON_ROOT,
    UDON_BLANK,
    UDON_NORMAL
};
typedef enum UdonNodeType        UdonNodeType;




/* --- Main return structures --- */
struct UdonNode {
    UdonList                     ll;

    UdonNodeType                 node_type;
    uint64_t                     source_line;
    uint64_t                     source_column;
    UdonString *                 name;
    UdonString *                 _name__tail;
    UdonString *                 id;
    UdonString *                 _id__tail;
    UdonList *                   classes;
    UdonList *                   _classes__tail;
    UdonDict *                   attributes;
    struct UdonNode *            children;
    struct UdonNode *            _children__tail;
};
typedef struct UdonNode          UdonNode;


struct UdonData {
    UdonList                     ll;

    UdonList *                   lines;
    UdonList *                   _lines__tail;
};
typedef struct UdonData          UdonData;




/* Opaque for internal use */
struct _UdonParseState;
typedef struct _UdonParseState _UdonParseState;

/* --- ParseState ---
 * Holds the data to be parsed and the result, along with internal state
 * information. Theoretically allows multiple parsers to run in parallel or
 * even to return in a partially parsed state.
 */
struct UdonParseState {
    /* --- Source --- */
    char *           source_buffer;
    size_t           source_size;
    char *           source_origin;  /* Filename, etc. Optional. */

    /* --- Result State --- */
    void *           result;
    UdonError        error;
    UdonString *     warnings;
    UdonString *     _warnings__tail; /* Points to last warning in linked list */
};
typedef struct UdonParseState UdonParseState;

/* --- MAIN INTERFACE --- */
extern _UdonParseState  *udon_init_from_file(char *filename);
extern UdonParseState   *udon_state(_UdonParseState *p);
extern void              udon_reset_parser(_UdonParseState *p);
extern int               udon_free_parser(_UdonParseState *p);
extern int                          udon_parse(_UdonParseState *p);


#ifdef __cplusplus
}
#endif
#endif
