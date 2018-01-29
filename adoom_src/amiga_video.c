#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/asl.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/lowlevel.h>
#include <proto/timer.h>

#include <clib/alib_protos.h>
#include <cybergraphx/cybergraphics.h>
#include <devices/gameport.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include <devices/timer.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>

#ifdef GRAFFITI
#include "graffiti.h"
#include "graffiti_lib.h"
#include "graffiti_protos.h"
#endif

#include "d_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "v_video.h"

#include "amiga_macros.h"
#include "amiga_median.h"
#include "amiga_mmu.h"
#include "amiga_sega.h"
#include "c2p8_040_amlaukka.h"
#include "c2p_020.h"
#include "c2p_030.h"
#include "c2p_040.h"
#include "r_draw.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef INDIVISION
#include "indivision.h"
#endif

/*experimental_c2p_stuff***********************************************/
extern int scaledviewwidth;
static C2PFunction c2p = NULL;

/**********************************************************************/
extern struct ExecBase *SysBase;
struct Library *AslBase = NULL;
struct Library *CyberGfxBase = NULL;
struct Library *LowLevelBase = NULL;
struct Library *KeymapBase = NULL;
struct GfxBase *GfxBase = NULL;
struct IORequest timereq;
struct Device *TimerBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

volatile struct Custom *const custom = (struct Custom *)0xdff000;

extern int cpu_type;

int SCREENWIDTH;
int SCREENHEIGHT;
int weirdaspect;

#define NUMPALETTES 14

// Palette translation for Indivision GFX
static byte ptranslate[128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x50, 0x51,
    0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
};
#ifdef INDIVISION
int video_indigfx = 0;  // Indivision GFX mode
ULONG gfxlength = 0;
#endif

static struct ScreenModeRequester *video_smr = NULL;
static struct Screen *video_screen = NULL;
static struct Window *video_window = NULL;
static struct BitMap video_bitmap[3] = {{0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}},
                                        {0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}},
                                        {0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}}};
static PLANEPTR video_raster[3] = {NULL, NULL, NULL};       /* contiguous bitplanes */
static UBYTE *video_compare_buffer[3] = {NULL, NULL, NULL}; /* in fastmem */
static struct RastPort video_rastport[3];
static struct ScreenBuffer *video_sb[2] = {NULL, NULL};
static struct DBufInfo *video_db = NULL;
static struct MsgPort *video_mp = NULL;
static int video_which = 0;
static BYTE video_sigbit1 = -1;
static BYTE video_sigbit2 = -1;
static volatile BYTE video_sigbit3 = -1;
static UBYTE *video_chipbuff = NULL;
static struct Task *video_maintask;
static struct Task *video_fliptask = NULL;
static int video_depth = 0;
static int video_oscan_height;
static FAR ULONG video_colourtable[NUMPALETTES][1 + 3 * 256 + 1];
static int video_palette_index = 0;
static int video_pending_palette_index = 0;
static volatile WORD video_palette_changed = 0;
static UBYTE video_xlate[NUMPALETTES][256]; /* xlate 8-bit to 6-bit EHB */
static BOOL video_is_ehb_mode = FALSE;
static BOOL video_is_native_mode = FALSE;
static BOOL video_is_cyber_mode = FALSE;
static BOOL video_is_using_blitter = FALSE;
static BOOL video_blit_is_in_progress = FALSE;
static BOOL video_use_mmu = FALSE;
static BOOL video_is_directcgx = FALSE;
static BOOL video_doing_fps = FALSE;

static APTR video_bitmap_handle = NULL;
static int video_f_cache_mode;
static int video_c_cache_mode;
static struct RastPort video_temprp;
static struct BitMap video_tmp_bm = {0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}};
static UWORD CHIP emptypointer[] = {
    0x0000, 0x0000, /* reserved, must be NULL */
    0x0000, 0x0000, /* 1 row of image data */
    0x0000, 0x0000  /* reserved, must be NULL */
};
static struct TextFont *video_topaz8font = NULL;

/* keyboard */
static BOOL video_is_rawkey;
static BOOL video_is_using_inputhandler;
static struct Interrupt *input_handler = NULL;
static struct MsgPort *ih_mp = NULL;
static struct IOStdReq *ih_io = NULL;
static BOOL inputhandler_is_open = FALSE;

static struct InputEvent *SAVEDS INTERRUPT video_inputhandler(REG(a0, struct InputEvent *ie),
                                                                      REG(a1, APTR data));
static int xlate[0x68] = {'`',
                          '1',
                          '2',
                          '3',
                          '4',
                          '5',
                          '6',
                          '7',
                          '8',
                          '9',
                          '0',
                          KEY_MINUS,
                          KEY_EQUALS,
                          '\\',
                          0,
                          '0',
                          'q',
                          'w',
                          'e',
                          'r',
                          't',
                          'y',
                          'u',
                          'i',
                          'o',
                          'p',
                          KEY_F11,
                          KEY_F12,
                          0,
                          '0',
                          '2',
                          '3',
                          'a',
                          's',
                          'd',
                          'f',
                          'g',
                          'h',
                          'j',
                          'k',
                          'l',
                          ';',
                          '\'',
                          KEY_ENTER,
                          0,
                          '4',
                          '5',
                          '6',
                          KEY_RSHIFT,
                          'z',
                          'x',
                          'c',
                          'v',
                          'b',
                          'n',
                          'm',
                          ',',
                          '.',
                          '/',
                          0,
                          '.',
                          '7',
                          '8',
                          '9',
                          ' ',
                          KEY_BACKSPACE,
                          KEY_TAB,
                          KEY_ENTER,
                          KEY_ENTER,
                          KEY_ESCAPE,
                          KEY_F11,
                          0,
                          0,
                          0,
                          KEY_MINUS,
                          0,
                          KEY_UPARROW,
                          KEY_DOWNARROW,
                          KEY_RIGHTARROW,
                          KEY_LEFTARROW,
                          KEY_F1,
                          KEY_F2,
                          KEY_F3,
                          KEY_F4,
                          KEY_F5,
                          KEY_F6,
                          KEY_F7,
                          KEY_F8,
                          KEY_F9,
                          KEY_F10,
                          '(',
                          ')',
                          '/',
                          '*',
                          KEY_EQUALS,
                          KEY_PAUSE,
                          KEY_RSHIFT,
                          KEY_RSHIFT,
                          0,
                          KEY_RCTRL,
                          KEY_LALT,
                          KEY_RALT,
                          0,
                          KEY_RCTRL};
static BOOL video_is_using_mouse = FALSE;

/* gameport stuff */
static struct MsgPort *gameport_mp = NULL;
static struct IOStdReq *gameport_io = NULL;
static BOOL gameport_is_open = FALSE;
static BOOL gameport_io_in_progress = FALSE;
static struct InputEvent gameport_ie;
static BYTE gameport_ct; /* controller type */
struct GamePortTrigger gameport_gpt = {
    GPTF_UPKEYS | GPTF_DOWNKEYS, /* gpt_Keys */
    0,                           /* gpt_Timeout */
    1,                           /* gpt_XDelta */
    1                            /* gpt_YDelta */
};

/* SEGA variables */
static ULONG prevSega;
static BOOL sega3_selected = FALSE;
static BOOL sega6_selected = FALSE;

static int video_graffiti = 0;
#ifdef GRAFFITI
struct Library *GraffitiBase = NULL;
static struct GRF_Screen *video_grf_screen = NULL;
#endif

/****************************************************************************/
static ULONG eclocks_per_second; /* EClock frequency in Hz */

static struct EClockVal start_time;
static unsigned int blit_time = 0;
static unsigned int safe_time = 0;
static unsigned int c2p_time = 0;
static unsigned int ccs_time = 0;
static unsigned int wpa8_time = 0;
static unsigned int lock_time = 0;
static unsigned int total_frames = 0;

/****************************************************************************/
static __inline void start_timer(void)
{
    ReadEClock(&start_time);
}
/****************************************************************************/
static __inline unsigned int end_timer(void)
{
    struct EClockVal end_time;

    ReadEClock(&end_time);
    return end_time.ev_lo - start_time.ev_lo;
}

/****************************************************************************/

