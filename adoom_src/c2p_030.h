#include "amiga_macros.h"

void REGARGS SAVEDS c2p1x1_cpu3blit1_queue_init(REG(d0, UWORD chunkyx),   // width
                                                REG(d1, UWORD chunkyy),   // height
                                                REG(d3, UWORD scroffsy),  // y offset
                                                REG(d5, ULONG bplsize),   // modulo to next plane
                                                REG(d6, ULONG signals1),
                                                REG(d7, ULONG signals3),
                                                REG(a2, struct Task *thistask),
                                                REG(a3, struct Task *othertask),
                                                REG(a4, UBYTE *chipbuff));

void REGARGS c2p1x1_cpu3blit1_queue(REG(a0, UBYTE *c2pscreen),
                                    REG(a1, UBYTE *bitplanes));
