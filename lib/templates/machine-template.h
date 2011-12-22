#ifndef __{{prefix|upcase}}_PARSER_H__
#define __{{prefix|upcase}}_PARSER_H__

#define _REENTRANT
#define _XOPEN_SOURCE 700

/*------------------ Memory management ----------------------------------------
 * Define _{{prefix|upcase}}_NONSTANDARD_MEMORY to use different malloc/free
 * implementations.
 */
#ifdef _{{prefix|upcase}}_NONSTANDARD_MEMORY
#ifdef __cplusplus
extern "C" {
#endif
  void *{{prefix}}_calloc(size_t count, size_t size);
  void *{{prefix}}_malloc(size_t size);
  void  {{prefix}}_free(void *ptr, size_t size);
#ifdef __cplusplus
}
#endif
#else

#define {{prefix}}_calloc(count,size) calloc(count,size)
#define {{prefix}}_malloc(size)       malloc(size)
#define {{prefix}}_free(ptr,size)     free(ptr)

#endif

/*------------------ Parser State ---------------------------------------------
 * Opaque struct to keep track of parsing state.
 */
typedef struct _{{prefix | capitalize}}ParseState {
    char                      *filename;
    int                       fd;
    ssize_t                   size;
    ssize_t                   qsize;

    char                      *p_start;
    uint64_t                  *p_quick;
    char                      *p_curr;

    char                      *p_end;
    uint64_t                  *p_qend;

    {{nodes[:root]}}          *root;
} {{prefix | capitalize}}ParseState;

#endif
