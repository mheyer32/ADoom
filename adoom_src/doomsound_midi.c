/******************************************************************************/
/*                                                                            */
/* includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/realtime.h>
#include <stabs.h>

#include "doomtype.h"

#include "DoomSnd.h"
#include "DoomSndFwd.h"
#include "camd.h"
#include "m_swap.h"

#include <assert.h>
#include <string.h>
//#include <dos.h>

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

struct Library *DoomSndBase = NULL;
struct ExecBase *SysBase = NULL;
struct Library *CamdBase = NULL;
struct Library *DoomSndFwdBase = NULL;
struct RealTimeBase *RealTimeBase = NULL;
struct DosLibrary *DOSBase = NULL;

#define NUM_CHANNELS 16

#define MIDI_PERCUSSION_CHAN 9
#define MUS_PERCUSSION_CHAN 15

// MUS event codes
typedef enum
{
    mus_releasekey = 0x00,
    mus_presskey = 0x10,
    mus_pitchwheel = 0x20,
    mus_systemevent = 0x30,
    mus_changecontroller = 0x40,
    mus_scoreend = 0x60
} musevent;

// MIDI event codes
typedef enum
{
    midi_releasekey = 0x80,
    midi_presskey = 0x90,
    midi_aftertouchkey = 0xA0,
    midi_changecontroller = 0xB0,
    midi_changepatch = 0xC0,
    midi_aftertouchchannel = 0xD0,
    midi_pitchwheel = 0xE0
} midievent;

// Structure to hold MUS file header
typedef struct
{
    byte id[4];
    unsigned short scorelength;
    unsigned short scorestart;
    unsigned short primarychannels;
    unsigned short secondarychannels;
    unsigned short instrumentcount;
} MusHeader;

// Cached channel velocities
static byte channelvelocities[] = {127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127};

// Timestamps between sequences of MUS events

static unsigned int queuedtime = 0;

// Counter for the length of the track

static unsigned int tracksize;

static const byte controller_map[] = {0x00, 0x20, 0x01, 0x07, 0x0A, 0x0B, 0x5B, 0x5D,
                                      0x40, 0x43, 0x78, 0x7B, 0x7E, 0x7F, 0x79};

static int channel_map[NUM_CHANNELS];

// This would require a multi-base library, since the mainTask would differ per applcication
static ULONG midiTics = 0;
static struct Task *mainTask = NULL;
static struct Task *playerTask = NULL;
static struct Player *player = NULL;
static struct MidiNode *midiNode = NULL;
static struct MidiLink *midiLink = NULL;
static BYTE playerSignalBit = -1;

typedef enum
{
    STOPPED,
    PAUSE,
    PLAYING
} SongState;

typedef union {
    const byte *b;
    const short *s;
    const int *i;
} MusData;

typedef struct
{
    MusHeader header;
    MusData dataPtr;
    MusData currentPtr;
    SongState state;
    boolean looping;
    boolean done;
} Song;

static Song *currentSong = NULL;

const char *FindMidiDevice(void)
{
    static char _outport[128] = "";
    char *retname = NULL;

    APTR key = LockCAMD(CD_Linkages);
    if (key != NULL) {
        struct MidiCluster *cluster = NextCluster(NULL);

        while (cluster && !retname) {
            // Get the current cluster name
            char *dev = cluster->mcl_Node.ln_Name;

            if (strstr(dev, "out") != NULL) {
                // This is an output device, return this
                strncpy(_outport, dev, sizeof(_outport));
                retname = _outport;
            } else {
                // Search the next one
                cluster = NextCluster(cluster);
            }
        }

        // If the user has a preference outport set, use this instead
        if (GetVar("DefMidiOut", _outport, sizeof(_outport), 0)) {
            retname = _outport;
        }

        UnlockCAMD(key);
    }

    return retname;
}

void ShutDownMidiTask(void)
{
    if (player) {
        SetConductorState(player, CONDSTATE_STOPPED, 0);
        DeletePlayer(player);
        player = NULL;
    }
    if (playerSignalBit != -1) {
        FreeSignal(playerSignalBit);
    }
    if (midiNode) {
        FlushMidi(midiNode);
        if (midiLink) {
            RemoveMidiLink(midiLink);
            midiLink = NULL;
        }
        DeleteMidi(midiNode);
        midiNode = NULL;
    }
}

boolean InitMidiTask(void)
{
    midiNode = CreateMidi(MIDI_MsgQueue, 0L, MIDI_SysExSize, 0, MIDI_Name, (Tag) "DOOM Midi Out", TAG_END);
    if (!midiNode) {
        goto failure;
    }

    const char *deviceName = FindMidiDevice();
    if (!deviceName) {
        goto failure;
    }

    midiLink = AddMidiLink(midiNode, MLTYPE_Sender, MLINK_Location, (Tag)deviceName /*"out.0"*/, TAG_END);
    if (!midiLink) {
        goto failure;
    }

    // technically, the Task is supposed to open its own library bases...
    struct Task *playerTask = FindTask(NULL);

    if ((playerSignalBit = AllocSignal(-1)) == -1) {
        goto failure;
    }

    ULONG err = 0;
    player =
        CreatePlayer(PLAYER_Name, (Tag) "DOOM Player", PLAYER_Conductor, (Tag) "DOOM Conductor", PLAYER_AlarmSigTask,
                     (Tag)playerTask, PLAYER_AlarmSigBit, (Tag)playerSignalBit, PLAYER_ErrorCode, (Tag)&err, TAG_END);
    if (!player) {
        goto failure;
    }

    return TRUE;

