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

#include "DoomSnd.h"
#include "DoomSndFwd.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_swap.h"

#include <camd.h>
#include <midi/mididefs.h>

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
static const byte controller_map[] = {0x00, 0x20, 0x01, 0x07, 0x0A, 0x0B, 0x5B, 0x5D,
                                      0x40, 0x43, 0x78, 0x7B, 0x7E, 0x7F, 0x79};

static char channel_map[NUM_CHANNELS];
static byte masterVolume;

// This would require a multi-base library, since the mainTask would differ per application

static const ULONG midiTics = TICK_FREQ / 140;  // FIXME: this probably gets rounded and will eventually
                                                // drift. We should keep a global song time and adjust
                                                // the ticks dependent on wallclock time

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
    MusData currentPtr;
    unsigned int currentTicks;
    SongState state;
    boolean looping;
    boolean done;
    MusHeader header;
    MusData dataPtr;
} Song;

volatile static Song *currentSong = NULL;

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
        // FIXME: set reset sysex to device to get clean slate
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

    // FIXME: set reset sysex to device to get clean slate

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

    err = SetConductorState(player, CONDSTATE_RUNNING, 0);

    return TRUE;

failure:
    ShutDownMidiTask();
    return FALSE;
}

static inline void RestoreA4(void)
{
    __asm volatile("\tlea ___a4_init, a4");
}

static inline void HaltTask(void)
{
    if (playerTask) {
        Signal(playerTask, SIGBREAKF_CTRL_E);
        Wait(SIGBREAKF_CTRL_E);
    }
}
static inline void ResumeTask(void)
{
    if (playerTask) {
        Signal(playerTask, SIGBREAKF_CTRL_E);
    }
}

static void SetMasterVolume(int volume);

static void __stdargs MidiPlayerTask(void);

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

__stdargs void __Mus_SetVol(int vol)
{
    if (playerTask && midiLink) {
        HaltTask();
        SetMasterVolume(vol);
        ResumeTask();
    } else {
        masterVolume = vol;
    }
}

void SetMasterVolume(int volume)
{
    masterVolume = volume;

    MidiMsg mm = {0};
    mm.mm_Data1 = MC_Volume;
    // DoomSound.library seems to accept 0..64
    mm.mm_Data2 = masterVolume * 2;
    if (mm.mm_Data2 & 0x80) {
        mm.mm_Data2 = 0x7F;
    }

    for (byte c = 0; c < NUM_CHANNELS; ++c) {
        mm.mm_Status = MS_Ctrl | c;
        PutMidiMsg(midiLink, &mm);
    }
}

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

        if ((playerTask = (struct Task *)CreateNewProcTags(NP_Name, (Tag) "DOOM Midi", NP_Priority, 21, NP_Entry,
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

    return (int)song;
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
    song->looping = !!looping;
    song->currentTicks = 0;
    song->done = false;
    song->currentPtr.b = song->dataPtr.b + song->header.scorestart;
    currentSong = song;
    ResumeTask();
}

__stdargs void __Mus_Stop(int handle)
{
    Song *song = (Song *)handle;

    HaltTask();
    song->state = STOPPED;
    song->currentTicks = 0;
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
    song->currentTicks = 0;
    song->state = PLAYING;
    ResumeTask();
}

__stdargs int __Mus_Done(int handle)
{
    Song *song = (Song *)handle;
    return song->done;
}

// Write a key press event
static inline void WritePressKey(byte channel, byte key, byte velocity)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_NoteOn | channel;
    mm.mm_Data1 = key & 0x7F;
    mm.mm_Data2 = velocity & 0x7F;

    PutMidiMsg(midiLink, &mm);
}

// Write a key release event
static inline void WriteReleaseKey(byte channel, byte key)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_NoteOff | channel;
    mm.mm_Data1 = key & 0x7F;
    mm.mm_Data2 = 0;

    PutMidiMsg(midiLink, &mm);
}

