

static inline void *node(pstate *parse_state) {
    UNPACK_STATE();
    void *s;
    int _udon_inline = 0;
    int _udon_ibase = col;
    int _udon_ipar = col - 1;

s_initialize:
    switch(*curr) {
        case ' ':
        case '\n':
        case '\t':
            curr++;
            PACK_STATE();
            return(value_node(parse_state));
        case ':':
            curr++;
            s = new_udon_full_node(); // Macro to malloc...
            goto s_attribute;
        case '.':
            s = new_udon_full_node();
            goto _s_identity__class;
        case '(':
            s = new_udon_full_node();
            PACK_STATE();
            s->name = protected(parse_state);
            goto s_identity;
        case '|':
            s = new_udon_full_node();
            
    }
}



// Packing and unpacking state
// Ordering the conditionals for speed
// Normal conditionals mixed in with case statements...
//
// Cleaner syntax still: probabilities, substates, new objects
