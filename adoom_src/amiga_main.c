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
// $Log:$
//
// DESCRIPTION:
//    Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

const char amigaversion[] = "$VER: ADoom 1.4 (8.1.2011)" /* __AMIGADATE__ */;

long __oslibversion = 38; /* we require at least OS3.0 for LoadRGB32() */
char __stdiowin[] = "CON:20/50/500/130/ADoom";
char __stdiov37[] = "/AUTO/CLOSE/WAIT";

#include <stdio.h>
#include <stdlib.h>

#include <workbench/icon.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>

#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/icon.h>

#include "doomdef.h"

#include "d_main.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fixed.h"

/**********************************************************************/
struct Library *IconBase = NULL;

int VERSION = 110;

extern int cpu_type;

int cpu_type;

/**********************************************************************/
int STDARGS main(int argc, char *argv[])
{
    struct WBStartup *argmsg;
    struct WBArg *wb_arg;
    struct DiskObject *obj;
    STRPTR *toolarray, s;
    int i, p;


    /* these command line arguments are flags */
    static char *flags[] = {
        "-inputhandler",  "-forcedemo",  "-changepitch", "-mouse",      "-joypad",    "-music",    "-nosfx",
        "-mmu",           "-fps",        "-rotatemap",   "-maponhu",    "-rtg",       "-native",   "-ehb",
        "-mousepointer",  "-sega3",      "-sega6",       "-pcchecksum", "-directcgx", "-graffiti", "-graffiti2",
        "-indivisiongfx", "-rawkey",     "-no7wire",     "-maxdemo",    "-nodraw",    "-noblit",   "-debugfile",
        "-shdev",         "-regdev",     "-comdev",      "-nomonsters", "-respawn",   "-fast",     "-devparm",
        "-altdeath",      "-deathmatch", "-cdrom",       "-playdemo",   "-avg"};
    /* these command line arguments each take a value */
    static char *settings[] = {
        "-screenmode", "-taskpriority", "-heapsize", "-cpu", "-forceversion", "-width", "-height", "-file", "-deh",
        "-waddir", "-timedemo", "-skill", "-episode", "-timer", "-statcopy", "-record", "-playdemo", "-timedemo",
        "-loadgame",
        "-config"
        "-turbo",
    };

    printf("%s\n", &amigaversion[6]);

    myargc = argc;
    if ((myargv = malloc(sizeof(char *) * MAXARGVS)) == NULL)
        I_Error("malloc(%d) failed", sizeof(char *) * MAXARGVS);
    memset(myargv, 0, sizeof(char *) * MAXARGVS);
    memcpy(myargv, argv, sizeof(char *) * myargc);

    /* parse icon tooltypes and convert them to argc/argv format */
    if (argc == 0) {
        argmsg = (struct WBStartup *)argv;
        wb_arg = argmsg->sm_ArgList;
        if ((myargv[myargc] = malloc(strlen(wb_arg->wa_Name) + 1)) == NULL)
            I_Error("malloc(%d) failed", strlen(wb_arg->wa_Name) + 1);
        strcpy(myargv[myargc++], wb_arg->wa_Name);
    }
    if ((IconBase = OpenLibrary("icon.library", 0)) == NULL)
        I_Error("Can't open icon.library");
    if ((obj = GetDiskObject(myargv[0])) != NULL) {
        toolarray = obj->do_ToolTypes;
        for (i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
            if (FindToolType(toolarray, &flags[i][1]) != NULL) {
                myargv[myargc++] = flags[i];
            }
        }
        for (i = 0; i < sizeof(settings) / sizeof(settings[0]); i++) {
            if ((s = FindToolType(toolarray, &settings[i][1])) != NULL) {
                myargv[myargc++] = settings[i];
                if ((myargv[myargc] = malloc(strlen(s) + 1)) == NULL)
                    I_Error("malloc(%d) failed", strlen(s) + 1);
                strcpy(myargv[myargc++], s);
            }
        }
        FreeDiskObject(obj);
    }

    if (argc != myargc) {
        printf("\nIcon tooltypes translated command line to:\n\n    ");
        for (i = 0; i < myargc; i++)
            printf(" %s", myargv[i]);
        printf("\n\n");
    }

    if ((SysBase->AttnFlags & AFF_68060) != 0)
        cpu_type = 68060;
    else if ((SysBase->AttnFlags & AFF_68040) != 0)
        cpu_type = 68040;
    else if ((SysBase->AttnFlags & AFF_68030) != 0)
        cpu_type = 68030;
    else if ((SysBase->AttnFlags & AFF_68020) != 0)
        cpu_type = 68020;
    else if ((SysBase->AttnFlags & AFF_68010) != 0)
        cpu_type = 68010;
    else
        cpu_type = 68000;

    p = M_CheckParm("-cpu");
    if (p && p < myargc - 1) {
        cpu_type = atoi(myargv[p + 1]);
    }

    if (cpu_type >= 68060) {
        if ((SysBase->AttnFlags & AFF_68881) != 0) {
            SetFPMode(); /* set FPU rounding mode to "trunc towards -infinity" */
            FixedMul = FixedMul_060fpu;
            FixedDiv = FixedDiv_060fpu;
        } else {
            FixedMul = FixedMul_060;
            FixedDiv = FixedDiv_040;
        }
    } else {
        FixedMul = FixedMul_040;
        FixedDiv = FixedDiv_040;
    }

    p = M_CheckParm("-forceversion");
    if (p && p < myargc - 1)
        VERSION = atoi(myargv[p + 1]);

    D_DoomMain();

    return 0;
}
