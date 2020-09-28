#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <exec/ports.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/realtime.h>

#include "DoomSnd.h"
#include "doomtype.h"
#include "m_swap.h"

#include "camd.h"
#include "midi/mididefs.h"

#include <assert.h>
#include <string.h>
//#include <dos.h>

struct Library *CamdBase = NULL;
struct RealTimeBase *RealTimeBase = NULL;
//struct DosLibrary *DOSBase = NULL;

static inline struct ExecBase * getSysBase(void)
{
    return SysBase;
}
#define LOCAL_SYSBASE() struct ExecBase * const SysBase = getSysBase()

static inline struct Library * getCamdBase(void)
{
    return CamdBase;
}
#define LOCAL_CAMDBASE() struct Library * const CamdBase = getCamdBase()

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
struct Task *g_playerTask = NULL;
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
    LOCAL_CAMDBASE();

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
    LOCAL_CAMDBASE();

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

boolean InitMidi(void)
{
    LOCAL_CAMDBASE();

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
    struct MsgPort * const mainMsgPort = g_mainMsgPort;
    struct MsgPort * const playerMsgPort = g_playerMsgPort;

    if (playerMsgPort) {
        LOCAL_SYSBASE();

        PlayerMessage msg;
        msg.msg.mn_Length = sizeof(msg);
        msg.msg.mn_ReplyPort = mainMsgPort;
        msg.code = command;
        msg.data = data;
        PutMsg(playerMsgPort, &msg.msg);
        WaitPort(mainMsgPort);
        GetMsg(mainMsgPort);
    }
}

static void __stdargs MidiPlayerTask(void);


__stdargs void __Mus_SetVol(int vol)
{
    if (g_playerTask && g_midiLink) {
        SendPlayerMessage(PC_VOLUME, vol);
    } else {
        g_masterVolume = vol;
    }
}

boolean ReadMusHeader(MusData *musdata, MusHeader *header)
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
static inline void WritePressKey(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, byte key, byte velocity)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_NoteOn | channel;
    mm.mm_Data1 = key & 0x7F;
    mm.mm_Data2 = velocity & 0x7F;

    PutMidiMsg(midiLink, &mm);
}

// Write a key release event
static inline void WriteReleaseKey(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, byte key)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_NoteOff | channel;
    mm.mm_Data1 = key & 0x7F;
    mm.mm_Data2 = 0;

    PutMidiMsg(midiLink, &mm);
}

// Write a pitch wheel/bend event
static inline boolean WritePitchWheel(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, unsigned short wheel)
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
static inline boolean WriteChangePatch(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, byte patch)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_Prog | channel;
    mm.mm_Data1 = patch & 0x7F;

    PutMidiMsg(midiLink, &mm);
}

// Write a valued controller change event
static inline void WriteChangeController_Valued(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, byte control, byte value)
{
    MidiMsg mm = {0};
    mm.mm_Status = MS_Ctrl | channel;
    mm.mm_Data1 = control;
    // Quirk in vanilla DOOM? MUS controller values should be
    // 7-bit, not 8-bit.
    // Fix on said quirk to stop MIDI players from complaining that
    // the value is out of range:
    mm.mm_Data2 = value & 0x80 ? 0x7F : value;

    PutMidiMsg(midiLink, &mm);
}

// expects volume to be in 0...0x7f range
static void WriteChannelVolume(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, byte volume)
{
    // affect channel volume by master volume
    g_channelVolumes[channel] = volume;
    byte channelVolume = (volume * g_masterVolume) / 64;
    WriteChangeController_Valued(CamdBase, midiLink, channel, MC_Volume, channelVolume);
}

// Write a valueless controller change event
static inline void WriteChangeController_Valueless(struct Library * const CamdBase, struct MidiLink * const midiLink, byte channel, byte control)
{
    return WriteChangeController_Valued(CamdBase, midiLink, channel, control, 0);
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
static byte GetMIDIChannel(struct Library * const CamdBase, struct MidiLink * const midiLink,  byte mus_channel)
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
            WriteChangeController_Valueless(CamdBase, midiLink, g_channelMap[mus_channel], MM_AllOff);
        }

        return g_channelMap[mus_channel];
    }
}

static inline UBYTE ReadByte(MusData * const data)
{
    return *(data->b++);
}