// Write a pitch wheel/bend event
static inline boolean WritePitchWheel(byte channel, unsigned short wheel)
{
    // Pitch Bend Change. This message is sent to indicate a change in the pitch bender (wheel or lever, typically).
    // The pitch bender is measured by a fourteen bit value. Center (no pitch change) is 2000H. Sensitivity is a
    // function of the receiver, but may be set using RPN 0. (lllllll) are the least significant 7 bits. (mmmmmmm)
    // are the most significant 7 bits.
    MidiMsg mm = {0};
    mm.mm_Status = MS_PitchBend | channel;
    mm.mm_Data1 = wheel & 0x7F;         // LSB
    mm.mm_Data2 = (wheel >> 7) & 0x7F;  // MSB

    PutMidiMsg(midiLink, &mm);
}

// Write a patch change event
static inline boolean WriteChangePatch(byte channel, byte patch)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_Prog | channel;
    mm.mm_Data1 = patch & 0x7F;

    PutMidiMsg(midiLink, &mm);
}

// Write a valued controller change event
static inline void WriteChangeController_Valued(byte channel, byte control, byte value)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_Ctrl | channel;
    mm.mm_Data1 = control & 0x7F;  // is & 7F really needed?
    // Quirk in vanilla DOOM? MUS controller values should be
    // 7-bit, not 8-bit.
    // Fix on said quirk to stop MIDI players from complaining that
    // the value is out of range:
    mm.mm_Data2 = value & 0x80 ? 0x7F : value;

    PutMidiMsg(midiLink, &mm);
}

// Write a valueless controller change event
static inline void WriteChangeController_Valueless(byte channel, byte control)
{
    return WriteChangeController_Valued(channel, control, 0);
}

//// Allocate a free MIDI channel.
static byte AllocateMIDIChannel(void)
{
    byte result;
    char max;
    char i;

    // Find the current highest-allocated channel.

    max = -1;

    for (i = 0; i < NUM_CHANNELS; ++i) {
        if (channel_map[i] > max) {
            max = channel_map[i];
        }
    }

    // max is now equal to the highest-allocated MIDI channel.  We can
    // now allocate the next available channel.  This also works if
    // no channels are currently allocated (max=-1)

    result = max + 1;

    // Don't allocate the MIDI percussion channel!

    if (result == MIDI_PERCUSSION_CHAN) {
        ++result;
    }

    return result;
}

//// Given a MUS channel number, get the MIDI channel number to use
//// in the outputted file.

static byte GetMIDIChannel(byte mus_channel)
{
    // Find the MIDI channel to use for this MUS channel.
    // MUS channel 15 is the percusssion channel.

    if (mus_channel == MUS_PERCUSSION_CHAN) {
        return MIDI_PERCUSSION_CHAN;
    } else {
        // If a MIDI channel hasn't been allocated for this MUS channel
        // yet, allocate the next free MIDI channel.

        if (channel_map[mus_channel] == -1) {
            channel_map[mus_channel] = AllocateMIDIChannel();

            // First time using the channel, send an "all notes off"
            // event. This fixes "The D_DDTBLU disease" described here:
            // https://www.doomworld.com/vb/source-ports/66802-the

            WriteChangeController_Valueless(channel_map[mus_channel], MM_AllOff);
        }

        return channel_map[mus_channel];
    }
}

static inline UBYTE ReadByte(MusData *data)
{
    return *(data->b++);
}

static void NewSong(Song *song)
{
    if (song) {
        // Seek to where the data is held
        song->currentPtr.b = song->dataPtr.b + song->header.scorestart;
        song->currentTicks = 0;

        SetConductorState(player, CONDSTATE_RUNNING, song->currentTicks);
        LONG res = SetPlayerAttrs(player, PLAYER_AlarmTime, midiTics, PLAYER_Ready, TRUE, TAG_END);
    } else {
    }

    // Initialise channel map to mark all channels as unused.
    for (byte channel = 0; channel < NUM_CHANNELS; ++channel) {
        if (channel_map[channel] != -1) {
            WriteChangeController_Valueless(channel_map[channel], MM_AllOff);
        }
        channel_map[channel] = -1;
    }

    SetMasterVolume(masterVolume);
}

