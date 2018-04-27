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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

#ifndef __M_SWAP__
#define __M_SWAP__

#ifdef __GNUG__
#pragma interface
#endif

// Endianess handling.
// WAD files are stored little endian.
#ifdef __BIG_ENDIAN__
// unsigned short	SwapSHORT(unsigned short);
// unsigned long	SwapLONG(unsigned long);
//#define SwapSHORT(x) (((x) >> 8) | ((x) << 8))
//#define SwapLONG(x) (((x) >> 24) | (((x) & 0xff0000) >> 8) | (((x) & 0xff00) << 8) | (((x) & 0xff) << 24))
//#define SWAPSHORT(x) ((short)SwapSHORT((unsigned short)(x)))
//#define SWAPLONG(x) ((long)SwapLONG((unsigned long)(x)))

static inline short SwapSHORT(short val)
{
	__asm __volatile
	(
		"ror.w	#8,%0"

		: "=d" (val)
		: "0" (val)
	);

	return val;
}

static inline long SwapLONG(long val)
{
	__asm __volatile
	(
		"ror.w	#8,%0 \n\t"
		"swap	%0 \n\t"
		"ror.w	#8,%0"

		: "=d" (val)
		: "0" (val)
	);

	return val;
}

#define SWAPSHORT(x) SwapSHORT(x)
#define SWAPLONG(x) SwapLONG(x)

#else
#define SWAPSHORT(x) (x)
#define SWAPLONG(x) (x)
#endif

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
