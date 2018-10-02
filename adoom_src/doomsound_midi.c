/******************************************************************************/
/*                                                                            */
/* includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/realtime.h>
#include <stabs.h>

#include "camd.h"

#define ADDTABL_6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    _ADDTABL_START(name);                                   \
    _ADDTABL_ARG(arg6);                                     \
    _ADDTABL_ARG(arg5);                                     \
    _ADDTABL_ARG(arg4);                                     \
    _ADDTABL_ARG(arg3);                                     \
    _ADDTABL_ARG(arg2);                                     \
    _ADDTABL_ARG(arg1);                                     \
    _ADDTABL_CALL(name);                                    \
    _ADDTABL_ENDN(name, 6)

/******************************************************************************/
/*                                                                            */
/* exports                                                                    */
/*                                                                            */
/******************************************************************************/

const char LibName[] = "doomsound_midi.library";
const char LibIdString[] = "version 0.1";

const UWORD LibVersion = 0;
const UWORD LibRevision = 1;

/******************************************************************************/
/*                                                                            */
/* global declarations                                                        */
/*                                                                            */
/******************************************************************************/

struct Library *myLibPtr = NULL;
struct ExecBase *SysBase = NULL;
struct Library *CamdBase = NULL;
struct Library *DoomSndBase = NULL;
struct RealTimeBase *RealTimeBase = NULL;

/******************************************************************************/
/*                                                                            */
/* user library initialization                                                */
/*                                                                            */
/* !!! CAUTION: This function may run in a forbidden state !!!                */
/*                                                                            */
/******************************************************************************/

int __saveds __UserLibInit(struct Library *myLib)
{
    /* setup your library base - to access library functions over *this* basePtr! */

    myLibPtr = myLib;

    /* !!! required !!! */
    SysBase = *(struct ExecBase **)4L;
    if ((RealTimeBase = (struct RealTimeBase *)OpenLibrary("realtime.library", 0)) == NULL) {
        goto failure;
    }
    if ((CamdBase = OpenLibrary("camd.library", 37L)) == NULL) {
        goto failure;
    }
    if ((DoomSndBase = OpenLibrary("doomsound.library", 0)) == NULL) {
        goto failure;
    }

failure:
    if (DoomSndBase) {
        CloseLibrary(DoomSndBase);
        DoomSndBase = NULL;
    }
    if (CamdBase) {
        CloseLibrary(CamdBase);
        CamdBase = NULL;
    }
    if (RealTimeBase) {
        CloseLibrary(&RealTimeBase->rtb_LibNode);
        RealTimeBase = NULL;
    }
    return 5;
}

/******************************************************************************/
/*                                                                            */
/* user library cleanup                                                       */
/*                                                                            */
/* !!! CAUTION: This function runs in a forbidden state !!!                   */
/*                                                                            */
/******************************************************************************/

void __saveds __UserLibCleanup(void)
{
    if (DoomSndBase) {
        CloseLibrary(DoomSndBase);
        DoomSndBase = NULL;
    }
    if (CamdBase) {
        CloseLibrary(CamdBase);
        CamdBase = NULL;
    }
    if (RealTimeBase) {
        CloseLibrary(&RealTimeBase->rtb_LibNode);
        RealTimeBase = NULL;
    }
}

/******************************************************************************/
/*                                                                            */
/* library dependent function(s)                                              */
/*                                                                            */
/******************************************************************************/
// Each of the AddTable macros creates a new table entry that puts the calling
// registers onto the stack, restores the base pointer into a4 and eventually
// calls the actual C function.
// FIXME: find a way to get around the stack push/pop business and use regparms

ADDTABL_1(Sfx_SetVol, d0); /* One Argument in d0 */
__stdargs __saveds void Sfx_SetVol(int vol) {

}

ADDTABL_6(Sfx_Start, a0, d0, d1, d2, d3, d4);
__stdargs __saveds void Sfx_Start(char *wave, int cnum, int step, int vol, int sep, int length) {}

ADDTABL_4(Sfx_Update, d0, d1, d2, d3);
__stdargs __saveds void Sfx_Update(int cnum, int step, int vol, int sep) {}

ADDTABL_1(Sfx_Stop, d0);
__stdargs __saveds void Sfx_Stop(int cnum) {}

ADDTABL_1(Sfx_Done, d0);
__stdargs __saveds int Sfx_Done(int cnum) {}

ADDTABL_1(Mus_SetVol, d0);
__stdargs __saveds void Mus_SetVol(int vol) {}

ADDTABL_1(Mus_Register, a0);
__stdargs __saveds int Mus_Register(void *musdata) {}

ADDTABL_1(Mus_Unregister, d0);
__stdargs __saveds void Mus_Unregister(int handle) {}

ADDTABL_2(Mus_Play, d0, d1);
__stdargs __saveds void Mus_Play(int handle, int looping) {}

ADDTABL_1(Mus_Stop, d0);
__stdargs __saveds void Mus_Stop(int handle) {}

ADDTABL_1(Mus_Pause, d0);
__stdargs __saveds void Mus_Pause(int handle) {}

ADDTABL_1(Mus_Resume, d0);
__stdargs __saveds void Mus_Resume(int handle) {}

ADDTABL_1(Mus_Done, d0);
__stdargs __saveds int Mus_Done(int handle) {}

/******************************************************************************/
/*                                                                            */
/* endtable marker (required!)                                                */
/*                                                                            */
/******************************************************************************/

ADDTABL_END();

/******************************************************************************/
/*                                                                            */
/* end of simplelib.c                                                         */
/*                                                                            */
/******************************************************************************/