static char *mode_name(ULONG mode)
/* Return a mode name for given mode, compatible with ASL mode requester */
/* Use name in DisplayInfo database if available */
/* else manually construct the name */
{
    DisplayInfoHandle handle;
    char *p;
    struct NameInfo nameinfo;
    struct DimensionInfo dimsinfo;
    static char modename[DISPLAYNAMELEN + 4 + 4 + 11 + 1];

    p = modename;
    *p = '\0';
    if ((handle = FindDisplayInfo(mode & ~SPRITES)) != NULL) {
        if (GetDisplayInfoData(handle, (UBYTE *)&nameinfo, sizeof(nameinfo), DTAG_NAME, 0) >
            sizeof(struct QueryHeader)) {
            p += sprintf(p, "%s", nameinfo.Name);
        } else if (GetDisplayInfoData(handle, (UBYTE *)&dimsinfo, sizeof(dimsinfo), DTAG_DIMS, 0) >=
                   (ULONG)66 /* sizeof(dimsinfo)*/) {
            switch (mode & MONITOR_ID_MASK) {
            case DEFAULT_MONITOR_ID:
                p += sprintf(p, "DEFAULT:"); /* PAL or NTSC??? */
                break;
            case NTSC_MONITOR_ID:
                p += sprintf(p, "NTSC:");
                break;
            case PAL_MONITOR_ID:
                p += sprintf(p, "PAL:");
                break;
            case VGA_MONITOR_ID:
                p += sprintf(p, "MULTISCAN:");
                break;
            case A2024_MONITOR_ID:
                p += sprintf(p, "A2024:");
                break;
            case PROTO_MONITOR_ID:
                p += sprintf(p, "PROTO:");
                break;
            case EURO72_MONITOR_ID:
                p += sprintf(p, "EURO72:");
                break;
            case EURO36_MONITOR_ID:
                p += sprintf(p, "EURO36:");
                break;
            case SUPER72_MONITOR_ID:
                p += sprintf(p, "SUPER72:");
                break;
            case DBLNTSC_MONITOR_ID:
                p += sprintf(p, "DBLNTSC:");
                break;
            case DBLPAL_MONITOR_ID:
                p += sprintf(p, "DBLPAL:");
                break;
            default:
                p += sprintf(p, "(unknown):");
                break;
            }
            p += sprintf(p, "%d x %d", dimsinfo.Nominal.MaxX - dimsinfo.Nominal.MinX + 1,
                         dimsinfo.Nominal.MaxY - dimsinfo.Nominal.MinY + 1);
            if ((mode & HAM_KEY) != 0)
                p += sprintf(p, " HAM");
            if ((mode & EXTRAHALFBRITE_KEY) != 0)
                p += sprintf(p, " EHB");
            if ((mode & LACE) != 0)
                p += sprintf(p, " Interlaced");
        } else {
            p += sprintf(p, "%s", "(unnamed)");
            if ((mode & HAM_KEY) != 0)
                p += sprintf(p, " HAM");
            if ((mode & EXTRAHALFBRITE_KEY) != 0)
                p += sprintf(p, " EHB");
            if ((mode & LACE) != 0)
                p += sprintf(p, " Interlaced");
        }
    } else {
        p += sprintf(p, "%s", "(unavailable)");
    }
    return (modename);
}

/****************************************************************************/

static ULONG parse_mode(char *modename)
/* Modename may be descriptive name ("PAL Lores"), or hex ("$00420001" or */
/* "0x12345678") or decimal ("32768"). */
{
    ULONG mode, reason;

    mode = INVALID_ID;
    if (modename != NULL) {
        if (modename[0] == '0' && modename[1] == 'x') {
            if (sscanf(&modename[2], "%lx", &mode) != 1)
                mode = INVALID_ID;
        } else if (modename[0] == '$') {
            if (sscanf(&modename[1], "%lx", &mode) != 1)
                mode = INVALID_ID;
        } else if (modename[0] >= '0' && modename[0] <= '9') {
            if (sscanf(modename, "%ld", &mode) != 1)
                mode = INVALID_ID;
        } else {
            while ((mode = NextDisplayInfo(mode)) != INVALID_ID) {
                if ((mode & LORESDPF_KEY) == 0) {
                    /* printf ("$%08x  \"%s\"\n", mode, mode_name (mode)); */
                    if (strcmp(mode_name(mode), modename) == 0)
                        break;
                }
            }
        }
    }
    if (FindDisplayInfo(mode) == NULL)
        I_Error("ScreenMode not in database: \"%s\"", modename);
    if ((reason = ModeNotAvailable(mode)) != 0)
        I_Error("Mode $%08x is not available: %ld", mode, reason);
    return (mode);
}

/**********************************************************************/
static void video_do_fps(struct RastPort *rp, int yoffset)
{
    ULONG x;
    static struct EClockVal start_time = {0, 0};
    struct EClockVal end_time;
    char msg[4];

    ReadEClock(&end_time);
    x = end_time.ev_lo - start_time.ev_lo;
    if (x != 0) {
        x = (eclocks_per_second + (x >> 1)) / x; /* round to nearest */
        msg[0] = (x % 1000) / 100 + '0';
        msg[1] = (x % 100) / 10 + '0';
        msg[2] = (x % 10) + '0';
        msg[3] = '\0';
        Move(rp, SCREENWIDTH - 24, yoffset + 6);
        Text(rp, msg, 3);
    }
    start_time = end_time;
}

/**********************************************************************/
// This asynchronous task waits in a loop for sigbit3 signals.
// Sigbit3 is signalled when the next frame is just finished.
// Sigbit3 may be signalled from the qblit() cleanup function or from the
// main task.
// On sigbit3, we load the new palette and call ChangeScreenBuffer().
// This way there is no delay calling ChangeScreenBuffer() after the last blit
// has finished.
// The main 3D rendering task may be interrupted to call ChangeScreenBuffer()
// SIGBREAKF_CTRL_F and SIGBREAKF_CTRL_C are used for synchronisation
// with the main task.

static void SAVEDS INTERRUPT video_flipscreentask(void)
{
    ULONG sig;
    struct MsgPort *video_dispport, *video_safeport;
    BOOL going, video_disp, video_safe;
    int i;

    video_sigbit3 = AllocSignal(-1);
    Signal(video_maintask, SIGBREAKF_CTRL_F);
    video_dispport = CreatePort(NULL, 0);
    video_safeport = CreatePort(NULL, 0);
    video_disp = TRUE;
    video_safe = TRUE;
    for (i = 0; i < 2; i++) {
        video_sb[i]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = video_dispport;
        video_sb[i]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = video_safeport;
    }
    going = (video_sigbit3 != -1) && (video_dispport != NULL) && (video_safeport != NULL);
    while (going) {
        sig = Wait(1 << video_sigbit3 | SIGBREAKF_CTRL_C);
        if ((sig & (1 << video_sigbit3)) != 0) {
            if (video_doing_fps)
                video_do_fps(&video_rastport[video_which], 0);
            if (video_palette_changed > 0) {
                LoadRGB32(&video_screen->ViewPort, video_colourtable[video_palette_index]);
                video_palette_changed--; /* keep it set for 2 frames for dblbuffing */
            }
            if ((ChangeScreenBuffer(video_screen, video_sb[video_which])) != 0) {
                video_disp = FALSE;
                video_safe = FALSE;
            }
            if (!video_safe) { /* wait until safe */
                Wait(1 << video_safeport->mp_SigBit);
                while (GetMsg(video_safeport) != NULL) /* clear message queue */
                    /* do nothing */;
                video_safe = TRUE;
            }
            Signal(video_maintask, SIGBREAKF_CTRL_F);
            if (!video_disp) { /* wait for previous frame to be displayed */
                Wait(1 << video_dispport->mp_SigBit);
                while (GetMsg(video_dispport) != NULL) /* clear message queue */
                    /* do nothing */;
                video_disp = TRUE;
            }
        }
        if ((sig & SIGBREAKF_CTRL_C) != 0) {
            going = FALSE;
        }
    }
    if (!video_safe) { /* wait for safe */
        Wait(1 << video_safeport->mp_SigBit);
        while (GetMsg(video_safeport) != NULL) /* clear message queue */
            /* do nothing */;
        video_safe = TRUE;
    }
    if (!video_disp) { /* wait for last frame to be displayed */
        Wait(1 << video_dispport->mp_SigBit);
        while (GetMsg(video_dispport) != NULL) /* clear message queue */
            /* do nothing */;
        video_disp = TRUE;
    }
    if (video_sigbit3 != -1)
        FreeSignal(video_sigbit3);
    if (video_dispport != NULL)
        DeletePort(video_dispport);
    if (video_safeport != NULL)
        DeletePort(video_safeport);
    Signal(video_maintask, SIGBREAKF_CTRL_F);
    Wait(0);
}

/**********************************************************************/
// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode

