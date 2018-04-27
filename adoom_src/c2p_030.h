#include "amiga_macros.h"
#include <hardware/blit.h>

typedef struct c2pbltnode
{
    struct bltnode _bltnode;  // regular bltnode as first element

    UWORD pad;

    APTR task;       // ptr to task to Signal(signals1) on cleanup()
    APTR othertask;  // ptr to task to Signal(signals3) on cleanup()
    ULONG signals1;  // signals to Signal() this task at cleanup
    ULONG signals3;  // signals to Signal() othertask at cleanup

    APTR c2p_screen;
    ULONG c2p_scroffs;
    ULONG c2p_scroffs2;
    LONG c2p_bplsize;
    APTR c2p_pixels;
    APTR c2p_pixels16;
    APTR c2p_blitbuf;
    USHORT c2p_chunkyy;
} c2pbltnode_t;

void c2p1x1_cpu3blit1_queue_init(REG(d0, UWORD chunkyx),   // width
                                 REG(d1, UWORD chunkyy),   // height
                                 REG(d3, UWORD scroffsy),  // y offset
                                 REG(d5, ULONG bplsize),   // modulo to next plane
                                 REG(d6, ULONG thistask_bltdone),
                                 REG(d7, ULONG othertask_bltdone),
                                 REG(a1, struct Task *thistask),
                                 REG(a2, struct Task *othertask),
                                 REG(a3, UBYTE *chipbuff),  // intermediate chipbuffer
                                 REG(a0, c2pbltnode_t *out_c2pbltnode));

void c2p1x1_cpu3blit1_queue(REGA0(UBYTE *c2pscreen), REGA1(UBYTE *bitplanes), REGA2(c2pbltnode_t *bltnode));
