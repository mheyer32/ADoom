/******************************************************************************/
/*                                                                            */
/* includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <exec/ports.h>
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
    mus_measureend = 0x50,
    mus_scoreend = 0x60,
    mus_unused = 0x70
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
    unsigned short reserved;
    unsigned short instrumentPatchList[1];  // The instrument patch list is simply an array of MIDI patch numbers that
                                            // are used in the song. This is presumably so hardware like the GUS can
                                            // have the required instrument samples loaded into memory before the song
                                            // begins. The instrument numbers are 0-127 for standard MIDI instruments,
                                            // and 135-181 for standard MIDI percussion (notes 35-81 on channel 10).
} MusHeader;

// Cached channel velocities
static const byte g_controllerMap[] = {
    MS_Prog,
    MC_Bank /*FIXME: was 0x20; MUS file format says controller 1 is 'bank select' so MC_Bank seems the right choice? */,
    MC_ModWheel,
    MC_Volume,
    MC_Pan,
    MC_Expression,
    MC_ExtDepth,
    MC_ChorusDepth,
    MC_Sustain,
    MC_SoftPedal,

    MC_Max /*120: all channels off*/,
    MM_AllOff,
    MM_Mono,
    MM_Poly,
    MM_ResetCtrl};

static byte g_channelVolumes[NUM_CHANNELS];
static byte g_channelVelocities[NUM_CHANNELS];
static signed char g_channelMap[NUM_CHANNELS];
static unsigned int g_masterVolume = 32;

static const ULONG MidiTics = TICK_FREQ / 140;  // FIXME: this probably gets rounded and will eventually
                                                // drift. We should keep a global song time and adjust
                                                // the ticks dependent on wallclock time

// Only one Game can talk to this library at  any time
static struct Task *g_MainTask = NULL;
static struct MsgPort *g_mainMsgPort = NULL;
static struct Task *g_playerTask = NULL;
static struct MsgPort *g_playerMsgPort = NULL;
static struct Player *g_player = NULL;
static struct MidiNode *g_midiNode = NULL;
static struct MidiLink *g_midiLink = NULL;
static BYTE g_playerSignalBit = -1;

typedef enum
{
    PC_PLAY,
    PC_STOP,
    PC_PAUSE,
    PC_RESUME,
    PC_VOLUME
} PlayerCommand;

typedef struct
{
    struct Message msg;
    PlayerCommand code;
    int data;
} PlayerMessage;

