// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------

#ifndef __M_FIXED__
#define __M_FIXED__

#ifdef __GNUG__
#pragma interface
#endif

#include "doomdef.h"

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)

typedef int fixed_t;

#ifdef AMIGA
typedef fixed_t (*FixedMulFunction)(REGD0(fixed_t a), REGD1(fixed_t b));
typedef fixed_t (*FixedDivFunction)(REGD0(fixed_t a), REGD1(fixed_t b));

extern FixedMulFunction FixedMul;
extern FixedDivFunction FixedDiv;

extern void SetFPMode(void);
extern fixed_t  FixedMul_040(REGD0(fixed_t a), REGD1(fixed_t b));
extern fixed_t  FixedMul_060fpu(REGD0(fixed_t a), REGD1(fixed_t b));
extern fixed_t  FixedMul_060(REGD0(fixed_t a), REGD1(fixed_t b));
extern fixed_t  FixedDiv_040(REGD0(fixed_t a), REGD1(fixed_t b));
extern fixed_t  FixedDiv_060fpu(REGD0(fixed_t a), REGD1(fixed_t b));

static __inline__ int ULongDiv(int eins,int zwei)
{
	__asm __volatile
	(
		"divul.l %2,%0:%0\n\t"

		: "=d" (eins)
		: "0" (eins), "d" (zwei)
	);

	return eins;
}
#else
fixed_t FixedMul(fixed_t a, fixed_t b);
fixed_t FixedDiv(fixed_t a, fixed_t b);
fixed_t FixedDiv2(fixed_t a, fixed_t b);
#endif

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
