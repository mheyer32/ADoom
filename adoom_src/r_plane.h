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
//	Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------

#ifndef __R_PLANE__
#define __R_PLANE__

#include "doomdef.h"
#include "r_data.h"
#include "r_plane.h"

#ifdef __GNUG__
#pragma interface
#endif

// Visplane related.
extern short* lastopening;

typedef void (*planefunction_t)(int top, int bottom);

extern planefunction_t floorfunc;
extern planefunction_t ceilingfunc_t;

// extern short		floorclip[SCREENWIDTH];
// extern short		ceilingclip[SCREENWIDTH];
extern short* floorclip;
extern short* ceilingclip;

// extern FAR fixed_t	yslope[SCREENHEIGHT];
// extern FAR fixed_t	distscale[SCREENWIDTH];
extern fixed_t* yslope;
extern fixed_t* distscale;

void resinit_r_plane(void);  // called before anything else

void R_InitPlanes(void);
void R_ClearPlanes(void);

#ifdef AMIGA
//#include "amiga_macros.h"
void R_MapPlane(REGD0(int y), REGD1(int x1), REGD2(int x2));
void R_MakeSpans(REG(d2, int x), REG(d3, int t1), REG(d4, int b1), REG(d5, int t2), REG(d6, int b2));
#else
void R_MakeSpans(int x, int t1, int b1, int t2, int b2);
void R_MapPlane(int y, int x1, int x2);
#endif

void R_DrawPlanes(void);

visplane_t* R_FindPlane(fixed_t height, int picnum, int lightlevel);

visplane_t* R_CheckPlane(visplane_t* pl, int start, int stop);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