typedef enum
{
    STOPPED = 0,
    PAUSED,
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

static void ResetChannels(void);

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

void ShutDownMidi(void)
{
    if (g_player) {
        SetConductorState(g_player, CONDSTATE_STOPPED, 0);
        DeletePlayer(g_player);
        g_player = NULL;
    }
    if (g_playerSignalBit != -1) {
        FreeSignal(g_playerSignalBit);
    }
    if (g_midiNode) {
        ResetChannels();

        // FIXME: set reset sysex to device to get clean slate
        FlushMidi(g_midiNode);
        if (g_midiLink) {
            RemoveMidiLink(g_midiLink);
            g_midiLink = NULL;
        }
        DeleteMidi(g_midiNode);
        g_midiNode = NULL;
    }
}

static void SetMasterVolume(int volume);
static void WriteChannelVolume(byte channel, byte volume);

boolean InitMidi(void)
{
    g_midiNode = CreateMidi(MIDI_MsgQueue, 0L, MIDI_SysExSize, 0, MIDI_Name, (Tag) "DOOM Midi Out", TAG_END);
    if (!g_midiNode) {
        goto failure;
    }

    const char *deviceName = FindMidiDevice();
    if (!deviceName) {
        goto failure;
    }

    g_midiLink = AddMidiLink(g_midiNode, MLTYPE_Sender, MLINK_Location, (Tag)deviceName /*"out.0"*/, TAG_END);
    if (!g_midiLink) {
        goto failure;
    }

    // FIXME: set reset sysex to device to get clean slate

    // technically, the Task is supposed to open its own library bases...
    struct Task *thisTask = FindTask(NULL);

    if ((g_playerSignalBit = AllocSignal(-1)) == -1) {
        goto failure;
    }

    ULONG err = 0;
    g_player =
        CreatePlayer(PLAYER_Name, (Tag) "DOOM Player", PLAYER_Conductor, (Tag) "DOOM Conductor", PLAYER_AlarmSigTask,
                     (Tag)thisTask, PLAYER_AlarmSigBit, (Tag)g_playerSignalBit, PLAYER_ErrorCode, (Tag)&err, TAG_END);
    if (!g_player) {
        goto failure;
    }

    //    err = SetConductorState(player, CONDSTATE_RUNNING, 0);

    for (byte channel = 0; channel < NUM_CHANNELS; ++channel) {
        g_channelVolumes[channel] = 127;
        g_channelVelocities[channel] = 127;
        g_channelMap[channel] = -1;
    }
    // start with a low, but audible volume
    SetMasterVolume(g_masterVolume);

    return TRUE;

failure:
    ShutDownMidi();
    return FALSE;
}

static inline void RestoreA4(void)
{
    __asm volatile("\tlea ___a4_init, a4");
}

static void SendPlayerMessage(PlayerCommand command, int data)
{
    if (g_playerMsgPort) {
        PlayerMessage msg;
        msg.msg.mn_Length = sizeof(msg);
        msg.msg.mn_ReplyPort = g_mainMsgPort;
        msg.code = command;
        msg.data = data;
        PutMsg(g_playerMsgPort, &msg.msg);
        WaitPort(g_mainMsgPort);
        GetMsg(g_mainMsgPort);
    }
}

static void __stdargs MidiPlayerTask(void);

/******************************************************************************/
/*                                                                            */
/* user library initialization                                                */
/*                                                                            */
/* !!! CAUTION: This function may run in a forbidden state !!!                */
/*                                                                            */
/******************************************************************************/

__stdargs void __UserLibCleanup(void);

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

__stdargs void __Mus_SetVol(int vol)
{
    if (g_playerTask && g_midiLink) {
        SendPlayerMessage(PC_VOLUME, vol);
    } else {
        g_masterVolume = vol;
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
    if (!g_playerTask) {
        g_MainTask = FindTask(NULL);

        g_mainMsgPort = CreatePort(NULL, 0);
        if (!g_mainMsgPort) {
            return 0;
        }

        BPTR file = 0;
        //        Open("PROGDIR:log.txt", MODE_READWRITE);
        //        Seek(file, 0, OFFSET_END);
        //        FPrintf(file, "Creating new Task  -------------------\n");

        // Set priority to 21, so it is just a bit higher than input and mouse movements won't slow down
        // playback
        if ((g_playerTask = (struct Task *)CreateNewProcTags(NP_Name, (Tag) "DOOM Midi", NP_Priority, 21, NP_Entry,
                                                             (Tag)MidiPlayerTask, NP_StackSize, 4096, NP_Output,
                                                             (Tag)file, TAG_END)) == NULL) {
            return 0;
        }

        ULONG signal = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E);
        if (!(signal & SIGBREAKF_CTRL_E)) {
            // Task failed to start or allocate resources. It'll signal SIGBREAKF_CTRL_C and exit
            g_playerTask = 0;
            return 0;
        }
    }

    Song *song = (Song *)AllocVec(sizeof(Song), MEMF_ANY | MEMF_CLEAR);
    if (!song) {
        return 0;
    }

    song->dataPtr.b = (const byte *)musdata;
    song->currentPtr.b = song->dataPtr.b;
    MusData tempData = song->dataPtr;  // make sure reading the header does not affect our data pointers
    if (!ReadMusHeader(&tempData, &song->header)) {
        FreeVec(song);
        return 0;
    }

    return (int)song;
}

__stdargs void __Mus_Unregister(int handle)
{
    Song *song = (Song *)handle;
    if (g_playerTask) {
        // kill player, because we only allow one registered music at any time
        Signal(g_playerTask, SIGBREAKF_CTRL_C);
        Wait(SIGBREAKF_CTRL_E);
        g_playerTask = NULL;

        if (g_mainMsgPort) {
            DeletePort(g_mainMsgPort);
            g_mainMsgPort = 0;
        }
    }
    if (song) {
        FreeVec(song);
    }
}

__stdargs void __Mus_Play(int handle, int looping)
{
    Song *song = (Song *)handle;
    if (!song)
        return;

    SendPlayerMessage(PC_STOP, handle);

    song->looping = !!looping;

    SendPlayerMessage(PC_PLAY, handle);
}

__stdargs void __Mus_Stop(int handle)
{
    Song *song = (Song *)handle;
    if (!song)
        return;
    // what if handle is not currently playing song?
    SendPlayerMessage(PC_STOP, 0);
}

__stdargs void __Mus_Pause(int handle)
{
    Song *song = (Song *)handle;
    if (!song)
        return;
    // what if handle is not currently playing song?
    SendPlayerMessage(PC_PAUSE, 0);
}

__stdargs void __Mus_Resume(int handle)
{
    Song *song = (Song *)handle;
    if (!song)
        return;
    SendPlayerMessage(PC_RESUME, 0);
}

__stdargs int __Mus_Done(int handle)
{
    Song *song = (Song *)handle;
    if (!song)
        return 1;

    return song->done;
}

// Write a key press event
static inline void WritePressKey(byte channel, byte key, byte velocity)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_NoteOn | channel;
    mm.mm_Data1 = key & 0x7F;
    mm.mm_Data2 = velocity & 0x7F;

    PutMidiMsg(g_midiLink, &mm);
}

