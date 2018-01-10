#ifndef _AMIGA_MACROS_H
#define _AMIGA_MACROS_H

/*
 *  amiga_macros.h - small macros for compiler specific stuff
 *  This file is public domain.
 */

#include <exec/types.h>

/*
 * macros for function definitions and declarations
 */

#ifdef __GNUC__
#define REG(xn, parm) parm __asm(#xn)
#define REGARGS __regargs
#define STDARGS __stdargs
#define SAVEDS __saveds
#define ALIGNED __attribute__ ((aligned(4))
#define FAR __far
#define CHIP __chip
#define INTERRUPT  __interrupt //__interrupt
#define INLINE __inline__
#else /* of __GNUC__ */

#ifdef __SASC
#define REG(xn, parm) register __##xn parm
#define REGARGS __asm
#define SAVEDS __saveds
#define ALIGNED __aligned
#define STDARGS __stdargs
#define FAR __far
#define CHIP __chip
#define INLINE __inline
#else /* of __SASC */

#ifdef _DCC
#define REG(xn, parm) __##xn parm
#define REGARGS
#define SAVEDS __geta4
#define FAR __far
#define CHIP __chip
#define INLINE
#endif /* _DCC */

#endif /* __SASC */

#endif /* __GNUC__ */

#define REGD0(x) REG(d0, x)
#define REGD1(x) REG(d1, x)
#define REGD2(x) REG(d2, x)
#define REGD3(x) REG(d3, x)
#define REGD4(x) REG(d4, x)
#define REGD5(x) REG(d5, x)
#define REGD6(x) REG(d6, x)
#define REGD7(x) REG(d7, x)

#define REGA0(x) REG(a0, x)
#define REGA1(x) REG(a1, x)
#define REGA2(x) REG(a2, x)
#define REGA3(x) REG(a3, x)
#define REGA4(x) REG(a4, x)
#define REGA5(x) REG(a5, x)
#define REGA6(x) REG(a6, x)


#endif /* _AMIGA_MACROS_H */
