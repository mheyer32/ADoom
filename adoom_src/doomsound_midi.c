/******************************************************************************/
/*                                                                            */
/* includes                                                                   */
/*                                                                            */
/******************************************************************************/
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/realtime.h>
#include <stabs.h>

#include "DoomSndFwd.h"

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
struct Library *DoomSndFwdBase = NULL;
struct Library *DoomSndBase = NULL;
struct ExecBase *SysBase = NULL;
struct DosLibrary * DOSBase = NULL;

extern struct Library *CamdBase;

extern struct Task *g_playerTask;

// The functions need to be __stdargs as this is the way the libnix library stabs
// macros expect these calling convention
extern __stdargs void __Mus_SetVol(int vol);
extern __stdargs int __Mus_Register(void *musdata);
extern __stdargs void __Mus_Unregister(int handle);
extern __stdargs void __Mus_Play(int handle, int looping);
extern __stdargs void __Mus_Stop(int handle);
extern __stdargs void __Mus_Pause(int handle);
extern __stdargs void __Mus_Resume(int handle);
extern __stdargs int __Mus_Done(int handle);

/******************************************************************************/
/*                                                                            */
/* user library initialization                                                */
/*                                                                            */
/* !!! CAUTION: This function may run in a forbidden state !!!                */
/*                                                                            */
/******************************************************************************/

static inline struct ExecBase * getSysBase(void)
{
    return SysBase;
}
#define LOCAL_SYSBASE() struct ExecBase * const SysBase = getSysBase()


__stdargs void __UserLibCleanup(void);

__stdargs int __UserLibInit(struct Library *myLib)
{
    /* setup your library base - to access library functions over *this* basePtr! */
    DoomSndBase = myLib;

    /* !!! required !!! */
    SysBase = *(struct ExecBase **)4L;

    {
        LOCAL_SYSBASE();

        if ((RealTimeBase = (struct RealTimeBase *)OpenLibrary("realtime.library", 0)) == NULL) {
            goto failure;
        }
        if ((CamdBase = OpenLibrary("camd.library", 37L)) == NULL) {
            goto failure;
        }
        if ((DoomSndFwdBase = OpenLibrary("doomsound.library", 0)) == NULL) {
            goto failure;
        }
        if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0)) == NULL) {
            goto failure;
        }
    }
    return 0;  // success!

failure:
    __UserLibCleanup();
    return -5;
}

/******************************************************************************/
/*                                                                            */
/* user library cleanup                                                       */
/*                                                                            */
/* !!! CAUTION: This function runs in a forbidden state !!!                   */
/*                                                                            */
/******************************************************************************/

// Because there are some significant restrictions on what OS functions a library
// can call from its expunge function, the __UserLibCleanup() function of a
// single base library is severely limited. Specifically, the expunge vector
// cannot break a Forbid() state! Since there are only a handful of OS functions
// that are guaranteed not to break the forbid state, very few OS functions are
// usable in the expunge function. Any function that uses the Wait() function
// breaks a forbid, so functions like

// WaitPort() are illegal. DOS I/O is illegal. In fact, the only functions that
// are legal in expunge are those which specifically mention in their autodoc
// that they do not break a forbid. These include (but are not limited to):

// AttemptSemaphore()          Disable()           FindPort()
// ReleaseSemaphore()          Enable()            FindTask()
// AllocMem()                  Signal()            AddHead()
// FreeMem()                   Cause()             AddTail()
// AllocVec()                  GetMsg()            RemHead()
// FreeVec()                   PutMsg()            RemTail()
// FindSemaphore()             ReplyMsg()          FindName()

__stdargs void __UserLibCleanup(void)
{
    LOCAL_SYSBASE();

    if (g_playerTask) {
        Signal(g_playerTask, SIGBREAKF_CTRL_C);
        g_playerTask = NULL;
    }

    if (DOSBase) {
        CloseLibrary(&DOSBase->dl_lib);
        DOSBase = NULL;
    }
    if (DoomSndFwdBase) {
        CloseLibrary(DoomSndFwdBase);
        DoomSndFwdBase = NULL;
    }
    if (CamdBase) {
        CloseLibrary(CamdBase);
        CamdBase = NULL;
    }
    if (RealTimeBase) {
        CloseLibrary(&RealTimeBase->rtb_LibNode);
        RealTimeBase = NULL;
    }
    DoomSndBase = NULL;
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

// Prefix my own library functions. When calling your own library functions, you're
// supposed to go through the unprefixed functions via library base pointer and offset,
// as other programs might have patched a lib's func table

ADDTABL_1(__Sfx_SetVol, d0); /* One Argument in d0 */
ADDTABL_6(__Sfx_Start, a0, d0, d1, d2, d3, d4);
ADDTABL_4(__Sfx_Update, d0, d1, d2, d3);
ADDTABL_1(__Sfx_Stop, d0);
ADDTABL_1(__Sfx_Done, d0);
ADDTABL_1(__Mus_SetVol, d0);
ADDTABL_1(__Mus_Register, a0);
ADDTABL_1(__Mus_Unregister, d0);
ADDTABL_2(__Mus_Play, d0, d1);
ADDTABL_1(__Mus_Stop, d0);
ADDTABL_1(__Mus_Pause, d0);
ADDTABL_1(__Mus_Resume, d0);
ADDTABL_1(__Mus_Done, d0);
ADDTABL_END();

__stdargs void __Sfx_SetVol(int vol)
{
    Fwd_Sfx_SetVol(vol);
}

__stdargs void __Sfx_Start(char *wave, int cnum, int step, int vol, int sep, int length)
{
    Fwd_Sfx_Start(wave, cnum, step, vol, sep, length);
}

__stdargs void __Sfx_Update(int cnum, int step, int vol, int sep)
{
    Fwd_Sfx_Update(cnum, step, vol, sep);
}

__stdargs void __Sfx_Stop(int cnum)
{
    Fwd_Sfx_Stop(cnum);
}

__stdargs int __Sfx_Done(int cnum)
{
    return Fwd_Sfx_Done(cnum);
}