failure:
    ShutDownMidiTask();
    return FALSE;
}

void RestoreA4(void)
{
    __asm volatile("\tlea ___a4_init, a4");
}

void MidiPlayerTask(void)
{
    RestoreA4();

    if (!InitMidiTask()) {
        Signal(mainTask, SIGBREAKF_CTRL_C);
        return;
    }

    //    struct Library *myDoomSndBase = OpenLibrary("doomsound.library", 0);

    Signal(mainTask, SIGBREAKF_CTRL_E);

    const ULONG signalBitMask = (1UL << playerSignalBit);
    const ULONG signalMask = signalBitMask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E;

    while (true) {
        ULONG signals = Wait(signalMask);
        if (signals & SIGBREAKF_CTRL_C) {
            break;
        }
        if (signals & SIGBREAKF_CTRL_E) {
            // acknowledge we're stopping
            //            SetSignal(0, signalBitMask);
            //            signals &= ~signalBitMask;
            Signal(mainTask, SIGBREAKF_CTRL_E);

            // wait for maintask to do its thing
            Wait(SIGBREAKF_CTRL_E);
        }
        if (signals & signalBitMask) {
            // PlayNextNote()

            // Rearm timer
            // FIXME: we could get fancy here and adjust the tic to the next event
            // based on the wallclock time that has passed
            //            LONG res =
            //                SetPlayerAttrs(player, PLAYER_AlarmTime, player->pl_AlarmTime + midiTics, PLAYER_Ready,
            //                TRUE, TAG_END);
        }
    };

    ShutDownMidiTask();
    //    CloseLibrary(myDoomSndBase);

    Signal(mainTask, SIGBREAKF_CTRL_E);
}

void HaltTask(void)
{
    if (playerTask) {
        Signal(playerTask, SIGBREAKF_CTRL_E);
        Wait(SIGBREAKF_CTRL_E);
    }
}
void ResumeTask(void)
{
    if (playerTask) {
        Signal(playerTask, SIGBREAKF_CTRL_E);
    }
}

/******************************************************************************/
/*                                                                            */
/* user library initialization                                                */
/*                                                                            */
/* !!! CAUTION: This function may run in a forbidden state !!!                */
/*                                                                            */
/******************************************************************************/