void I_InitGraphics(void)
{
    int i, retries, config_done;
    ULONG propertymask, idcmp, wflags, width, pixfmt;
    struct Rectangle rect;
    char reqtitle[32];
    int mode, depth, nbytes, p;
    DisplayInfoHandle handle;
    struct DisplayInfo dispinfo;
    struct DimensionInfo dimsinfo;
    static struct TextAttr topaz8 = {"topaz.font", 8, FS_NORMAL, FPF_ROMFONT};

    DEBUGSTEP();

    video_maintask = FindTask(NULL);

    DEBUGSTEP();

    if ((KeymapBase = OpenLibrary("keymap.library", 0)) == NULL)
        I_Error("Can't open keymap.library");

    DEBUGSTEP();

    if ((GfxBase = (struct GfxBase*)OpenLibrary("graphics.library", 0)) == NULL)
        I_Error("Can't open graphics.library");

    if ((IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library", 0)) == NULL)
        I_Error("Can't open intuition.library");

    DEBUGSTEP();

    if ((video_topaz8font = OpenFont(&topaz8)) == NULL)
        I_Error("Can't open topaz8 font");

    DEBUGSTEP();

    OpenDevice("timer.device", UNIT_ECLOCK, &timereq, 0);
    
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, &timereq, 0))
        I_Error("Can't open timer.device!");

    TimerBase = timereq.io_Device;
    
    eclocks_per_second = ReadEClock(&start_time);

    video_doing_fps = M_CheckParm("-fps");

    DEBUGSTEP();

#ifdef GRAFFITI
    if (video_graffiti != 0) {
        if ((GraffitiBase = OpenLibrary("graffiti.library", 0)) == NULL)
            I_Error("Can't open graffiti.library");
        if ((video_grf_screen = OpenChunkyScreen(video_graffiti)) == NULL)
            I_Error("graffiti.library OpenChunkyScreen() failed");
        video_screen = (struct Screen *)video_grf_screen->GRF_ScreenID;
        video_depth = 8;
    } else {
#endif
#ifdef INDIVISION
        if (video_indigfx != 0) {
            printf("Indivision GFX screen: %d x %d (8bit)\n", SCREENWIDTH, SCREENHEIGHT);

            /* Then, open a dummy screen with one bitplane
               to minimize interference from chipset DMA    */

            video_depth = 1;

            for (i = 0; i < (3); i++) {
                if ((video_raster[i] = (PLANEPTR)AllocRaster(SCREENWIDTH, video_depth * SCREENHEIGHT)) == NULL)
                    I_Error("AllocRaster() for Indivision GFX failed");

                memset(video_raster[i], 0, video_depth * RASSIZE(SCREENWIDTH, SCREENHEIGHT));
                InitBitMap(&video_bitmap[i], video_depth, SCREENWIDTH, SCREENHEIGHT);
                video_bitmap[i].Planes[0] = video_raster[i];
                InitRastPort(&video_rastport[i]);
                video_rastport[i].BitMap = &video_bitmap[i];

                SetAPen(&video_rastport[i], 1);
                SetBPen(&video_rastport[i], 0);
                SetDrMd(&video_rastport[i], JAM2);
                SetFont(&video_rastport[i], video_topaz8font);
            }

            if ((video_screen =
                     OpenScreenTags(NULL, SA_Type, CUSTOMSCREEN | CUSTOMBITMAP, SA_DisplayID, 0x00000000, SA_DClip,
                                    (ULONG)&rect, SA_Width, SCREENWIDTH, SA_Height, SCREENHEIGHT, SA_Depth, video_depth,
                                    SA_Font, &topaz8, SA_Draggable, FALSE, SA_AutoScroll, FALSE, SA_Exclusive, TRUE,
                                    SA_Quiet, TRUE, SA_BitMap, &video_bitmap[0], /* custom bitmap, contiguous planes */
                                    TAG_DONE, 0)) == NULL) {
                I_Error("OpenScreen() for Indivision GFX failed");
            }

            /* Indivision GFX initialization */
            /* First, intialize Indivision GFX core */

            retries = 16;
            config_done = 0;

            // Apologies for the crude way to kick Indivision ECS safely into GFX mode

            while ((retries > 0) && (config_done == 0)) {
                indivision_core(0xdff1f0, 0x3c00);  // $3c00 = Indivision GFX Core

                i = TimeDelay(UNIT_VBLANK, 1, 0);  // 1 second to configure FPGA

                indivision_waitconfigdone(0xdff1f0);  // Wait for FPGA startup
                if (indivision_checkcore(0xdff0ac, 0x3000) != 0)
                    config_done = 1;
                else
                    retries--;
            }

            if (retries == 0) {
                video_indigfx = 0;
                I_Error("could not init Indivision GFX hardware\n");
            }

            /* Now initialize Indivision GFX core
               for 8 bit direct chunky graphics  */

            i = TimeDelay(UNIT_VBLANK, 2, 0);
            indivision_initscreen(0xdff0ac, video_indigfx - 1);
        } else
#endif
        {
            DEBUGSTEP();

            if (AslBase == NULL) {
                if ((AslBase = OpenLibrary("asl.library", 37L)) == NULL ||
                    (video_smr = AllocAslRequestTags(ASL_ScreenModeRequest, TAG_DONE)) == NULL) {
                    I_Error(
                        "OpenLibrary("
                        "asl.library"
                        ", 37) failed");
                }
            }
            CyberGfxBase = OpenLibrary("cybergraphics.library", 0); /* may be NULL */

            /* first check tooltypes for SCREENMODE */
            mode = -1;
            p = M_CheckParm("-screenmode");
            if (p && p < myargc - 1) {
                mode = parse_mode(myargv[p + 1]);
            }

            /* if not found in icon tooltypes, then put up a ScreenMode requester */
            if (mode == -1) {
                propertymask = /* DIPF_IS_EXTRAHALFBRITE | */ DIPF_IS_DUALPF | DIPF_IS_PF2PRI | DIPF_IS_HAM;
                if (CyberGfxBase != NULL)
                    mode = BestCModeIDTags(CYBRBIDTG_NominalWidth, SCREENWIDTH, CYBRBIDTG_NominalHeight, SCREENHEIGHT,
                                           CYBRBIDTG_Depth, 8, TAG_DONE);
                else if (GfxBase->LibNode.lib_Version >= 39)
                    mode = BestModeID(BIDTAG_NominalWidth, SCREENWIDTH, BIDTAG_NominalHeight, SCREENHEIGHT,
                                      BIDTAG_Depth, video_depth, BIDTAG_DIPFMustNotHave, propertymask, TAG_DONE);
                else
                    mode = EXTRAHALFBRITE_KEY;
                if (AslBase->lib_Version >= 38) {
                    sprintf(reqtitle, "ADoom %dx%d", SCREENWIDTH, SCREENHEIGHT);
                    if (!AslRequestTags(video_smr, ASLSM_TitleText, (ULONG)reqtitle, ASLSM_InitialDisplayID, mode,
                                        ASLSM_MinWidth, SCREENWIDTH, ASLSM_MinHeight, SCREENHEIGHT, ASLSM_MaxWidth,
                                        MAXSCREENWIDTH, ASLSM_MaxHeight, MAXSCREENHEIGHT, ASLSM_MinDepth, 6,
                                        ASLSM_MaxDepth, 8, ASLSM_PropertyMask, propertymask, ASLSM_PropertyFlags, 0,
                                        TAG_DONE)) {
                        I_Error("AslRequest() failed");
                    }
                    mode = video_smr->sm_DisplayID;
                }
            }

            DEBUGSTEP();

            if ((handle = FindDisplayInfo(mode)) == NULL) {
                I_Error("Can't FindDisplayInfo() for mode %08x", mode);
            }

            DEBUGSTEP();

            if ((nbytes = GetDisplayInfoData(handle, (UBYTE *)&dispinfo, sizeof(struct DisplayInfo), DTAG_DISP, 0)) <
                40 /*sizeof(struct DisplayInfo)*/) {
                I_Error("Can't GetDisplayInfoData() for mode %08x, got %d bytes", mode, nbytes);
            }
            if ((nbytes = GetDisplayInfoData(handle, (UBYTE *)&dimsinfo, sizeof(dimsinfo), DTAG_DIMS, 0)) <
                66 /* sizeof(dimsinfo)*/) {
                I_Error("Can't GetDisplayInfoData() for mode %08x, got %d bytes", mode, nbytes);
            }

            video_oscan_height = dimsinfo.MaxOScan.MaxY - dimsinfo.MaxOScan.MinY + 1;

            video_is_cyber_mode = 0;
            if (CyberGfxBase != NULL)
                video_is_cyber_mode = IsCyberModeID(mode);

            /* this test needs improving */
            video_is_native_mode =
                ((GfxBase->LibNode.lib_Version < 39 || (dispinfo.PropertyFlags & DIPF_IS_EXTRAHALFBRITE) != 0 ||
                  (dispinfo.PropertyFlags & DIPF_IS_AA) != 0 || (dispinfo.PropertyFlags & DIPF_IS_ECS) != 0 ||
                  (dispinfo.PropertyFlags & DIPF_IS_DBUFFER) != 0) &&
                 !video_is_cyber_mode && (dispinfo.PropertyFlags & DIPF_IS_FOREIGN) == 0);

            video_is_ehb_mode = ((dispinfo.PropertyFlags & DIPF_IS_EXTRAHALFBRITE) != 0);

            DEBUGSTEP();

            /* manual overrides */
            if (M_CheckParm("-rtg"))
                video_is_native_mode = FALSE;
            else if (M_CheckParm("-native"))
                video_is_native_mode = TRUE;
            if (M_CheckParm("-ehb"))
                video_is_ehb_mode = TRUE;

            printf("Screen Mode $%08x is ", mode);
            if (video_is_native_mode)
                printf(" NATIVE-PLANAR");
            else
                printf(" FOREIGN");
            if (video_is_ehb_mode)
                printf(" EXTRAHALFBRITE");
            else
                printf(" 8-BIT");
            if (video_is_cyber_mode)
                printf(" CYBERGRAPHX");
            printf("\n");

            if (video_is_ehb_mode)
                video_depth = 6;
            else
                video_depth = 8;

            rect.MinX = 0;
            rect.MinY = 0;
            rect.MaxX = SCREENWIDTH - 1;
            rect.MaxY = SCREENHEIGHT - 1;

            DEBUGSTEP();

            /* use the mmu hack only if cpu is 68040+ and user specified -mmu */
            if (M_CheckParm("-mmu") && cpu_type >= 68040)
                video_use_mmu = true;

            if (video_is_native_mode) {
                video_is_using_blitter = (cpu_type < 68040);

                for (i = 0; i < (video_is_using_blitter ? 2 : 3); i++) {
                    if (video_use_mmu) {
                        ULONG *ptr, mem;
                        mem = (ULONG)AllocVec(SCREENWIDTH * SCREENHEIGHT / 8 * video_depth + 4100, MEMF_CHIP);
                        if (!mem)
                            I_Error("AllocVec() failed");
                        ptr = (ULONG *)((mem & ~0xfff) + 4096);
                        ptr[-1] = 4096 - (mem & 0xfff);
                        video_raster[i] = (byte *)ptr;
                        video_c_cache_mode = mmu_mark(video_raster[i], SCREENWIDTH * SCREENHEIGHT / 8 * video_depth,
                                                      CM_IMPRECISE, SysBase);
                    } else {
                        if ((video_raster[i] = (PLANEPTR)AllocRaster(SCREENWIDTH, video_depth * SCREENHEIGHT)) == NULL)
                            I_Error("AllocRaster() failed");
                    }
                    memset(video_raster[i], 0, video_depth * RASSIZE(SCREENWIDTH, SCREENHEIGHT));
                    InitBitMap(&video_bitmap[i], video_depth, SCREENWIDTH, SCREENHEIGHT);
                    for (depth = 0; depth < video_depth; depth++)
                        video_bitmap[i].Planes[depth] = video_raster[i] + depth * RASSIZE(SCREENWIDTH, SCREENHEIGHT);
                    InitRastPort(&video_rastport[i]);
                    video_rastport[i].BitMap = &video_bitmap[i];
                    SetAPen(&video_rastport[i], (1 << video_depth) - 1);
                    SetBPen(&video_rastport[i], 0);
                    SetDrMd(&video_rastport[i], JAM2);
                    SetFont(&video_rastport[i], video_topaz8font);
                }

                /* experimental c2p stuff */
                if (!video_is_using_blitter)
                    c2p = c2p8_reloc(screens[0], &video_bitmap[0], SysBase);

                DEBUGSTEP();

                if ((video_screen = OpenScreenTags(
                         NULL, SA_Type, CUSTOMSCREEN | CUSTOMBITMAP, SA_DisplayID, mode, SA_DClip, (ULONG)&rect,
                         SA_Width, SCREENWIDTH, SA_Height, SCREENHEIGHT, SA_Depth, video_depth, SA_Font, (ULONG)&topaz8,
                         /* SA_Draggable,FALSE, */
                         /* SA_AutoScroll,FALSE, */
                         /* SA_Exclusive,TRUE, */
                         SA_Quiet, TRUE, SA_BitMap, (ULONG)&video_bitmap[0], /* custom bitmap, contiguous planes */
                         TAG_DONE, 0)) == NULL) {
                    I_Error("OpenScreen() failed");
                }
                for (i = 0; i < (video_is_using_blitter ? 2 : 3); i++) {
                    if ((video_compare_buffer[i] = malloc(SCREENWIDTH * SCREENHEIGHT)) == NULL)
                        I_Error("Out of memory allocating %d bytes", SCREENWIDTH * SCREENHEIGHT);
                    memset(video_compare_buffer[i], 0, SCREENWIDTH * SCREENHEIGHT);
                }
                if (video_is_using_blitter) {
                    for (i = 0; i < 2; i++) {
                        if ((video_sb[i] = AllocScreenBuffer(video_screen, &video_bitmap[i], 0)) == NULL)
                            I_Error("Can't allocate structure for double buffering");
                    }
                } else {
                    video_db = AllocDBufInfo(&video_screen->ViewPort);
                    video_mp = CreateMsgPort();
                    if (!video_db || !video_mp)
                        I_Error("Can't allocate dbuffer information");
                    video_db->dbi_DispMessage.mn_ReplyPort = video_mp;
                    video_which = 1;
                    ChangeVPBitMap(&video_screen->ViewPort, &video_bitmap[0], video_db);
                }

                DEBUGSTEP();
            } else { /* non-native ScreenMode */
                if (video_is_cyber_mode && M_CheckParm("-directcgx"))
                    video_is_directcgx = TRUE;
                if ((video_screen = OpenScreenTags(NULL, SA_Type, CUSTOMSCREEN, SA_DisplayID, mode, SA_DClip,
                                                   (ULONG)&rect, SA_Width, SCREENWIDTH, SA_Height,
                                                   video_is_directcgx ? video_oscan_height << 1 : SCREENHEIGHT,
                                                   SA_Depth, video_depth, SA_Font, (ULONG)&topaz8,
                                                   /* SA_Draggable,FALSE, */
                                                   /* SA_AutoScroll,FALSE, */
                                                   /* SA_Exclusive,TRUE, */
                                                   SA_Quiet, TRUE, TAG_DONE, 0)) == NULL) {
                    I_Error("OpenScreen() failed");
                }
            }

            if (video_is_directcgx) {
                video_bitmap_handle = LockBitMapTags(video_screen->ViewPort.RasInfo->BitMap, LBMI_WIDTH, (ULONG)&width,
                                                     LBMI_DEPTH, (ULONG)&depth, LBMI_PIXFMT, (ULONG)&pixfmt,
                                                     LBMI_BASEADDRESS, (ULONG)&screens[0], TAG_DONE);
                UnLockBitMap(video_bitmap_handle);
                video_bitmap_handle = NULL;
                screens[0] = NULL;
                if (/* width != SCREENWIDTH || */
                    depth != video_depth || pixfmt != PIXFMT_LUT8) {
                    I_Error("ScreenMode of width %d, depth %d, pixfmt %d cannot be used with -directcgx", width, depth,
                            pixfmt);
                }
            } else {
                if (video_use_mmu) {
                    video_f_cache_mode =
                        mmu_mark(screens[0], (SCREENWIDTH * SCREENHEIGHT + 4096) & ~0xfff, CM_WRITETHROUGH, SysBase);
                }
            }
        }
#ifdef GRAFFITI
    }
#endif

    DEBUGSTEP();

    video_is_using_inputhandler = M_CheckParm("-inputhandler");
    if (video_is_using_inputhandler)
        idcmp = 0;
    else
        idcmp = IDCMP_RAWKEY;
    wflags = WFLG_ACTIVATE | WFLG_BORDERLESS | WFLG_RMBTRAP | WFLG_NOCAREREFRESH | WFLG_SIMPLE_REFRESH;
    if (M_CheckParm("-mouse")) {
        video_is_using_mouse = TRUE;
        if (!video_is_using_inputhandler) {
            idcmp |= IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE | IDCMP_MOUSEBUTTONS;
            wflags |= WFLG_REPORTMOUSE;
        }
    }

    DEBUGSTEP();

    if ((video_window = OpenWindowTags(NULL, WA_Left, 0, WA_Top, 0, WA_Width, video_graffiti != 0 ? 640 : SCREENWIDTH,
                                       WA_Height, video_is_directcgx ? video_oscan_height << 1 : SCREENHEIGHT, WA_IDCMP,
                                       idcmp, WA_Flags, wflags, WA_CustomScreen, (ULONG)video_screen, TAG_DONE)) ==
        NULL) {
        I_Error("OpenWindow() failed");
    }

    DEBUGSTEP();

    InitBitMap(&video_tmp_bm, video_depth, SCREENWIDTH, 1);
    for (depth = 0; depth < video_depth; depth++)
        if ((video_tmp_bm.Planes[depth] = (PLANEPTR)AllocRaster(SCREENWIDTH, 1)) == NULL)
            I_Error("AllocRaster() failed");
    video_temprp = *video_window->RPort;
    video_temprp.Layer = NULL;
    video_temprp.BitMap = &video_tmp_bm;

    DEBUGSTEP();

    if (video_is_native_mode) {
        if (video_is_using_blitter) {
            if ((video_sigbit1 = AllocSignal(-1)) == -1 || (video_sigbit2 = AllocSignal(-1)) == -1)
                I_Error("Can't allocate signal!\n");
            Signal(video_maintask, (1 << video_sigbit1) | (1 << video_sigbit2));
            /* initial state is finished */
            if ((video_fliptask = CreateTask("ADoom_flipscreen", 100, video_flipscreentask, 4096)) == NULL)
                I_Error("Can't create subtask");
            Wait(SIGBREAKF_CTRL_F);
            if (video_sigbit3 == -1)
                I_Error("Subtask couldn't allocate sigbit");
            if ((video_chipbuff = AllocMem(2 * SCREENWIDTH * SCREENHEIGHT, MEMF_CHIP | MEMF_CLEAR)) == NULL)
                I_Error("Out of CHIP memory allocating %d bytes", 2 * SCREENWIDTH * SCREENHEIGHT);

            DEBUGSTEP();
            if (!video_is_ehb_mode)
                printf("c2p1x1_cpu3blit1_queue_init %p", &c2p1x1_cpu3blit1_queue_init); fflush(stdout);
                c2p1x1_cpu3blit1_queue_init(SCREENWIDTH, SCREENHEIGHT, 0, ((unsigned)SCREENWIDTH * SCREENHEIGHT) / 8,
                                            1 << video_sigbit1, 1 << video_sigbit3, video_maintask, video_fliptask,
                                            video_chipbuff);
        }
    }

    DEBUGSTEP();

    if (!M_CheckParm("-mousepointer"))
        SetPointer(video_window, emptypointer, 1, 16, 0, 0);

    DEBUGSTEP();

    I_RecalcPalettes();

    /* keyboard & joystick initialisation */

    video_is_rawkey = M_CheckParm("-rawkey");

    if (video_is_using_inputhandler) {
        if ((ih_mp = CreatePort(NULL, 0)) == NULL)
            I_Error("CreatePort() failed");
        if ((ih_io = (struct IOStdReq *)CreateExtIO(ih_mp, sizeof(struct IOStdReq))) == NULL)
            I_Error("CreateExtIO() failed");
        if (OpenDevice("input.device", 0, (struct IORequest *)ih_io, 0) != 0)
            I_Error("OpenDevice(\"input.device\") failed");
        inputhandler_is_open = TRUE;
        if ((input_handler = (struct Interrupt *)AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
            I_Error("AllocMem() failed");
        input_handler->is_Code = (void (*)(void))video_inputhandler;
        input_handler->is_Data = NULL;
        input_handler->is_Node.ln_Pri = 127;
        input_handler->is_Node.ln_Name = "ADoom_InputHandler";
        ih_io->io_Data = input_handler;
        ih_io->io_Command = IND_ADDHANDLER;
        DoIO((struct IORequest *)ih_io);
        xlate[0x66] = KEY_RCTRL;
    }

    if (M_CheckParm("-sega3"))
        sega3_selected = TRUE;

    if (M_CheckParm("-sega6"))
        sega6_selected = TRUE;

    if (M_CheckParm("-joypad")) {
        if ((LowLevelBase = OpenLibrary("lowlevel.library", 0)) == NULL)
            I_Error("-joypad option specified and can't open lowlevel.library");
    } else {
        DEBUGSTEP();

        if ((gameport_mp = CreatePort(NULL, 0)) == NULL ||
            (gameport_io = (struct IOStdReq *)CreateExtIO(gameport_mp, sizeof(struct IOStdReq))) == NULL ||
            OpenDevice("gameport.device", 1, (struct IORequest *)gameport_io, 0) != 0)
            I_Error("Can't open gameport.device");

        gameport_is_open = TRUE;

        Forbid();

        gameport_io->io_Command = GPD_ASKCTYPE;
        gameport_io->io_Length = 1;
        gameport_io->io_Data = &gameport_ct;
        DoIO((struct IORequest *)gameport_io);

        if (gameport_ct != GPCT_NOCONTROLLER) {
            Permit();
            fprintf(stderr, "Another task is using the gameport!  Joystick disabled");
            CloseDevice((struct IORequest *)gameport_io);
            gameport_is_open = FALSE;
        } else {
            gameport_ct = GPCT_ABSJOYSTICK;
            gameport_io->io_Command = GPD_SETCTYPE;
            gameport_io->io_Length = 1;
            gameport_io->io_Data = &gameport_ct;
            DoIO((struct IORequest *)gameport_io);

            Permit();

            gameport_io->io_Command = GPD_SETTRIGGER;
            gameport_io->io_Length = sizeof(struct GamePortTrigger);
            gameport_io->io_Data = &gameport_gpt;
            DoIO((struct IORequest *)gameport_io);

            gameport_io->io_Command = GPD_READEVENT;
            gameport_io->io_Length = sizeof(struct InputEvent);
            gameport_io->io_Data = &gameport_ie;
            SendIO((struct IORequest *)gameport_io);
            gameport_io_in_progress = TRUE;
        }
    }
}

/**********************************************************************/
// void I_SetColour (int n, UBYTE r, UBYTE g, UBYTE b)
//{
//  SetRGB32 (&video_screen->ViewPort, n, r * 0x01010101u, g * 0x01010101u, b * 0x01010101u);
//}
//
/**********************************************************************/
void I_ShutdownGraphics(void)
{
    int depth, i, config_done;

#ifdef INDIVISION
    if (video_indigfx != 0) {
        config_done = 0;

        while (config_done == 0) {
            indivision_core(0xdff1f0, 0x0c00);
            i = TimeDelay(UNIT_VBLANK, 1, 0);
            indivision_waitconfigdone(0xdff1f0);
            if (indivision_checkcore(0xdff0ac, 0x1000) != 0)
                config_done = 1;
        }
        i = TimeDelay(UNIT_VBLANK, 1, 0);
    }
#endif
    if (video_screen != NULL && video_is_directcgx) {
        if (video_bitmap_handle != NULL) {
            UnLockBitMap(video_bitmap_handle);
            video_bitmap_handle = NULL;
            screens[0] = NULL;
        }
        video_screen->ViewPort.RasInfo->RyOffset = 0;
        ScrollVPort(&video_screen->ViewPort);
    }
    if (LowLevelBase != NULL) {
        CloseLibrary(LowLevelBase);
        LowLevelBase = NULL;
    }
    if (gameport_io_in_progress) {
        AbortIO((struct IORequest *)gameport_io);
        WaitIO((struct IORequest *)gameport_io);
        gameport_io_in_progress = FALSE;
        gameport_ct = GPCT_NOCONTROLLER;
        gameport_io->io_Command = GPD_SETCTYPE;
        gameport_io->io_Length = 1;
        gameport_io->io_Data = &gameport_ct;
        DoIO((struct IORequest *)gameport_io);
    }
    if (gameport_is_open) {
        CloseDevice((struct IORequest *)gameport_io);
        gameport_is_open = FALSE;
    }
    if (gameport_io != NULL) {
        DeleteExtIO((struct IORequest *)gameport_io);
        gameport_io = NULL;
    }
    if (gameport_mp != NULL) {
        DeletePort(gameport_mp);
        gameport_mp = NULL;
    }
    if (input_handler != NULL) {
        ih_io->io_Data = input_handler;
        ih_io->io_Command = IND_REMHANDLER;
        DoIO((struct IORequest *)ih_io);
        FreeMem(input_handler, sizeof(struct Interrupt));
        input_handler = NULL;
    }
    if (inputhandler_is_open) {
        CloseDevice((struct IORequest *)ih_io);
        inputhandler_is_open = FALSE;
    }
    if (ih_io != NULL) {
        DeleteExtIO((struct IORequest *)ih_io);
        ih_io = NULL;
    }
    if (ih_mp != NULL) {
        DeletePort(ih_mp);
        ih_mp = NULL;
    }
    if (video_blit_is_in_progress) {
        Wait(SIGBREAKF_CTRL_F);
        video_blit_is_in_progress = FALSE;
    }
    if (video_fliptask != NULL) {
        Signal(video_fliptask, SIGBREAKF_CTRL_C);
        Wait(SIGBREAKF_CTRL_F);
        DeleteTask(video_fliptask);
        video_fliptask = NULL;
    }
    if (video_is_using_blitter) {
        if (video_sigbit1 != -1) {
            Wait(1 << video_sigbit1);  // wait for last c2p8 to finish pass 3
            FreeSignal(video_sigbit1);
            video_sigbit1 = -1;
        }
        if (video_sigbit2 != -1) {
            Wait(1 << video_sigbit2);  // wait for last c2p8 to completely finish
            FreeSignal(video_sigbit2);
            video_sigbit2 = -1;
        }
    }
    if (video_window != NULL) {
        ClearPointer(video_window);
        CloseWindow(video_window);
        video_window = NULL;
    }
    if (video_is_using_blitter) {
        for (i = 0; i < 2; i++) {
            if (video_sb[i] != NULL) {
                FreeScreenBuffer(video_screen, video_sb[i]);
                video_sb[i] = NULL;
            }
        }
    } else {
        if (video_mp) {
            WaitPort(video_mp);
            while (GetMsg(video_mp))
                ;
            DeleteMsgPort(video_mp);
            video_mp = NULL;
        }
        if (video_db) {
            FreeDBufInfo(video_db);
            video_db = NULL;
        }
    }
#ifdef GRAFFITI
    if (video_grf_screen != NULL) {
        CloseChunkyScreen(video_grf_screen);
        video_grf_screen = NULL;
        video_screen = NULL;
    }
    if (GraffitiBase != NULL) {
        CloseLibrary(GraffitiBase);
        GraffitiBase = NULL;
    }
#endif
    if (video_screen != NULL) {
        CloseScreen(video_screen);
        video_screen = NULL;
    }
    for (depth = 0; depth < video_depth; depth++) {
        if (video_tmp_bm.Planes[depth] != NULL) {
            FreeRaster(video_tmp_bm.Planes[depth], SCREENWIDTH, 1);
            video_tmp_bm.Planes[depth] = NULL;
        }
    }
    if (video_chipbuff != NULL) {
        FreeMem(video_chipbuff, 2 * SCREENWIDTH * SCREENHEIGHT);
        video_chipbuff = NULL;
    }
    for (i = 0; i < (video_is_using_blitter ? 2 : 3); i++) {
        if (video_raster[i] != NULL) {
            if (video_use_mmu) {
                ULONG *ptr = (ULONG *)video_raster[i];
                UBYTE *p2 = (UBYTE *)ptr;
                mmu_mark(video_raster[i], SCREENWIDTH * SCREENHEIGHT / 8 * video_depth, video_c_cache_mode, SysBase);
                FreeVec(p2 - ptr[-1]);
            } else
                FreeRaster(video_raster[i], SCREENWIDTH, video_depth * SCREENHEIGHT);
            video_raster[i] = NULL;
        }
        if (video_compare_buffer[i] != NULL) {
            free(video_compare_buffer[i]);
            video_compare_buffer[i] = NULL;
        }
    }
    if (video_use_mmu && !video_is_directcgx) {
        if (screens[0] != NULL)
            mmu_mark(screens[0], (SCREENWIDTH * SCREENHEIGHT + 4096) & ~0xfff, video_f_cache_mode, SysBase);
    }
    if (KeymapBase == NULL) {
        CloseLibrary(KeymapBase);
        KeymapBase = NULL;
    }
    if (CyberGfxBase == NULL) {
        CloseLibrary(CyberGfxBase);
        CyberGfxBase = NULL;
    }
    /* experimental c2p stuff */
    if (c2p && !video_is_using_blitter) {
        c2p8_deinit(c2p, SysBase);
        c2p = NULL;
    }
    if (video_topaz8font != NULL) {
        CloseFont(video_topaz8font);
        video_topaz8font = NULL;
    }
    if (TimerBase)
        CloseDevice(&timereq);
}

/**********************************************************************/
// recalculate colourtable[][] after changing usegamma
void I_RecalcPalettes(void)
{
    int p, i;
    byte *palette;
    static UBYTE gpalette[3 * 256];
    static int lu_palette;

    DEBUGSTEP();
    lu_palette = W_GetNumForName("PLAYPAL");
    for (p = 0; p < NUMPALETTES; p++) {
        palette = (byte *)W_CacheLumpNum(lu_palette, PU_STATIC) + p * 768;

        if (video_is_ehb_mode) {
            i = 3 * 256 - 1;
            do {
                gpalette[i] = gammatable[usegamma][palette[i]];
            } while (--i >= 0);
            video_colourtable[p][0] = (32 << 16) + 0;
            median_cut(gpalette, &video_colourtable[p][1], video_xlate[p]);
            video_colourtable[p][33] = 0;
        } else {
#ifdef GRAFFITI
            if (video_graffiti != 0) {
                for (i = 0; i < 256; i++)
                    video_colourtable[p][i] = (((UBYTE)gammatable[usegamma][palette[3 * i + 0]]) << 16) +
                                              (((UBYTE)gammatable[usegamma][palette[3 * i + 1]]) << 8) +
                                              ((UBYTE)gammatable[usegamma][palette[3 * i + 2]]);
            } else {
#endif
#ifdef INDIVISION
                if (video_indigfx != 0) {
                    // Indivision GFX uses 15bit palette
                    for (i = 0; i < 128; i++)
                        video_colourtable[p][ptranslate[i]] =
                            ((((UBYTE)(gammatable[usegamma][palette[6 * i + 3]]) & 0xf8) >> 3) |
                             (((UBYTE)(gammatable[usegamma][palette[6 * i + 4]]) & 0xf8) << 2) |
                             (((UBYTE)(gammatable[usegamma][palette[6 * i + 5]]) & 0xf8) << 7) |

                             (((UBYTE)(gammatable[usegamma][palette[6 * i + 0]]) & 0xf8) << 13) |
                             (((UBYTE)(gammatable[usegamma][palette[6 * i + 1]]) & 0xf8) << 18) |
                             (((UBYTE)(gammatable[usegamma][palette[6 * i + 2]]) & 0xf8) << 23));
                } else
#endif
                {
                    DEBUGSTEP();
                    i = 3 * 256 - 1;
                    video_colourtable[p][i + 2] = 0; // finish the list
                    do {
                        // Better to define c locally here instead of for the whole function:
                        ULONG c = gammatable[usegamma][palette[i]];
                        c += (c << 8);
                        video_colourtable[p][i + 1] = (c << 16) + c;
                    } while (--i >= 0);
                    //encode ""change 256 colors starting at palette index 0
                    video_colourtable[p][0] = (256 << 16) + 0;
                }
#ifdef GRAFFITI
            }
#endif
        }
    }
}

/**********************************************************************/
// Takes full 8 bit values.
void I_SetPalette(byte *palette, int palette_index)
{
    if (video_is_ehb_mode)
        if (video_is_using_blitter) {
            video_palette_changed = 2; /* double buffering */
            video_pending_palette_index = palette_index;
        } else {
            video_palette_changed = 3; /* triple buffering */
            video_palette_index = palette_index;
        }
    else {
        video_palette_changed = 1;
        video_palette_index = palette_index;
    }
}

/**********************************************************************/
// Called by anything that renders to screens[0] (except 3D view)
void I_MarkRect(REG(d0, int left), REG(d1, int top), REG(d2, int width), REG(d3, int height))
{
    M_AddToBox(dirtybox, left, top);
    M_AddToBox(dirtybox, left + width - 1, top + height - 1);
}

/**********************************************************************/
void I_StartUpdate(void)
{
    UBYTE *base_address;

    if (video_is_directcgx) {
        if (video_palette_changed != 0) {
            LoadRGB32(&video_screen->ViewPort, video_colourtable[video_palette_index]);
            video_palette_changed = 0;
        }
        if (video_bitmap_handle == NULL) {
            start_timer();
            video_bitmap_handle = LockBitMapTags(video_screen->ViewPort.RasInfo->BitMap, LBMI_BASEADDRESS,
                                                 (ULONG)&base_address, TAG_DONE);
            lock_time += end_timer();
            video_which = 1 - video_which;
            if (video_which != 0)
                screens[0] = base_address + (SCREENWIDTH * video_oscan_height);
            else
                screens[0] = base_address;
            R_InitBuffer(scaledviewwidth, viewheight); /* recalc ylookup2[] */
        }
    }
}

/**********************************************************************/
void I_UpdateNoBlit(void)
{
}
/**********************************************************************/
void I_FinishUpdate(void)
/* This needs optimising to copy just the parts that changed,
   especially if the user has shrunk the playscreen. */
{
    int top, left, width, height;

    total_frames++;

#ifdef GRAFFITI
    if (video_graffiti != 0) {
        start_timer();
        CopyChunkyScreen(video_grf_screen, screens[0]);
        SetChunkyPalette(video_grf_screen, (long *)video_colourtable[video_palette_index]);
        video_palette_changed = 0;
        ccs_time += end_timer();
        return;
    }
#endif
#ifdef INDIVISION
    if (video_indigfx != 0) {
        start_timer();
        indivision_gfxcopy(0xdff0ac, screens[0], (long *)&video_colourtable[video_palette_index][0], 0x0, gfxlength);
        ccs_time += end_timer();  // Indivision GFX video update
        return;
    }
#endif
    if (video_is_directcgx) {
        if (video_bitmap_handle != NULL) {
            UnLockBitMap(video_bitmap_handle);
            video_bitmap_handle = NULL;
            screens[0] = NULL;

            video_screen->ViewPort.RasInfo->RyOffset = video_which != 0 ? video_oscan_height : 0;
            ScrollVPort(&video_screen->ViewPort);

            if (gamestate != GS_LEVEL) {
                M_AddToBox(dirtybox, 0, 0);
                M_AddToBox(dirtybox, SCREENWIDTH - 1, SCREENHEIGHT - 1);
            }

            /* copy dirtybox into other buffer, hopefully using gfx-card blitter!? */
            if (dirtybox[BOXRIGHT] >= dirtybox[BOXLEFT] && dirtybox[BOXTOP] >= dirtybox[BOXBOTTOM]) {
                if (video_which != 0) {
                    // copy 1 -> 0
                    ClipBlit(video_window->RPort, dirtybox[BOXLEFT], video_oscan_height + dirtybox[BOXBOTTOM],
                             video_window->RPort, dirtybox[BOXLEFT], dirtybox[BOXBOTTOM],
                             dirtybox[BOXRIGHT] - dirtybox[BOXLEFT] + 1, dirtybox[BOXTOP] - dirtybox[BOXBOTTOM] + 1,
                             0xc0);
                } else {
                    // copy 0 -> 1
                    ClipBlit(video_window->RPort, dirtybox[BOXLEFT], dirtybox[BOXBOTTOM], video_window->RPort,
                             dirtybox[BOXLEFT], video_oscan_height + dirtybox[BOXBOTTOM],
                             dirtybox[BOXRIGHT] - dirtybox[BOXLEFT] + 1, dirtybox[BOXTOP] - dirtybox[BOXBOTTOM] + 1,
                             0xc0);
                }
            }
            M_ClearBox(dirtybox);
        } else
            I_Error("I_FinishUpdate() called without calling I_StartUpdate() first");

        if (video_doing_fps)
            video_do_fps(video_window->RPort, video_which != 0 ? video_oscan_height : 0);
        return;
    }

    /* update only the viewwindow and dirtybox when gamestate == GS_LEVEL */
    if (gamestate == GS_LEVEL) {
        if (dirtybox[BOXLEFT] < viewwindowx)
            left = dirtybox[BOXLEFT];
        else
            left = viewwindowx;
        if (dirtybox[BOXRIGHT] + 1 > viewwindowx + scaledviewwidth)
            width = dirtybox[BOXRIGHT] + 1 - left;
        else
            width = viewwindowx + scaledviewwidth - left;
        if (dirtybox[BOXBOTTOM] < viewwindowy) /* BOXBOTTOM is really the top! */
            top = dirtybox[BOXBOTTOM];
        else
            top = viewwindowy;
        if (dirtybox[BOXTOP] + 1 > viewwindowy + viewheight)
            height = dirtybox[BOXTOP] + 1 - top;
        else
            height = viewwindowy + viewheight - top;
        M_ClearBox(dirtybox);
#ifdef RANGECHECK
        if (left < 0 || left + width > SCREENWIDTH || top < 0 || top + height > SCREENHEIGHT)
            I_Error("I_FinishUpdate: Box out of range: %d %d %d %d", left, top, width, height);
#endif
    } else {
        left = 0;
        top = 0;
        width = SCREENWIDTH;
        height = SCREENHEIGHT;
    }

    if (video_is_native_mode) {
        if (video_is_using_blitter) {
            DEBUGSTEP();
            start_timer();
            Wait(1 << video_sigbit1); /* wait for prev c2p() to finish pass 3 */
            blit_time += end_timer();
            if (video_blit_is_in_progress) {
                start_timer();
                Wait(SIGBREAKF_CTRL_F); /* wait for prev ChangeScreenBuffer() safe */
                safe_time += end_timer();
                video_blit_is_in_progress = FALSE;
            }
            video_which = 1 - video_which; /* render to the hidden bitmap */
            start_timer();
            if (video_is_ehb_mode) {
                video_palette_index = video_pending_palette_index;
                c2p_6_020(&screens[0][SCREENWIDTH * top], video_bitmap[video_which].Planes, 1 << video_sigbit1,
                          1 << video_sigbit2, 1 << video_sigbit3, SCREENWIDTH * height, (SCREENWIDTH >> 3) * top,
                          video_xlate[video_palette_index], video_fliptask, video_chipbuff);
            } else {
                if (cpu_type < 68030)
                    c2p_8_020(&screens[0][SCREENWIDTH * top], video_bitmap[video_which].Planes, 1 << video_sigbit1,
                              1 << video_sigbit2, 1 << video_sigbit3, SCREENWIDTH * height, (SCREENWIDTH >> 3) * top,
                              video_fliptask, video_chipbuff);
                else
                    c2p1x1_cpu3blit1_queue(screens[0], video_raster[video_which]);
            }
            c2p_time += end_timer();
            video_blit_is_in_progress = TRUE;
        } else {
            start_timer();
            if (video_is_ehb_mode) {
                c2p_6_040(screens[0], video_raster[video_which], video_compare_buffer[video_which],
                          video_xlate[video_palette_index], (SCREENWIDTH * SCREENHEIGHT) >> 3, video_palette_changed);
                if (video_palette_changed > 0) {
                    if (video_palette_changed == 3)
                        LoadRGB32(&video_screen->ViewPort, video_colourtable[video_palette_index]);
                    video_palette_changed--;
                }
            } else {
                if (c2p && scaledviewwidth >= SCREENWIDTH - 64)
                    c2p(screens[0], video_raster[video_which], screens[0] + SCREENWIDTH * SCREENHEIGHT);
                else
                    c2p_8_040(screens[0], video_raster[video_which], video_compare_buffer[video_which],
                              (SCREENWIDTH * SCREENHEIGHT) >> 3);
                if (video_palette_changed != 0) {
                    LoadRGB32(&video_screen->ViewPort, video_colourtable[video_palette_index]);
                    video_palette_changed = 0;
                }
            }
            c2p_time += end_timer();
            if (video_doing_fps)
                video_do_fps(&video_rastport[video_which], 0);
            WaitPort(video_mp);
            while (GetMsg(video_mp))
                /* do nothing */;
            ChangeVPBitMap(&video_screen->ViewPort, &video_bitmap[video_which], video_db);
            if (++video_which == 3)
                video_which = 0;
        }
    } else { /* non-native ScreenMode */
        if (video_palette_changed != 0) {
            LoadRGB32(&video_screen->ViewPort, video_colourtable[video_palette_index]);
            video_palette_changed = 0;
        }
        start_timer();
        if (GfxBase->LibNode.lib_Version >= 40)
            WriteChunkyPixels(video_window->RPort, left, top, left + width - 1, top + height - 1,
                              &screens[0][SCREENWIDTH * top + left], SCREENWIDTH);
        else
            WritePixelArray8(video_window->RPort, 0, top, SCREENWIDTH - 1, top + height - 1,
                             &screens[0][SCREENWIDTH * top], &video_temprp);
        wpa8_time += end_timer();
        if (video_doing_fps)
            video_do_fps(video_window->RPort, 0);
    }
}

/**********************************************************************/
// Wait for vertical retrace or pause a bit.  Use when quit game.
void I_WaitVBL(int count)
{
    for (; count > 0; count--)
        WaitTOF();
}

/**********************************************************************/
void I_ReadScreen(byte *scr)
{
    UBYTE *base_address;

    if (video_is_directcgx)
        if (video_bitmap_handle != NULL) {
#if 0
      UnLockBitMap (video_bitmap_handle);
      video_bitmap_handle = NULL;
      screens[0] = NULL;
      ReadPixelArray8 (video_window->RPort, 0,
                       video_which != 0 ? video_oscan_height : 0,
                       SCREENWIDTH-1, SCREENHEIGHT-1, scr, &video_temprp);
      video_bitmap_handle = LockBitMapTags (video_screen->ViewPort.RasInfo->BitMap,
                                            LBMI_BASEADDRESS, &base_address,
                                            TAG_DONE);
      if (video_which != 0)
        screens[0] = base_address + (SCREENWIDTH * video_oscan_height);
      else
        screens[0] = base_address;
      R_InitBuffer (scaledviewwidth, viewheight); /* recalc ylookup2[] */
#endif
            // memcpy (scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
            CopyMemQuick(screens[0], scr, SCREENWIDTH * SCREENHEIGHT);
        } else {
            video_bitmap_handle = LockBitMapTags(video_screen->ViewPort.RasInfo->BitMap, LBMI_BASEADDRESS,
                                                 (ULONG)&base_address, TAG_DONE);
            // memcpy (scr, base_address + (video_which != 0 ?
            //        SCREENWIDTH * video_oscan_height: 0), SCREENWIDTH * SCREENHEIGHT);
            CopyMemQuick(base_address + (video_which != 0 ? SCREENWIDTH * video_oscan_height : 0), scr,
                         SCREENWIDTH * SCREENHEIGHT);
            UnLockBitMap(video_bitmap_handle);
            video_bitmap_handle = NULL;
#if 0
      ReadPixelArray8 (video_window->RPort, 0,
                       video_which != 0 ? video_oscan_height : 0,
                       SCREENWIDTH-1, SCREENHEIGHT-1, scr, &video_temprp);
#endif
        }
    else
        memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

/**********************************************************************/
void I_BeginRead(void)
{
}
/**********************************************************************/
void I_EndRead(void)
{
}
/**********************************************************************/
int xlate_key(UWORD rawkey, UWORD qualifier, APTR eventptr)
{
    char buffer[4], c;
    struct InputEvent ie;

    if (video_is_rawkey)
        if (rawkey < 0x68)
            return xlate[rawkey];
        else
            return 0;
    else if (rawkey > 0x00 && rawkey < 0x0a)  // '1'..'9', no SHIFT French keyboards
        return '0' + rawkey;
    else if (rawkey == 0x0a)  // '0'
        return '0';
    else if (rawkey < 0x40) {
        ie.ie_Class = IECLASS_RAWKEY;
        ie.ie_Code = rawkey;
        ie.ie_Qualifier = qualifier;
        ie.ie_EventAddress = eventptr;
        if (MapRawKey(&ie, buffer, sizeof(buffer), NULL) > 0) {
            c = buffer[0];
            if (c >= '0' && c <= '9') /* numeric pad */
                switch (c) {
                case '0':
                    return ' ';
                case '1':
                    return ',';
                case '2':
                    return KEY_RCTRL;
                case '3':
                    return '.';
                case '4':
                    return KEY_LEFTARROW;
                case '5':
                    return KEY_DOWNARROW;
                case '6':
                    return KEY_RIGHTARROW;
                case '7':
                    return ',';
                case '8':
                    return KEY_UPARROW;
                case '9':
                    return '.';
                }
            else if (c >= 'A' && c <= 'Z')
                return c - 'A' + 'a';
            else if (c == '<')
                return ',';
            else if (c == '>')
                return '.';
            else if (c == '-')
                return KEY_MINUS;
            else if (c == '=')
                return KEY_EQUALS;
            else if (c == '[')
                return KEY_F11;
            else if (c == ']')
                return KEY_F12;
            else if (c == '\r')
                return KEY_ENTER;
            else if (c == '\n')
                return KEY_ENTER;
            else
                return c;
        } else
            return 0;
    } else if (rawkey < 0x68)
        return xlate[rawkey];

    return 0;
}

/**********************************************************************/
//
static struct InputEvent *SAVEDS INTERRUPT video_inputhandler(REG(a0, struct InputEvent *ie),
                                                                      REG(a1, APTR data))
{
    event_t event;
    static event_t mouseevent = {0};

    while (ie != NULL) {
        if (ie->ie_Class == IECLASS_RAWKEY) {
            if ((ie->ie_Code & IECODE_UP_PREFIX) != 0) {
                ie->ie_Code &= ~IECODE_UP_PREFIX;
                event.type = ev_keyup;
            } else {
                event.type = ev_keydown;
            }
            if (ie->ie_Code < 0x68) {
                event.data1 = xlate_key(ie->ie_Code, ie->ie_Qualifier, ie->ie_EventAddress);
                D_PostEvent(&event);
            }
        } else if (video_is_using_mouse && ie->ie_Class == IECLASS_RAWMOUSE) {
            mouseevent.type = ev_mouse;
            switch (ie->ie_Code) {
            case IECODE_LBUTTON:
                mouseevent.data1 |= 1;
                break;
            case (IECODE_LBUTTON | IECODE_UP_PREFIX):
                mouseevent.data1 &= ~1;
                break;
            case IECODE_RBUTTON:
                mouseevent.data1 |= 2;
                break;
            case (IECODE_RBUTTON | IECODE_UP_PREFIX):
                mouseevent.data1 &= ~2;
                break;
            case IECODE_MBUTTON:
                mouseevent.data1 |= 4;
                break;
            case (IECODE_MBUTTON | IECODE_UP_PREFIX):
                mouseevent.data1 &= ~4;
                break;
            default:
                break;
            }
            mouseevent.data2 = (ie->ie_X << 3);
            mouseevent.data3 = -(ie->ie_Y << 5);
            D_PostEvent(&mouseevent);
        }
        ie = ie->ie_NextEvent;
    }
    return NULL;
}

/**********************************************************************/
void amiga_getevents(void)
{
    event_t event;
    ULONG class;
    UWORD code;
    WORD mousex, mousey;
    struct IntuiMessage *msg;
    static ULONG previous = 0;
    static event_t joyevent = {0}, mouseevent = {0};
    ULONG currSega;
    int doomkey;

    DEBUGSTEP();

    if (video_window != NULL && video_window->UserPort != NULL) {
        while ((msg = (struct IntuiMessage *)GetMsg(video_window->UserPort)) != NULL) {
            class = msg->Class;
            code = msg->Code;
            mousex = msg->MouseX;
            mousey = msg->MouseY;
            if (class == IDCMP_RAWKEY) {
                if ((code & 0x80) != 0) {
                    code &= ~0x80;
                    event.type = ev_keyup;
                } else {
                    event.type = ev_keydown;
                }
                if (code < 0x68)
                    doomkey = xlate_key(code, msg->Qualifier, msg->IAddress);
            }
            ReplyMsg((struct Message *)msg); /* reply after xlating key */
            if (class == IDCMP_RAWKEY) {
                if (code < 0x68 && doomkey != 0) {
                    event.data1 = doomkey;
                    D_PostEvent(&event);
                    /* printf ("key %02x -> %02x\n", code, doomkey); */
                }
            } else if (class == IDCMP_MOUSEMOVE) {
                mouseevent.type = ev_mouse;
                mouseevent.data2 = (mousex << 3);
                mouseevent.data3 = -(mousey << 5);
                D_PostEvent(&mouseevent);
            } else if (class == IDCMP_MOUSEBUTTONS) {
                mouseevent.type = ev_mouse;
                switch (code) {
                case SELECTDOWN:
                    mouseevent.data1 |= 1;
                    break;
                case SELECTUP:
                    mouseevent.data1 &= ~1;
                    break;
                case MENUDOWN:
                    mouseevent.data1 |= 2;
                    break;
                case MENUUP:
                    mouseevent.data1 &= ~2;
                    break;
                case MIDDLEDOWN:
                    mouseevent.data1 |= 4;
                    break;
                case MIDDLEUP:
                    mouseevent.data1 &= ~4;
                    break;
                default:
                    break;
                }
                D_PostEvent(&mouseevent);
            }
        }
    }

    if (gameport_is_open && gameport_io_in_progress) {
        while (GetMsg(gameport_mp) != NULL) {
            switch (gameport_ie.ie_Code) {
            case IECODE_LBUTTON:
                joyevent.data1 |= 1;
                break;
            case IECODE_LBUTTON | IECODE_UP_PREFIX:
                joyevent.data1 &= ~1;
                break;
            case IECODE_RBUTTON:
                joyevent.data1 |= 2;
                break;
            case IECODE_RBUTTON | IECODE_UP_PREFIX:
                joyevent.data1 &= ~2;
                break;
            case IECODE_MBUTTON:
                joyevent.data1 |= 4;
                break;
            case IECODE_MBUTTON | IECODE_UP_PREFIX:
                joyevent.data1 &= ~4;
                break;
            case IECODE_NOBUTTON:
                joyevent.data2 = gameport_ie.ie_X;
                joyevent.data3 = gameport_ie.ie_Y;
                break;
            default:
                break;
            }
            joyevent.type = ev_joystick;
            D_PostEvent(&joyevent);
            gameport_io->io_Command = GPD_READEVENT;
            gameport_io->io_Length = sizeof(struct InputEvent);
            gameport_io->io_Data = &gameport_ie;
            SendIO((struct IORequest *)gameport_io);
        }
    }

    /* CD32 joypad handler code supplied by Gabry (ggreco@iol.it) */

    if (LowLevelBase != NULL) {
        event_t joyevent;
        ULONG joypos = ReadJoyPort(1);

        if (previous == joypos)
            return;

        joyevent.type = ev_joystick;
        joyevent.data1 = joyevent.data2 = joyevent.data3 = 0;

        if (joypos & JPF_BUTTON_RED)
            joyevent.data1 |= 1;
        else
            joyevent.data1 &= ~1;

        if (joypos & JP_DIRECTION_MASK) {
            if (joypos & JPF_JOY_LEFT) {
                joyevent.data2 = -1;
            } else if (joypos & JPF_JOY_RIGHT) {
                joyevent.data2 = 1;
            }
            if (joypos & JPF_JOY_UP) {
                joyevent.data3 = -1;
            } else if (joypos & JPF_JOY_DOWN) {
                joyevent.data3 = 1;
            }
        }

        if (joypos & JP_TYPE_GAMECTLR) {
            event_t event;

            // Play/Pause = ESC (Menu)
            if (joypos & JPF_BUTTON_PLAY && !(previous & JPF_BUTTON_PLAY)) {
                event.type = ev_keydown;
                event.data1 = KEY_ESCAPE;
                D_PostEvent(&event);
            } else if (previous & JPF_BUTTON_PLAY) {
                event.type = ev_keyup;
                event.data1 = KEY_ESCAPE;
                D_PostEvent(&event);
            }

            // YELLOW = SHIFT (button 2) (Run)
            if (joypos & JPF_BUTTON_YELLOW)
                joyevent.data1 |= 4;
            else
                joyevent.data1 &= ~4;

            // BLUE = SPACE (button 3) (Open/Operate)

            if (joypos & JPF_BUTTON_BLUE)
                joyevent.data1 |= 8;
            else
                joyevent.data1 &= ~8;

            // GREEN = RETURN (show msg)

            if (joypos & JPF_BUTTON_GREEN && !(previous & JPF_BUTTON_GREEN)) {
                event.type = ev_keydown;
                event.data1 = 13;
                D_PostEvent(&event);
            } else if (previous & JPF_BUTTON_GREEN) {
                event.type = ev_keyup;
                event.data1 = 13;
                D_PostEvent(&event);
            }

            // FORWARD & REVERSE - ALT (Button1) Strafe left/right

            if (joypos & JPF_BUTTON_FORWARD) {
                joyevent.data1 |= 2;
                joyevent.data2 = 1;
            } else if (joypos & JPF_BUTTON_REVERSE) {
                joyevent.data1 |= 2;
                joyevent.data2 = -1;
            } else
                joyevent.data1 &= ~2;
        }

        D_PostEvent(&joyevent);

        previous = joypos;
    }

    /* SEGA joypad handler code by Joe Fenton, loosely based on CD32 handling */

    if (sega3_selected || sega6_selected) {
        event_t joyevent, event;

        if (sega3_selected) {
            currSega = Sega3();
        } else {
            currSega = Sega6();
        }

        if (prevSega == currSega)
            return;

        joyevent.type = ev_joystick;
        joyevent.data1 = joyevent.data2 = joyevent.data3 = 0;

        // B = fire
        if (currSega & 0x10)
            joyevent.data1 |= 1;
        else
            joyevent.data1 &= ~1;

        // directionals
        if (currSega & 0xF) {
            if (currSega & 4) {
                joyevent.data2 = -1;
            } else if (currSega & 8) {
                joyevent.data2 = 1;
            }
            if (currSega & 1) {
                joyevent.data3 = -1;
            } else if (currSega & 2) {
                joyevent.data3 = 1;
            }
        }

        // Mode = ESC (Menu)
        if (currSega & 0x80000 && !(prevSega & 0x80000)) {
            event.type = ev_keydown;
            event.data1 = KEY_ESCAPE;
            D_PostEvent(&event);
        } else if (prevSega & 0x80000) {
            event.type = ev_keyup;
            event.data1 = KEY_ESCAPE;
            D_PostEvent(&event);
        }

        // Y = SHIFT (Run)
        if (currSega & 0x20000)
            joyevent.data1 |= 4;
        else
            joyevent.data1 &= ~4;

        // Start = SPACE (Open/Operate)
        if (currSega & 0x2000)
            joyevent.data1 |= 8;
        else
            joyevent.data1 &= ~8;

        // A & C - ALT (Button1) Strafe left/right
        if (currSega & 0x20) {
            joyevent.data1 |= 2;
            joyevent.data2 = 1;
        } else if (currSega & 0x1000) {
            joyevent.data1 |= 2;
            joyevent.data2 = -1;
        } else
            joyevent.data1 &= ~2;

        // X = RETURN (show msg)
        if (currSega & 0x40000 && !(prevSega & 0x40000)) {
            event.type = ev_keydown;
            event.data1 = 13;
            D_PostEvent(&event);
        } else if (prevSega & 0x40000) {
            event.type = ev_keyup;
            event.data1 = 13;
            D_PostEvent(&event);
        }

        // Z = TAB (show map)
        if (currSega & 0x10000 && !(prevSega & 0x10000)) {
            event.type = ev_keydown;
            event.data1 = 9;
            D_PostEvent(&event);
        } else if (prevSega & 0x10000) {
            event.type = ev_keyup;
            event.data1 = 9;
            D_PostEvent(&event);
        }

        D_PostEvent(&joyevent);

        prevSega = currSega;
    }
}

/**********************************************************************/
static void calc_time(ULONG time, char *msg)
{
    printf("Total %s = %u us  (%u us/frame)\n", msg, (ULONG)(1000000.0 * ((double)time) / ((double)eclocks_per_second)),
           (ULONG)(1000000.0 * ((double)time) / ((double)eclocks_per_second) / ((double)total_frames)));
}

/**********************************************************************/
void _STDvideo_cleanup(void)
{
    I_ShutdownGraphics();
    if (video_smr != NULL) {
        FreeAslRequest(video_smr);
        video_smr = NULL;
    }

    /* printf ("EClocks per second = %d\n", eclocks_per_second); */
    if (total_frames > 0) {
        printf("Total number of frames = %u\n", total_frames);
        calc_time(blit_time, "blit wait time        ");
        calc_time(safe_time, "safe wait time        ");
        calc_time(c2p_time, "Chunky2Planar time    ");
        calc_time(ccs_time, "CopyChunkyScreen time ");
        calc_time(wpa8_time, "WritePixelArray8 time ");
        calc_time(lock_time, "LockBitMap time       ");
    }
}

/**********************************************************************/
