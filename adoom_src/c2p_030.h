void __asm c2p1x1_cpu3blit1_queue_init (
                      register __d0 UWORD chunkyx,  // width
                      register __d1 UWORD chunkyy,  // height
                      register __d3 UWORD scroffsy, // y offset
                      register __d5 ULONG bplsize,  // modulo to next plane
                      register __d6 ULONG signals1,
                      register __d7 ULONG signals3,
                      register __a2 struct Task *thistask,
                      register __a3 struct Task *othertask,
                      register __a4 UBYTE *chipbuff);

void __asm c2p1x1_cpu3blit1_queue (
                      register __a0 UBYTE *c2pscreen,
                      register __a1 UBYTE *bitplanes);
