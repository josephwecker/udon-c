/* Generated !{datetime} by GenMachine */
#ifndef __{{parser|up}}_H__
#define __{{parser|up}}_H__
#ifndef __cplusplus
//extern "C" {
#endif

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

/* TODO:
 *  - Linked Lists if needed
 *  - Hash tables if needed
 *  - Strings if needed (can't imagine them not being needed)
 *  - Types / enums as per the source
 *  - Public prototypes
 *
 */

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
    struct {{parser|cap}}List *            next;
};
typedef struct {{parser|cap}}GmList        {{parser|cap}}GmList;
{% endif %}
{% if use_gmdict %}
struct {{parser|cap}}GmDict {
    {{parser|cap}}GmList                   keys;
    {{parser|cap}}GmList                   _keys__tail;
    struct hsearch_data *        table;
    uint64_t                     size;
    uint64_t                     allocated;
};
typedef struct {{parser|cap}}GmDict        {{parser|cap}}GmDict;
{% endif %}

{% for e in enums %}
{{e}}

{% endfor %}{% for s in structs %}
{{s}}

{% endfor %}


#ifdef __cplusplus
}
#endif
#endif
