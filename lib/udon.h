#ifndef UDON_H
#define UDON_H

#include <stdint.h>


#define UNPACK_STATE()       uint64_t *qcurr = state->p_quick; \
                             char     *curr  = state->p_curr;  \
                             uint64_t *qeof  = state->p_qend;  \
                             char     *eof   = state->p_end;
#define COMMIT_STATE()       state->p_quick = qcurr; state->p_curr = curr
#define discard() curr++; COMMIT_STATE();

// Modified from http://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
//             really just:  (v - 0x0101...) & ~v & 0x8080...
#define q_haszero(v)         ((v) - UINT64_C(0x0101010101010101)) & ~(v) & UINT64_C(0x8080808080808080)
#define q_hasval(v,n)        (q_haszero((v) ^ (~UINT64_C(0)/255 * (n))))

// Search for one of two characters- first big chunks at a time and then get
// the actual byte. (First statement resets qcurr to whatever curr is so curr
// is authoritative).
#define qscan2(c1,c2)        qcurr=(uint64_t *)curr; \
                             while((qcurr <= qeof) && !q_hasval(*qcurr,(c1)) && !q_hasval(*qcurr,(c2)))\
                                 qcurr++; \
                             curr=(char *)qcurr;     \
                             while((curr <= eof) && (*curr != (c1)) && (*curr != (c2)))\
                                 curr++;
#define qscan3(c1,c2,c3)     qcurr=(uint64_t *)curr; \
                             while((qcurr <= qeof) && !q_hasval(*qcurr,(c1)) && \
                                     !q_hasval(*qcurr,(c2)) && !q_hasval(*qcurr,(c3)))\
                                 qcurr++; \
                             curr=(char *)qcurr;     \
                             while((curr <= eof) && (*curr != (c1)) && (*curr != (c2)) && (*curr != (c3)))\
                                 curr++;

typedef struct {
    char      *filename;
    int        fd;
    ssize_t    size;
    ssize_t    qsize;

    char      *p_start;
    uint64_t  *p_quick;
    char      *p_curr;

    char      *p_end;
    uint64_t  *p_qend;
    // ref_start for current "word" start...
    //
    // structures for holding the results objects...
} pstate;


pstate *init_from_file(char *filename);
int     parse         (pstate *state);
void    reset_state   (pstate *state);

#endif
