void __asm c2p_8_020(register __a2 UBYTE *fBUFFER, register __a4 PLANEPTR *planes, register __d0 ULONG signals1,
                     register __d1 ULONG signals2, register __d4 ULONG signals3,
                     register __d2 ULONG pixels,  // width*height
                     register __d3 ULONG offset,  // byte offset into plane
                     register __a5 struct Task *othertask,
                     register __a3 UBYTE *chipbuffer);  // 2*width*height

void __asm c2p_6_020(register __a2 UBYTE *fBUFFER, register __a4 PLANEPTR *planes, register __d0 ULONG signals1,
                     register __d1 ULONG signals2, register __d4 ULONG signals3,
                     register __d2 ULONG pixels,  // width*height
                     register __d3 ULONG offset,  // byte offset into plane
                     register __a1 UBYTE *xlate, register __a5 struct Task *othertask,
                     register __a3 UBYTE *chipbuffer);  // 2*width*height
