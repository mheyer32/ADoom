#ifndef _C2P8_040_AMLAUKKA_H
#define _C2P8_040_AMLAUKKA_H

/*
 *  c2p8_040_amlaukka.h - optimized c2p
 *  by Aki Laukkanen <amlaukka@cc.helsinki.fi>
 *
 *  This file is public domain.
 */

#include "amiga_macros.h"
#include <graphics/gfx.h>

extern void REGARGS (*c2p8_reloc(REG(a0, UBYTE *chunky), REG(a1, struct BitMap *bitmap),
                                 REG(a6, struct ExecBase *SysBase)))(REG(a0, const UBYTE *chunky),
                                                                     REG(a1, UBYTE *raster),
                                                                     REG(a2, const UBYTE *chunky_end));

extern void REGARGS *c2p8_deinit(REG(a0, void REGARGS (*c2p)(REG(a0, const UBYTE *chunky), REG(a1, UBYTE *raster),
                                                             REG(a2, const UBYTE *chunky_end))),
                                 REG(a6, struct ExecBase *SysBase));

#endif
