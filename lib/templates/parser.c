/*---- {{parser}} parser implementation - automatically generated by genmachine ----*/
#define _XOPEN_SOURCE 700
#define _REENTRANT

#include "{{parser}}.h"
#include <stddef.h>
#include <stdlib.h>
{% if use_gmdict %}#include <string.h>{% endif %}


#define _{{parser|up}}_EOF p->curr == p->end

int {{parser}}_global_error = {{parser|up}}_OK;
{% if use_gmdict %}
/* TODO:
 *  - Ability to specify a different set of hash routines (specifically to use
 *    with other language bindings)
 */
static inline int is_prime(unsigned int n) {
    /* Rarely run, ok if slow */
    unsigned int div = 3;
    while(div * div < n && n % div != 0) div += 2;
    return n % div != 0;
}

static inline int gm_string_eq({{parser|cap}}GmString *a, {{parser|cap}}GmString *b) {
    if(a==NULL || b==NULL) return 0;
    if(a->length != b->length) return 0;
    return (memcmp(a->start, b->start, a->length) == 0);
}

{{parser|cap}}GmDict *gm_dict_create_standard(void) {
    {{parser|cap}}GmDict *gmd = malloc(sizeof({{parser|cap}}GmDict));
    if(!gmd) return NULL; /* TODO: longjump to error */
    gmd->size = 19;
    gmd->filled = 0;
    gmd->table = ({{parser|cap}}GmEntry *) calloc(gmd->size + 1, sizeof({{parser|cap}}GmEntry));
    if(!gmd->table) return NULL; /* TODO: err */
    return gmd;
}

{{parser|cap}}GmDict *gm_dict_create(size_t s) {
    {{parser|cap}}GmDict *gmd = malloc(sizeof({{parser|cap}}GmDict));
    if(!gmd) return NULL; /* TODO: longjump to error */
    if(s < 3) s = 3;
    s |= 1; /* ensure odd */
    while(!is_prime(s)) s += 2;
    gmd->size = s;
    gmd->filled = 0;
    gmd->table = ({{parser|cap}}GmEntry *) calloc(gmd->size + 1, sizeof({{parser|cap}}GmEntry));
    if(!gmd->table) return NULL; /* TODO: err */
    return gmd;
}

void gm_dict_destroy({{parser|cap}}GmDict *gmd) {
    if(gmd == NULL) return;
    if(gmd->table == NULL) return;
    free(gmd->table);
    gmd->table = NULL;
}

/* Returns a pointer to the old value */
/* Right now doesn't replace key w/ pointer to textually identical one. Change
 * if for some reason later we need to clean up the old key or something... */
void * gm_dict_add_or_update({{parser|cap}}GmDict *gmd, {{parser|cap}}GmString *key, void *new_value) {
    uint64_t idx;
    uint64_t hval = key->length;
    uint64_t count = hval;
    void *   retval;
    while(count-- > 0) {
        hval <<= 4;
        hval += key->start[count];
    }
    if(hval == 0) ++hval;

    idx = hval % gmd->size + 1; /* First simple hash function: modulus (except 0) */
    if(gmd->table[idx]._used) {
        if(gmd->table[idx]._used == hval && gm_string_eq(key,gmd->table[idx].key)) {
            retval = gmd->table[idx].value;
            gmd->table[idx].value = new_value;
            return retval;
        }
        uint64_t hval2 = 1 + hval % (gmd->size - 2);
        uint64_t first_idx = idx;
        do {
            if(idx <= hval2) idx = gmd->size + idx - hval2;
            else idx -= hval2;
            if(idx == first_idx) break;
            if(gmd->table[idx]._used == hval && gm_string_eq(key, gmd->table[idx].key)) {
                retval = gmd->table[idx].value;
                gmd->table[idx].value = new_value;
                return retval;
            }
        } while(gmd->table[idx]._used);
    }

    /* Not found */
    if(gmd->filled == gmd->size) {
        /* TODO: realloc (and zero out all new)*/
        return NULL;
    }
    gmd->table[idx]._used = hval;
    gmd->table[idx].key = key;
    gmd->table[idx].value = new_value;
    ++gmd->filled;
    return NULL;
}

void * gm_dict_value_for({{parser|cap}}GmDict *gmd, {{parser|cap}}GmString *key) {
    uint64_t idx;
    uint64_t hval = key->length;
    uint64_t count = hval;
    void *   retval;
    while(count-- > 0) {
        hval <<= 4;
        hval += key->start[count];
    }
    if(hval == 0) ++hval;

    idx = hval % gmd->size + 1; /* First simple hash function: modulus (except 0) */
    if(gmd->table[idx]._used) {
        if(gmd->table[idx]._used == hval && gm_string_eq(key,gmd->table[idx].key)) {
            return gmd->table[idx].value;
        }
        uint64_t hval2 = 1 + hval % (gmd->size - 2);
        uint64_t first_idx = idx;
        do {
            /* Steps through all available indices because size is prime */
            if(idx <= hval2) idx = gmd->size + idx - hval2;
            else idx -= hval2;
            if(idx == first_idx) return NULL;
            if(gmd->table[idx]._used == hval && gm_string_eq(key, gmd->table[idx].key)) {
                return gmd->table[idx].value;
            }
        } while(gmd->table[idx]._used);
    }
    return NULL;
}
{% endif %}

{% for f in functions %}
{{f}}
{% endfor %}

