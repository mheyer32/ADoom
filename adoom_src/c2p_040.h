#include "amiga_macros.h"

void c2p_8_040(REG(a0, UBYTE *chunky_data), REG(a1, PLANEPTR raster), REG(a2, UBYTE *compare_buffer),
               REG(d1, ULONG plsiz));

void c2p_6_040(REG(a0, UBYTE *chunky_data), REG(a1, PLANEPTR raster), REG(a2, UBYTE *compare_buffer),
               REG(a4, UBYTE *xlate), REG(d1, ULONG plsiz), REG(d2, BOOL video_palette_changed));
