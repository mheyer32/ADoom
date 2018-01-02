void __asm c2p_8_040(register __a0 UBYTE *chunky_data, register __a1 PLANEPTR raster,
                     register __a2 UBYTE *compare_buffer, register __d1 ULONG plsiz);

void __asm c2p_6_040(register __a0 UBYTE *chunky_data, register __a1 PLANEPTR raster,
                     register __a2 UBYTE *compare_buffer, register __a4 UBYTE *xlate, register __d1 ULONG plsiz,
                     register __d2 BOOL video_palette_changed);
