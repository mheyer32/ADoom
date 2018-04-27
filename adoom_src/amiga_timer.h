#ifndef AMIGA_TIMER_H
#define AMIGA_TIMER_H

#include <proto/timer.h>

extern BOOL InitTimer(void);
extern void ShutdownTimer(void);

static inline void StartTimer(struct EClockVal *start_timer)
{
    ReadEClock(start_timer);
}

static inline ULONG EndTimer(const struct EClockVal *start_timer)
{
    struct EClockVal end_time;
    ReadEClock(&end_time);
    return end_time.ev_lo - start_timer->ev_lo;
}

extern void PrintTime(ULONG time, const char *msg, ULONG total_frames);

// short cuts: assumes a 'struct EClockVal start_time;' variable in current scope
#define start_timer() StartTimer(&start_time)
#define end_timer() EndTimer(&start_time)

#endif // AMIGA_TIMER_H
