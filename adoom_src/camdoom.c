const char amigaversion[] = "$VER: CAMDoom 1.0 (16.9.2020)" /* __AMIGADATE__ */;
char __stdiowin[] = "CON:20/50/500/130/CAMDoom";
char __stdiov37[] = "/AUTO/CLOSE/WAIT";

#include <proto/camd.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/realtime.h>

#include <stabs.h>

#include "amiga_macros.h"

#include <stdio.h>
#include <stdlib.h>

extern __stdargs void __Mus_SetVol(int vol);
extern __stdargs int __Mus_Register(void *musdata);
extern __stdargs void __Mus_Unregister(int handle);
extern __stdargs void __Mus_Play(int handle, int looping);
extern __stdargs void __Mus_Stop(int handle);
extern __stdargs void __Mus_Pause(int handle);
extern __stdargs void __Mus_Resume(int handle);
extern __stdargs int __Mus_Done(int handle);

static struct Library *DoomSndBase = NULL;
static volatile int g_LastSong = 0;

static void __saveds Mus_SetVol(REGD0(int vol))
{
    __Mus_SetVol(vol);
}
static int __saveds Mus_Register(REGA0(void *musdata))
{
    g_LastSong = __Mus_Register(musdata);
    return g_LastSong;
}
static void __saveds Mus_Unregister(REGD0(int handle))
{
    __Mus_Unregister(handle);
    g_LastSong = 0;
}
static void __saveds Mus_Play(REGD0(int handle), REGD1(int looping))
{
    __Mus_Play(handle, looping);
}
static void __saveds Mus_Stop(REGD0(int handle))
{
    __Mus_Stop(handle);
}
static void __saveds Mus_Pause(REGD0(int handle))
{
    __Mus_Pause(handle);
}
static void __saveds Mus_Resume(REGD0(int handle))
{
    __Mus_Resume(handle);
}
static int __saveds Mus_Done(REGD0(int handle))
{
    return __Mus_Done(handle);
}

struct Inject
{
    LONG lvo;
    APTR new;
    APTR old;
};

static struct Inject fnTable[] = {{0x3c, &Mus_SetVol}, {0x42, &Mus_Register}, {0x48, &Mus_Unregister},
                                  {0x4e, &Mus_Play},   {0x54, &Mus_Stop},     {0x5a, &Mus_Pause},
                                  {0x60, &Mus_Resume}, {0x66, &Mus_Done}};

void shutdown(void)
{
    // FIXME: need to stop music if its still playing!
    if (DoomSndBase) {
        // Wait for the right moment to patch back
        while (TRUE) {
            Forbid();
            if (Mus_Done(g_LastSong)) {
                break;
            }
            Permit();
            Printf("Waiting for replay to end.\n");
            Flush(Output());
            Delay(50);
        }
        int numFns = sizeof(fnTable) / sizeof(fnTable[0]);
        for (int f = 0; f < numFns; ++f) {
            SetFunction(DoomSndBase, -fnTable[f].lvo, fnTable[f].old);
        }
        Permit();

        CloseLibrary(DoomSndBase);
        DoomSndBase = NULL;
    }

    if (CamdBase) {
        CloseLibrary(CamdBase);
        CamdBase = NULL;
    }
    if (RealTimeBase) {
        CloseLibrary((struct Library *)RealTimeBase);
        RealTimeBase = NULL;
    }

    Printf("Exiting.");
}

int __stdargs main(int argc, char *argv[])
{
    atexit(&shutdown);

    const char *libName = "doomsound.library";
    if (argc > 1)
    {
        libName = argv[1];
    }

    int rval = 5;
    if ((DoomSndBase = OpenLibrary(libName, 0)) == NULL) {
        Printf("Could not open '%s'.\n", (ULONG)libName);
        return rval;
    }

    int numFns = sizeof(fnTable) / sizeof(fnTable[0]);
    for (int f = 0; f < numFns; ++f) {
        fnTable[f].old = SetFunction(DoomSndBase, -fnTable[f].lvo, fnTable[f].new);
    }

    if ((CamdBase = OpenLibrary("camd.library", 0)) == NULL) {
        Printf("Could not open 'camd.library'.\n");
        return rval;
    }
    if ((RealTimeBase = (struct RealTimeBase *)OpenLibrary("realtime.library", 0)) == NULL) {
        Printf("Could not open 'realtime.library'.\n");
        return rval;
    }

    rval = 0;

    Printf("%s patched to play music via CAMD.\n Press RETURN to undo the patch and exit.", (ULONG)libName);
    Flush(Output());

    getchar();

    return rval;
}