__stdargs int __UserLibInit(struct Library *myLib)
{
    /* setup your library base - to access library functions over *this* basePtr! */
    DoomSndBase = myLib;

    /* !!! required !!! */
    SysBase = *(struct ExecBase **)4L;
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

    return 1;  // success!

failure:
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
    return 0;
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
    if (playerTask) {
        Signal(playerTask, SIGBREAKF_CTRL_C);
        playerTask = NULL;
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

__stdargs void __Mus_SetVol(int vol) {}

boolean __stdargs ReadMusHeader(MusData *musdata, MusHeader *header)
{
    static const char magic[5] = "MUS\x1A";

    memcpy(header->id, musdata->i++, 4);
    header->scorelength = SWAPSHORT(*(musdata->s++));
    header->scorestart = SWAPSHORT(*(musdata->s++));
    header->primarychannels = SWAPSHORT(*(musdata->s++));
    header->secondarychannels = SWAPSHORT(*(musdata->s++));
    header->instrumentcount = SWAPSHORT(*(musdata->s++));

    return !memcmp(header->id, magic, 4);
}

__stdargs int __Mus_Register(void *musdata)
{
    if (!playerTask) {
        mainTask = FindTask(NULL);

        if ((playerTask = (struct Task *)CreateNewProcTags(NP_Name, (Tag) "DOOM Midi", NP_Priority, 10, NP_Entry,
                                                           (Tag)MidiPlayerTask, NP_StackSize, 64000, TAG_END)) ==
            NULL) {
            return 0;
        }

        ULONG signal = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E);
        if (!(signal & SIGBREAKF_CTRL_E)) {
            // Task failed to start or allocate resources. It'll signal SIGBREAKF_CTRL_E and exit
            playerTask = 0;
            return -1;
        }
    }

    Song *song = (Song *)AllocVec(sizeof(Song), MEMF_ANY | MEMF_CLEAR);
    if (!song) {
        return 0;
    }

    song->dataPtr.b = (const byte *)musdata;
    song->currentPtr.b = song->dataPtr.b;
    if (!ReadMusHeader(&song->currentPtr, &song->header)) {
        FreeVec(song);
        return 0;
    }
    //    Mus_Resume(1);
    //    Mus_Stop(1);
    //    Mus_Unregister(1);

    //    HaltTask();
    //    currentMus = (const MusHeader*)musdata;
    //    currentMusData = (const musevent*)((const byte*)currentMus + currentMus->scorestart);

    return (int)song;
    //    return 0;
}

__stdargs void __Mus_Unregister(int handle)
{
    Song *song = (Song *)handle;
    if (playerTask) {
        HaltTask();
        if (currentSong == song) {
            currentSong = NULL;
        }
        ResumeTask();

        // kill player, because we only allow one registered music at any time
        Signal(playerTask, SIGBREAKF_CTRL_C);
        Wait(SIGBREAKF_CTRL_E);
        playerTask = NULL;
    }
    if (song) {
        FreeVec(song);
    }
}

__stdargs void __Mus_Play(int handle, int looping)
{
    Song *song = (Song *)handle;

    HaltTask();
    song->state = PLAYING;
    currentSong = song;
    ResumeTask();
}

__stdargs void __Mus_Stop(int handle)
{
    Song *song = (Song *)handle;

    HaltTask();
    song->state = STOPPED;
    currentSong = NULL;
    ResumeTask();
}

__stdargs void __Mus_Pause(int handle)
{
    Song *song = (Song *)handle;

    HaltTask();
    song->state = PAUSE;
    ResumeTask();
}

__stdargs void __Mus_Resume(int handle)
{
    Song *song = (Song *)handle;

    HaltTask();
    song->state = PAUSE;
    ResumeTask();
}

__stdargs int __Mus_Done(int handle)
{
    Song *song = (Song *)handle;
    return song->done;
}

/******************************************************************************/
/*                                                                            */
/* endtable marker (required!)                                                */
/*                                                                            */
/******************************************************************************/

// Write timestamp to a MIDI file.

// static boolean WriteTime(unsigned int time, const byte *midioutput)
//{
//    unsigned int buffer = time & 0x7F;
//    byte writeval;

//    while ((time >>= 7) != 0) {
//        buffer <<= 8;
//        buffer |= ((time & 0x7F) | 0x80);
//    }

//    for (;;) {
//        writeval = (byte)(buffer & 0xFF);

//        if (mem_fwrite(&writeval, 1, 1, midioutput) != 1) {
//            return true;
//        }

//        ++tracksize;

//        if ((buffer & 0x80) != 0) {
//            buffer >>= 8;
//        } else {
//            queuedtime = 0;
//            return false;
//        }
//    }
//}

//// Write the end of track marker
// static boolean WriteEndTrack(const byte *midioutput)
//{
//    byte endtrack[] = {0xFF, 0x2F, 0x00};

//    if (WriteTime(queuedtime, midioutput)) {
//        return true;
//    }

//    if (mem_fwrite(endtrack, 1, 3, midioutput) != 3) {
//        return true;
//    }

//    tracksize += 3;
//    return false;
//}

//// Write a key press event
// static boolean WritePressKey(byte channel, byte key, byte velocity, const byte *midioutput)
//{
//    byte working = midi_presskey | channel;

//    if (WriteTime(queuedtime, midioutput)) {
//        return true;
//    }

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = key & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = velocity & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    tracksize += 3;

//    return false;
//}

//// Write a key release event
// static boolean WriteReleaseKey(byte channel, byte key, const byte *midioutput)
//{
//    byte working = midi_releasekey | channel;

//    if (WriteTime(queuedtime, midioutput)) {
//        return true;
//    }

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = key & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = 0;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    tracksize += 3;

//    return false;
//}

//// Write a pitch wheel/bend event
// static boolean WritePitchWheel(byte channel, short wheel, const byte *midioutput)
//{
//    byte working = midi_pitchwheel | channel;

//    if (WriteTime(queuedtime, midioutput)) {
//        return true;
//    }

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = wheel & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = (wheel >> 7) & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    tracksize += 3;
//    return false;
//}

//// Write a patch change event
// static boolean WriteChangePatch(byte channel, byte patch, const byte *midioutput)
//{
//    byte working = midi_changepatch | channel;

//    if (WriteTime(queuedtime, midioutput)) {
//        return true;
//    }

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = patch & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    tracksize += 2;

//    return false;
//}

//// Write a valued controller change event

// static boolean WriteChangeController_Valued(byte channel, byte control, byte value, const byte *midioutput)
//{
//    byte working = midi_changecontroller | channel;

//    if (WriteTime(queuedtime, midioutput)) {
//        return true;
//    }

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    working = control & 0x7F;

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    // Quirk in vanilla DOOM? MUS controller values should be
//    // 7-bit, not 8-bit.

//    working = value;  // & 0x7F;

//    // Fix on said quirk to stop MIDI players from complaining that
//    // the value is out of range:

//    if (working & 0x80) {
//        working = 0x7F;
//    }

//    if (mem_fwrite(&working, 1, 1, midioutput) != 1) {
//        return true;
//    }

//    tracksize += 3;

//    return false;
//}

//// Write a valueless controller change event
// static boolean WriteChangeController_Valueless(byte channel, byte control, const byte *midioutput)
//{
//    return WriteChangeController_Valued(channel, control, 0, midioutput);
//}

//// Allocate a free MIDI channel.

// static int AllocateMIDIChannel(void)
//{
//    int result;
//    int max;
//    int i;

//    // Find the current highest-allocated channel.

//    max = -1;

//    for (i = 0; i < NUM_CHANNELS; ++i) {
//        if (channel_map[i] > max) {
//            max = channel_map[i];
//        }
//    }

//    // max is now equal to the highest-allocated MIDI channel.  We can
//    // now allocate the next available channel.  This also works if
//    // no channels are currently allocated (max=-1)

//    result = max + 1;

//    // Don't allocate the MIDI percussion channel!

//    if (result == MIDI_PERCUSSION_CHAN) {
//        ++result;
//    }

//    return result;
//}

//// Given a MUS channel number, get the MIDI channel number to use
//// in the outputted file.

// static int GetMIDIChannel(int mus_channel, const byte *midioutput)
//{
//    // Find the MIDI channel to use for this MUS channel.
//    // MUS channel 15 is the percusssion channel.

//    if (mus_channel == MUS_PERCUSSION_CHAN) {
//        return MIDI_PERCUSSION_CHAN;
//    } else {
//        // If a MIDI channel hasn't been allocated for this MUS channel
//        // yet, allocate the next free MIDI channel.

//        if (channel_map[mus_channel] == -1) {
//            channel_map[mus_channel] = AllocateMIDIChannel();

//            // First time using the channel, send an "all notes off"
//            // event. This fixes "The D_DDTBLU disease" described here:
//            // https://www.doomworld.com/vb/source-ports/66802-the
//            WriteChangeController_Valueless(channel_map[mus_channel], 0x7b, midioutput);
//        }

//        return channel_map[mus_channel];
//    }
//}

//// Read a MUS file from a stream (musinput) and output a MIDI file to
//// a stream (midioutput).
////
//// Returns 0 on success or 1 on failure.

// boolean mus2mid(const byte *musinput, const byte *midioutput)
//{
//    // Header for the MUS file
//    MusHeader musfileheader;

//    // Descriptor for the current MUS event
//    byte eventdescriptor;
//    int channel;  // Channel number
//    musevent event;

//    // Bunch of vars read from MUS lump
//    byte key;
//    byte controllernumber;
//    byte controllervalue;

//    // Buffer used for MIDI track size record
//    byte tracksizebuffer[4];

//    // Flag for when the score end marker is hit.
//    int hitscoreend = 0;

//    // Temp working byte
//    byte working;
//    // Used in building up time delays
//    unsigned int timedelay;

//    // Initialise channel map to mark all channels as unused.

//    for (channel = 0; channel < NUM_CHANNELS; ++channel) {
//        channel_map[channel] = -1;
//    }

//    // Grab the header

//    if (!ReadMusHeader(musinput, &musfileheader)) {
//        return true;
//    }

//#ifdef CHECK_MUS_HEADER
//    // Check MUS header
//    if (musfileheader.id[0] != 'M' || musfileheader.id[1] != 'U' || musfileheader.id[2] != 'S' ||
//        musfileheader.id[3] != 0x1A) {
//        return true;
//    }
//#endif

//    // Seek to where the data is held
//    if (mem_fseek(musinput, (long)musfileheader.scorestart, MEM_SEEK_SET) != 0) {
//        return true;
//    }

//    // So, we can assume the MUS file is faintly legit. Let's start
//    // writing MIDI data...

//    mem_fwrite(midiheader, 1, sizeof(midiheader), midioutput);
//    tracksize = 0;

//    // Now, process the MUS file:
//    while (!hitscoreend) {
//        // Handle a block of events:

//        while (!hitscoreend) {
//            // Fetch channel number and event code:

//            if (mem_fread(&eventdescriptor, 1, 1, musinput) != 1) {
//                return true;
//            }

//            channel = GetMIDIChannel(eventdescriptor & 0x0F, midioutput);
//            event = eventdescriptor & 0x70;

//            switch (event) {
//            case mus_releasekey:
//                if (mem_fread(&key, 1, 1, musinput) != 1) {
//                    return true;
//                }

//                if (WriteReleaseKey(channel, key, midioutput)) {
//                    return true;
//                }

//                break;

//            case mus_presskey:
//                if (mem_fread(&key, 1, 1, musinput) != 1) {
//                    return true;
//                }

//                if (key & 0x80) {
//                    if (mem_fread(&channelvelocities[channel], 1, 1, musinput) != 1) {
//                        return true;
//                    }

//                    channelvelocities[channel] &= 0x7F;
//                }

//                if (WritePressKey(channel, key, channelvelocities[channel], midioutput)) {
//                    return true;
//                }

//                break;

//            case mus_pitchwheel:
//                if (mem_fread(&key, 1, 1, musinput) != 1) {
//                    break;
//                }
//                if (WritePitchWheel(channel, (short)(key * 64), midioutput)) {
//                    return true;
//                }

//                break;

//            case mus_systemevent:
//                if (mem_fread(&controllernumber, 1, 1, musinput) != 1) {
//                    return true;
//                }
//                if (controllernumber < 10 || controllernumber > 14) {
//                    return true;
//                }

//                if (WriteChangeController_Valueless(channel, controller_map[controllernumber], midioutput)) {
//                    return true;
//                }

//                break;

//            case mus_changecontroller:
//                if (mem_fread(&controllernumber, 1, 1, musinput) != 1) {
//                    return true;
//                }

//                if (mem_fread(&controllervalue, 1, 1, musinput) != 1) {
//                    return true;
//                }

//                if (controllernumber == 0) {
//                    if (WriteChangePatch(channel, controllervalue, midioutput)) {
//                        return true;
//                    }
//                } else {
//                    if (controllernumber < 1 || controllernumber > 9) {
//                        return true;
//                    }

//                    if (WriteChangeController_Valued(channel, controller_map[controllernumber], controllervalue,
//                                                     midioutput)) {
//                        return true;
//                    }
//                }

//                break;

//            case mus_scoreend:
//                hitscoreend = 1;
//                break;

//            default:
//                return true;
//                break;
//            }

//            if (eventdescriptor & 0x80) {
//                break;
//            }
//        }
//        // Now we need to read the time code:
//        if (!hitscoreend) {
//            timedelay = 0;
//            for (;;) {
//                if (mem_fread(&working, 1, 1, musinput) != 1) {
//                    return true;
//                }

//                timedelay = timedelay * 128 + (working & 0x7F);
//                if ((working & 0x80) == 0) {
//                    break;
//                }
//            }
//            queuedtime += timedelay;
//        }
//    }

//    // End of track
//    if (WriteEndTrack(midioutput)) {
//        return true;
//    }

//    // Write the track size into the stream
//    if (mem_fseek(midioutput, 18, MEM_SEEK_SET)) {
//        return true;
//    }

//    tracksizebuffer[0] = (tracksize >> 24) & 0xff;
//    tracksizebuffer[1] = (tracksize >> 16) & 0xff;
//    tracksizebuffer[2] = (tracksize >> 8) & 0xff;
//    tracksizebuffer[3] = tracksize & 0xff;

//    if (mem_fwrite(tracksizebuffer, 1, 4, midioutput) != 4) {
//        return true;
//    }

//    return false;
//}
