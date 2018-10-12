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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------

#ifndef __W_WAD__
#define __W_WAD__

#ifdef __GNUG__
#pragma interface
#endif

#include <stdio.h>
#include "doomdef.h"

//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char identification[4];
    int numlumps;
    int infotableofs;

} wadinfo_t;

typedef struct
{
    int filepos;
    int size;
    char name[8];

} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char name[8];
    FILE* handle;
    int position;
    int size;
} lumpinfo_t;

extern void** lumpcache;
extern lumpinfo_t* lumpinfo;
extern int numlumps;

void W_InitMultipleFiles(const char* const * filenames);
void W_Reload(void);

int W_CheckNumForName(const char* name);
int W_GetNumForName(const char* name);

int W_LumpLength(REGD0(int lump));
void W_ReadLump(REGD0(int lump), REGA0(void* dest));

void* W_CacheLumpNum(REGD0(int lump), REGD1(int tag));
void* W_CacheLumpName(const char* name, int tag);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