// Write a key release event
static inline void WriteReleaseKey(byte channel, byte key)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_NoteOff | channel;
    mm.mm_Data1 = key & 0x7F;
    mm.mm_Data2 = 0;

    PutMidiMsg(g_midiLink, &mm);
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

    PutMidiMsg(g_midiLink, &mm);
}

// Write a patch change event
static inline boolean WriteChangePatch(byte channel, byte patch)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_Prog | channel;
    mm.mm_Data1 = patch & 0x7F;

    PutMidiMsg(g_midiLink, &mm);
}

// Write a valued controller change event
static inline void WriteChangeController_Valued(byte channel, byte control, byte value)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_Ctrl | channel;
    mm.mm_Data1 = control;
    // Quirk in vanilla DOOM? MUS controller values should be
    // 7-bit, not 8-bit.
    // Fix on said quirk to stop MIDI players from complaining that
    // the value is out of range:
    mm.mm_Data2 = value & 0x80 ? 0x7F : value;

    PutMidiMsg(g_midiLink, &mm);
}

// expects volume to be in 0...0x7f range
static void WriteChannelVolume(byte channel, byte volume)
{
    // affect channel volume by master volume
    g_channelVolumes[channel] = volume;
    byte channelVolume = (volume * g_masterVolume) / 64;
    WriteChangeController_Valued(channel, MC_Volume, channelVolume);
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
        if (g_channelMap[i] > max) {
            max = g_channelMap[i];
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

    assert(result < NUM_CHANNELS);
    return result;
}

// Given a MUS channel number, get the MIDI channel number to use
static byte GetMIDIChannel(byte mus_channel)
{
    // Find the MIDI channel to use for this MUS channel.
    // MUS channel 15 is the percusssion channel.

    if (mus_channel == MUS_PERCUSSION_CHAN) {
        return MIDI_PERCUSSION_CHAN;
    } else {
        // If a MIDI channel hasn't been allocated for this MUS channel
        // yet, allocate the next free MIDI channel.

        if (g_channelMap[mus_channel] == -1) {
            g_channelMap[mus_channel] = AllocateMIDIChannel();

            // First time using the channel, send an "all notes off"
            // event. This fixes "The D_DDTBLU disease" described here:
            // https://www.doomworld.com/vb/source-ports/66802-the
            WriteChangeController_Valueless(g_channelMap[mus_channel], MM_AllOff);
        }

        return g_channelMap[mus_channel];
    }
}

static inline UBYTE ReadByte(MusData *data)
{
    return *(data->b++);
}

static void ResetChannels(void)
{
    // Initialise channel map to mark all channels as unused.
    for (byte channel = 0; channel < NUM_CHANNELS; ++channel) {
        WriteChangeController_Valueless(channel, MM_AllOff);
        WriteChangeController_Valueless(channel, MM_ResetCtrl);
        WriteChannelVolume(channel, 127);
        g_channelMap[channel] = -1;
    }
}

static void RewindSong(Song *song)
{
    // Seek to start of song data
    song->currentPtr.b = song->dataPtr.b + song->header.scorestart;
    song->done = false;
}

static void RestartPlayTime(Song *song)
{
    song->currentTicks = 0;
    //    SetConductorState(g_player, CONDSTATE_SHUTTLE, 0);
    SetConductorState(g_player, CONDSTATE_RUNNING, 0);

    LONG res = SetPlayerAttrs(g_player, PLAYER_AlarmTime, 0, PLAYER_Ready, TRUE, TAG_END);
}

static void PlaySong(Song *song)
{
    // Silence all channels
    ResetChannels();

    // FIXME: what if some other song is playing already?
    if (song) {
        song->state = PLAYING;
        RewindSong(song);
        RestartPlayTime(song);
    } else {
    }
}

static void StopSong(Song *song)
{
    song->done = true;
    song->state = STOPPED;
}

static void PlayNextEvent(Song *song)
{
    while (song->state == PLAYING) {
        boolean loopSong = false;
        // Fetch channel number and event code:
        byte eventdescriptor = ReadByte(&song->currentPtr);

        byte channel = GetMIDIChannel(eventdescriptor & 0x0F);
        musevent event = eventdescriptor & 0x70;

        switch (event) {
        case mus_releasekey: {
            byte key = ReadByte(&song->currentPtr);
            WriteReleaseKey(channel, key);
        } break;

        case mus_presskey: {
            byte key = ReadByte(&song->currentPtr);
            if (key & 0x80) {
                // second byte has volume
                g_channelVelocities[channel] = ReadByte(&song->currentPtr) & 0x7F;
            }
            WritePressKey(channel, key, g_channelVelocities[channel]);
            //            FPrintf(Output(), "mus_presskey.... %lu \n", (unsigned)key);
        } break;

        case mus_pitchwheel: {
            byte bend = ReadByte(&song->currentPtr);
            WritePitchWheel(channel, (unsigned short)bend * 64);
            //            FPrintf(Output(), "mus_pitchwheel.... %lu \n", (long)bend);
        } break;

        case mus_systemevent: {
            byte controllernumber = ReadByte(&song->currentPtr);
            if (controllernumber >= 10 && controllernumber <= 14) {
                WriteChangeController_Valueless(channel, g_controllerMap[controllernumber]);
            }
            //            FPrintf(Output(), "mus_systemevent.... %lu \n", (long)controllernumber);
        } break;

        case mus_changecontroller: {
            byte controllernumber = ReadByte(&song->currentPtr);
            byte controllervalue = ReadByte(&song->currentPtr);

            //            FPrintf(Output(), "mus_changecontroller.... %lu %u \n", (long)controllernumber,
            //                    (unsigned short)controllervalue);

            if (controllernumber == 0) {
                //                FPrintf(Output(), "WriteChangePatch.... \n");
                WriteChangePatch(channel, controllervalue);
            } else {
                if (controllernumber >= 1 && controllernumber <= 9) {
                    if (controllernumber == 3) {
                        WriteChannelVolume(channel, controllervalue);
                    } else {
                        WriteChangeController_Valued(channel, g_controllerMap[controllernumber], controllervalue);
                    }
                }
            }
        } break;

        case mus_scoreend:
            if (!song->looping) {
                StopSong(song);
                return;  // no further parsing, no need to setup new player event
            } else {
                loopSong = true;
            }
            break;

        case mus_measureend:
            StopSong(song);
            return;  // no further parsing, no need to setup new player event
            break;

        case mus_unused:
            ReadByte(&song->currentPtr);
            break;

        default:
            assert(0);
            break;
        }

        if (eventdescriptor & 0x80) {
            // time delay, indicating we should wait for some time before playing the next
            // note.
            unsigned int timedelay = 0;
            int x = 0;
            while (true) {
                ++x;
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
            if (alarmTime < g_player->pl_MetricTime) {
                // If song time has fallen behind wallclock, reset player time to catch up
                RestartPlayTime(song);
                alarmTime = 0;
            }

            if (loopSong == true) {
                // rewind here, to not disturb parsing the even delay
                RewindSong(song);
            }
            LONG res = SetPlayerAttrs(g_player, PLAYER_AlarmTime, alarmTime, PLAYER_Ready, TRUE, TAG_END);
            return;

        } else {
            if (loopSong == true) {
                RewindSong(song);
                RestartPlayTime(song);
                return;
            }
        }
    }
}

void SetMasterVolume(int volume)
{
    g_masterVolume = volume;

    for (char channel = 0; channel < NUM_CHANNELS; ++channel) {
        WriteChannelVolume(channel, g_channelVolumes[channel]);
    }
    //    for (char channel = 0; channel < NUM_CHANNELS; ++channel) {
    //        char midiChannel = channel_map[channel];
    //        if (midiChannel != -1) {
    //            WriteChannelVolume(midiChannel, 100);
    //        }
    //    }
    //    WriteChannelVolume(MIDI_PERCUSSION_CHAN, 100);
}

static void __stdargs MidiPlayerTask(void)
{
    RestoreA4();

    if (!InitMidi()) {
        Signal(g_MainTask, SIGBREAKF_CTRL_C);
        return;
    }

    if (!(g_playerMsgPort = CreatePort(NULL, 0))) {
        ShutDownMidi();
        Signal(g_MainTask, SIGBREAKF_CTRL_C);
        return;
    }

    // Make sure the library does not get unloaded before MidiPlayerTask exits
    struct Library *myDoomSndBase = OpenLibrary("doomsound.library", 0);

    // Let main thread know were alive
    Signal(g_MainTask, SIGBREAKF_CTRL_E);

    const ULONG playerSignalBitMask = (1UL << g_playerSignalBit);
    const ULONG playerMsgMask = (1UL << g_playerMsgPort->mp_SigBit);
    const ULONG signalMask = playerSignalBitMask | playerMsgMask | SIGBREAKF_CTRL_C;

    Song *playingSong = NULL;

    while (true) {
        ULONG signals = Wait(signalMask);
        if (signals & SIGBREAKF_CTRL_C) {
            break;
        }
        if (signals & playerMsgMask) {
            PlayerMessage *msg = NULL;
            while (msg = (PlayerMessage *)GetMsg(g_playerMsgPort)) {
                switch (msg->code) {
                case PC_PLAY:
                    playingSong = (Song *)msg->data;
                    PlaySong(playingSong);
                    break;
                case PC_STOP:
                    StopSong(playingSong);
                    break;
                case PC_RESUME:
                    playingSong->state = PLAYING;
                    break;
                case PC_PAUSE:
                    playingSong->state = PAUSED;
                case PC_VOLUME:
                    SetMasterVolume(msg->data);
                    break;
                }
                ReplyMsg((struct Message *)msg);
            }
        }
        if ((signals & playerSignalBitMask) && playingSong) {
            PlayNextEvent(playingSong);
        }
    };

    {
        struct Message *msg = NULL;
        while (msg = GetMsg(g_playerMsgPort)) /* Make sure port is empty. */
        {
            ReplyMsg(msg);
        }
    }

    DeletePort(g_playerMsgPort);

    ShutDownMidi();
    CloseLibrary(myDoomSndBase);

    Forbid();
    Signal(g_MainTask, SIGBREAKF_CTRL_E);
}