static void ResetChannels(void)
{
    LOCAL_CAMDBASE();
    struct MidiLink * const midiLink = g_midiLink;

    // Initialise channel map to mark all channels as unused.
    for (byte channel = 0; channel < NUM_CHANNELS; ++channel) {
        WriteChangeController_Valueless(CamdBase, midiLink, channel, MM_AllOff);
        WriteChangeController_Valueless(CamdBase, midiLink, channel, MM_ResetCtrl);
        WriteChannelVolume(CamdBase, midiLink, channel, 127);
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
    LOCAL_CAMDBASE();

    song->currentTicks = 0;

    struct Player * const player = g_player;

    SetConductorState(player, CONDSTATE_RUNNING, 0);
    LONG res = SetPlayerAttrs(player, PLAYER_AlarmTime, 0, PLAYER_Ready, TRUE, TAG_END);
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
    LOCAL_CAMDBASE();
    struct MidiLink * const midiLink = g_midiLink;

    while (song->state == PLAYING) {
        boolean loopSong = false;
        // Fetch channel number and event code:
        byte eventdescriptor = ReadByte(&song->currentPtr);

        byte channel = GetMIDIChannel(CamdBase, midiLink, eventdescriptor & 0x0F);
        musevent event = eventdescriptor & 0x70;

        switch (event) {
        case mus_releasekey: {
            byte key = ReadByte(&song->currentPtr);
            WriteReleaseKey(CamdBase, midiLink, channel, key);
        } break;

        case mus_presskey: {
            byte key = ReadByte(&song->currentPtr);
            if (key & 0x80) {
                // second byte has volume
                g_channelVelocities[channel] = ReadByte(&song->currentPtr) & 0x7F;
            }
            WritePressKey(CamdBase, midiLink, channel, key, g_channelVelocities[channel]);
            //            FPrintf(Output(), "mus_presskey.... %lu \n", (unsigned)key);
        } break;

        case mus_pitchwheel: {
            byte bend = ReadByte(&song->currentPtr);
            WritePitchWheel(CamdBase, midiLink, channel, (unsigned short)bend * 64);
            //            FPrintf(Output(), "mus_pitchwheel.... %lu \n", (long)bend);
        } break;

        case mus_systemevent: {
            byte controllernumber = ReadByte(&song->currentPtr);
            if (controllernumber >= 10 && controllernumber <= 14) {
                WriteChangeController_Valueless(CamdBase, midiLink, channel, g_controllerMap[controllernumber]);
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
                WriteChangePatch(CamdBase, midiLink, channel, controllervalue);
            } else {
                if (controllernumber >= 1 && controllernumber <= 9) {
                    if (controllernumber == 3) {
                        WriteChannelVolume(CamdBase, midiLink, channel, controllervalue);
                    } else {
                        WriteChangeController_Valued(CamdBase, midiLink, channel, g_controllerMap[controllernumber], controllervalue);
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

            struct Player * const player = g_player;
            if (alarmTime < player->pl_MetricTime) {
                // If song time has fallen behind wallclock, reset player time to catch up
                RestartPlayTime(song);
                alarmTime = 0;
            }

            if (loopSong == true) {
                // rewind here, to not disturb parsing the even delay
                RewindSong(song);
            }
            LONG res = SetPlayerAttrs(player, PLAYER_AlarmTime, alarmTime, PLAYER_Ready, TRUE, TAG_END);
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
    LOCAL_CAMDBASE();
    struct MidiLink * const midiLink = g_midiLink;

    g_masterVolume = volume;

    for (char channel = 0; channel < NUM_CHANNELS; ++channel) {
        WriteChannelVolume(CamdBase, midiLink, channel, g_channelVolumes[channel]);
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

    LOCAL_SYSBASE();

    if (!InitMidi()) {
        Signal(g_MainTask, SIGBREAKF_CTRL_C);
        return;
    }

    if (!(g_playerMsgPort = CreatePort(NULL, 0))) {
        ShutDownMidi();
        Signal(g_MainTask, SIGBREAKF_CTRL_C);
        return;
    }
    struct MsgPort * const playerMsgPort = g_playerMsgPort;

    // Make sure the library does not get unloaded before MidiPlayerTask exits
    struct Library *myDoomSndBase = OpenLibrary("doomsound.library", 0);

    // Let main thread know were alive
    Signal(g_MainTask, SIGBREAKF_CTRL_E);

    const ULONG playerSignalBitMask = (1UL << g_playerSignalBit);
    const ULONG playerMsgMask = (1UL << playerMsgPort->mp_SigBit);
    const ULONG signalMask = playerSignalBitMask | playerMsgMask | SIGBREAKF_CTRL_C;

    Song *playingSong = NULL;

    while (true) {
        ULONG signals = Wait(signalMask);
        if (signals & SIGBREAKF_CTRL_C) {
            break;
        }
        if (signals & playerMsgMask) {
            PlayerMessage *msg = NULL;
            while (msg = (PlayerMessage *)GetMsg(playerMsgPort)) {
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
        while (msg = GetMsg(playerMsgPort)) /* Make sure port is empty. */
        {
            ReplyMsg(msg);
        }
    }

    DeletePort(playerMsgPort);
    g_playerMsgPort = NULL;

    ShutDownMidi();
    CloseLibrary(myDoomSndBase);

    Forbid();
    Signal(g_MainTask, SIGBREAKF_CTRL_E);
}