static void PlayNextEvent(Song *song)
{
    byte key;
    byte controllernumber;
    byte controllervalue;

    while (song->state == PLAYING) {
        // Fetch channel number and event code:
        byte eventdescriptor = ReadByte(&song->currentPtr);

        byte channel = GetMIDIChannel(eventdescriptor & 0x0F);
        musevent event = eventdescriptor & 0x70;

        switch (event) {
        case mus_releasekey:
            key = ReadByte(&song->currentPtr);
            WriteReleaseKey(channel, key);
            break;

        case mus_presskey:
            key = ReadByte(&song->currentPtr);
            if (key & 0x80) {
                // second byte has volume
                channelvelocities[channel] = ReadByte(&song->currentPtr) & 0x7F;
            }
            WritePressKey(channel, key, channelvelocities[channel]);
            break;

        case mus_pitchwheel:
            key = ReadByte(&song->currentPtr);
            WritePitchWheel(channel, (unsigned short)key * 64);
            break;

        case mus_systemevent:
            controllernumber = ReadByte(&song->currentPtr);
            if (controllernumber >= 10 && controllernumber <= 14) {
                WriteChangeController_Valueless(channel, controller_map[controllernumber]);
            }
            break;

        case mus_changecontroller:
            controllernumber = ReadByte(&song->currentPtr);
            controllervalue = ReadByte(&song->currentPtr);
            if (controllernumber == 0) {
                WriteChangePatch(channel, controllervalue);
            } else {
                if (controllernumber >= 1 && controllernumber <= 9) {
                    WriteChangeController_Valued(channel, controller_map[controllernumber], controllervalue);
                }
            }
            break;

        case mus_scoreend:
            if (!song->looping) {
                song->done = true;
                song->state = STOPPED;
            } else {
                NewSong(song);
                return;
            }
            break;

        default:
            break;
        }

        if (eventdescriptor & 0x80) {
            // time delay, indicating we should wait for some time before playing the next
            // note
            // Now we need to read the time code:
            // Used in building up time delays
            unsigned int timedelay = 0;
            while (true) {
                byte working = ReadByte(&song->currentPtr);
                timedelay = timedelay * 128 + (working & 0x7F);
                if ((working & 0x80) == 0) {
                    break;
                }
            }
            // Advance global time in song
            song->currentTicks += timedelay;
            // convert MUS ticks into realtime.library ticks
            ULONG alarmTime = (song->currentTicks * TICK_FREQ) / 140;
            LONG res = SetPlayerAttrs(player, PLAYER_AlarmTime, alarmTime, PLAYER_Ready, TRUE, TAG_END);
            return;
        }
    }
}

static void __stdargs MidiPlayerTask(void)
{
    RestoreA4();

    if (!InitMidiTask()) {
        Signal(mainTask, SIGBREAKF_CTRL_C);
        return;
    }

    // Make sure the library does not get unloaded before MidiPlayerTask exits
    struct Library *myDoomSndBase = OpenLibrary("doomsound.library", 0);

    Signal(mainTask, SIGBREAKF_CTRL_E);

    const ULONG signalBitMask = (1UL << playerSignalBit);
    const ULONG signalMask = signalBitMask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E;

    Song *playingSong = NULL;

    while (true) {
        ULONG signals = Wait(signalMask);
        if (signals & SIGBREAKF_CTRL_C) {
            break;
        }
        if (signals & SIGBREAKF_CTRL_E) {
            // acknowledge we're stopping

            SetSignal(0, signalBitMask);
            signals &= ~signalBitMask;
            Signal(mainTask, SIGBREAKF_CTRL_E);

            // wait for maintask to do its thing
            Wait(SIGBREAKF_CTRL_E);

            if (currentSong != playingSong) {
                playingSong = (Song *)currentSong;
                NewSong(playingSong);
            } else {
                if (playingSong && playingSong->currentTicks == 0) {
                    // song time has been reset, so reset conductor time
                    SetConductorState(player, CONDSTATE_RUNNING, 0);
                }
            }
        }
        if (signals & signalBitMask && playingSong) {
            PlayNextEvent(playingSong);
            if (playingSong->done) {
                playingSong = NULL;
            }
        }
    };

    ShutDownMidiTask();
    CloseLibrary(myDoomSndBase);

    Signal(mainTask, SIGBREAKF_CTRL_E);
}
