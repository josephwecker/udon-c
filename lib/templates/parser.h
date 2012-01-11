/*---- {{parser}} parser header/api - automatically generated by genmachine ----*/
#ifndef __{{parser|up}}_H__
#define __{{parser|up}}_H__ 1
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>


/* --- Allow setting different malloc/free routines --- */
#ifdef _{{parser|up}}_NONSTANDARD_MEMORY
void *{{parser}}_calloc(size_t count, size_t size);
void *{{parser}}_malloc(size_t size);
void  {{parser}}_free(void *ptr, size_t size);
#else

#define {{parser}}_calloc(count,size) calloc(count,size)
#define {{parser}}_malloc(size)       malloc(size)
#define {{parser}}_free(ptr,size)     free(ptr)
#endif

/* --- Error values --- */
#include <sysexits.h>
#define {{parser|up}}_OK             EX_OK
#define {{parser|up}}_MEMORY_ERR     EX_OSERR
#define {{parser|up}}_FILE_OPEN_ERR  EX_NOINPUT
#define {{parser|up}}_FILE_READ_ERR  EX_IOERR
#define {{parser|up}}_DATA_ERR       EX_DATAERR

/* --- Global parser error state --- */
extern int {{parser}}_global_error;
extern char {{parser}}_global_error_msg[128];

{% if use_gmstring %}
struct {{parser|cap}}GmString {
    char *                       start;
    uint64_t                     length;
};
typedef struct {{parser|cap}}GmString      {{parser|cap}}GmString;
{% endif %}
{% if use_gmlist %}
struct {{parser|cap}}GmList {
    void *                       v;
    struct {{parser|cap}}GmList *            next;
};
typedef struct {{parser|cap}}GmList        {{parser|cap}}GmList;
{% endif %}
/* --- For hash tables --- */
{% if use_gmdict %}
struct {{parser|cap}}GmEntry {
    {{parser|cap}}GmString *key;
    void *value;
    unsigned int _used;
};
typedef struct {{parser|cap}}GmEntry {{parser|cap}}GmEntry;

struct {{parser|cap}}GmDict {
    {{parser|cap}}GmEntry *table;
    uint64_t size;
    unsigned int filled;
};
typedef struct {{parser|cap}}GmDict {{parser|cap}}GmDict;
{% endif %}

/* TODO:
 *  - Linked Lists if needed
 *  - Hash tables if needed
 *  - Strings if needed (can't imagine them not being needed)
 *  - Types / enums as per the source
 *  - Public prototypes
 *
 */

/*{% if use_gmdict %}
struct {{parser|cap}}GmDict {
    {{parser|cap}}GmList                   keys;
    {{parser|cap}}GmList                   _keys__tail;
    struct hsearch_data *        table;
    uint64_t                     size;
    uint64_t                     allocated;
};
typedef struct {{parser|cap}}GmDict        {{parser|cap}}GmDict;
{% endif %}*/

{% for e in enums %}
{{e}}

{% endfor %}{% for s in structs %}
{{s}}

{% endfor %}

/* TODO: move opaque parts into their own struct in the c file with this as a
 *       member
 */
struct {{parser|cap}}ParseState {
    // --- Set these ---
    char             *buffer;
    size_t           size;
    char             *filename;  // optional, of course
    jmp_buf          err_jmpbuf; // Use setjmp(...)

    // --- Result State ---
    void             *result;
    unsigned int     error_code;
    char             error_message[256];
    {{parser|cap}}GmList         warnings;

    // --- Current State ---
    unsigned int     state;
    uint64_t         line;
    uint64_t         column;

    // --- Generally Opaque ---
    char             *curr;
    uint64_t         *qcurr;
    char             *end;
    uint64_t         *qend;
    size_t           qsize;  // Automatically calculated, for quickscans.
    char             *alpha; // Used for accumulating
};
typedef struct {{parser|cap}}ParseState {{parser|cap}}ParseState;

{% for p in priv_protos %}
{{p}}{% endfor %}

#ifdef __cplusplus
}
#endif
#endif

