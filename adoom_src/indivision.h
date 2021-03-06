#include "amiga_macros.h"

void indivision_core(REG(a0, ULONG indivisioncpldbase), REG(d2, UWORD coreadd));

void indivision_waitconfigdone(REG(a0, ULONG indivisioncpldbase));

ULONG indivision_checkcore(REG(a0, ULONG indivisionfpgabase), REG(d1, UWORD coreid));

void indivision_initscreen(REG(a0, ULONG indivisionfpgabase), REG(d0, UBYTE screenmode));

void indivision_gfxcopy(REG(a0, ULONG indivisionfpgabase), REG(a2, UBYTE *chunky_data), REG(a3, long *palette_data),
                        REG(d0, ULONG gfxaddr), REG(d1, ULONG size));
