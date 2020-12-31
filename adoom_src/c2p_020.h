#include "amiga_macros.h"

void c2p_8_020(REG(a2, UBYTE *fBUFFER), REG(a6, PLANEPTR *planes), REG(d0, ULONG signals1), REG(d1, ULONG signals2),
               REG(d4, ULONG signals3), REG(d2, ULONG pixels),                // width*height
               REG(d3, ULONG offset),                                         // byte offset into plane
               REG(d5, struct Task *othertask), REG(a3, UBYTE *chipbuffer));  // 2*width*height

void c2p_6_020(REG(a2, UBYTE *fBUFFER), REG(a6, PLANEPTR *planes), REG(d0, ULONG signals1), REG(d1, ULONG signals2),
               REG(d4, ULONG signals3), REG(d2, ULONG pixels),  // width*height
               REG(d3, ULONG offset),                           // byte offset into plane
               REG(a1, UBYTE *xlate), REG(d5, struct Task *othertask), REG(a3, UBYTE *chipbuffer));  // 2*width*height
