#include "amiga_timer.h"

#include <proto/exec.h>

#include <stdio.h>

struct Device *TimerBase = NULL;
struct IORequest timereq;
static ULONG eclocks_per_ms; /* EClock frequency in 1000Hz */

BOOL InitTimer(void)
{
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, &timereq, 0)) {
        return FALSE;
    }

    TimerBase = timereq.io_Device;

    struct EClockVal time;
    eclocks_per_ms = ReadEClock(&time) / 1000;

    return TRUE;
}

void ShutdownTimer()
{
    if (TimerBase) {
        CloseDevice(&timereq);
        TimerBase = NULL;
    }
}

void PrintTime(ULONG time, const char* msg, ULONG total_frames)
{
    ULONG usec = time * 1000 / eclocks_per_ms;
    printf("Total %s = %u us  (%u us/frame)\n", msg, usec, usec / total_frames);
}
