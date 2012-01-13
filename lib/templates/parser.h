/*---- genm parser header/api - automatically generated by genmachine ----*/
#ifndef __GENM_H__
#define __GENM_H__ 1
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
#ifdef _GENM_NONSTANDARD_MEMORY
    void *genm_calloc(size_t count, size_t size);
    void *genm_malloc(size_t size);
    void  genm_free(void *ptr, size_t size);
#else
    #define genm_calloc(count,size) calloc(count,size)
    #define genm_malloc(size)       malloc(size)
    #define genm_free(ptr,size)     free(ptr)
#endif

/* --- Error values ---
 * (Tied to sysexits standard exit values so you can exit a program with them
 * and have it be somewhat meaningful).
 */
#define GENM_OK             EX_OK
#define GENM_MEMORY_ERR     EX_OSERR
#define GENM_FILE_OPEN_ERR  EX_NOINPUT
#define GENM_FILE_READ_ERR  EX_IOERR
#define GENM_DATA_ERR       EX_DATAERR

/* --- Global parser error state ---
 * These get created in the parser implementation file as a backup. Feel free
 * to ignore in multithreaded environments etc. and use the error members of
 * the GenmParseState struct instead.
 */

struct GenmError {
    unsigned int code;
    char         message[256];

    const char * parser_file;
    uint64_t     parser_line;
    const char * parser_function;

    const char * data_file;
    uint64_t     data_line;
    uint64_t     data_column;
};
typedef struct GenmError GenmError;
extern GenmError genm_global_error;


#define genm_error_string

{% if use_gmstring %}/* --- String ---
 * Not null-terminated and by default simply a pointer into the original data,
 * so you may want to allocate a copy of it and null-terminate it depending on
 * how you want to use it.
 */
struct GenmString {
    char * start;
    uint64_t length;
};
typedef struct GenmString GenmString;
{% endif %}

{% if use_gmlist %}/* --- Linked List --- */
struct GenmList {
    void * v;
    struct GenmList * next;
};
typedef struct GenmList GenmList;
{% endif %}

{% if use_gmdict %}/* --- Dict / Hash table --- */
/* Opaque dictionary instance */
struct GenmDict;
typedef struct GenmDict GenmDict;

/* Iterating and querying dictionaries */
extern GenmList *genm_dict_keys(GenmDict *dict);
extern void * genm_dict_value_for(GenmDict *dict, GenmString *key);

/* Mostly internally used interface, exposed in case it's customized */
extern void * genm_dict_add_or_update(GenmDict *dict, GenmString *key, void *new_value);
extern GenmDict *genm_dict_create(void); /* default size */
extern GenmDict *genm_dict_create_sized(size_t size);
extern void genm_dict_destroy(GenmDict *dict);

{% endif %}

/* --- Return structure constants / enums --- */
{% for e in enums %}{{e}}

{% endfor %}

/* --- Main return structures --- */
{% for s in structs %}{{s}}

{% endfor %}

/* Opaque for internal use */
struct _GenmParseState;
typedef struct _GenmParseState _GenmParseState;

/* --- ParseState ---
 * Holds the data to be parsed and the result, along with internal state
 * information. Theoretically allows multiple parsers to run in parallel or
 * even to return in a partially parsed state.
 */
struct GenmParseState {
    /* --- Source --- */
    char *           source_buffer;
    size_t           source_size;
    char *           source_origin;  /* Filename, etc. Optional. */

    /* --- Result State --- */
    void *           result;
    GenmError        error;
    GenmList *       warnings;
};
typedef struct GenmParseState GenmParseState;

/* --- MAIN INTERFACE --- */
extern _GenmParseState  *genm_init_from_file(char *filename);
extern GenmParseState   *genm_state(_GenmParseState *p);
extern void              genm_reset_parser(_GenmParseState *p);
extern int               genm_free_parser(_GenmParseState *p);
{% for p in pub_protos %}extern {{p}}
{% endfor %}

#ifdef __cplusplus
}
#endif
#endif
