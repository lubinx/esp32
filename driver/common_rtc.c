#include <time.h>
#include <sys/time.h>

#include "hw/hal/rtc_hal.h"

/***************************************************************************/
/** @implements: posix
****************************************************************************/
int gettimeofday(struct timeval *tv, void *_tz)
{
    RTC_time(tv);

    ARG_UNUSED(_tz);
    return 0;
}

int _gettimeofday_r(struct _reent *r, struct timeval *tv, void *_tz)
{
    RTC_time(tv);

    ARG_UNUSED(r, _tz);
    return 0;
}

/***************************************************************************/
/** @implements
****************************************************************************/
time_t time(time_t *timep)
{
    struct timeval tv;
    RTC_time(&tv);

    if (timep)
        *timep = tv.tv_sec;

    return tv.tv_sec;
}

int stime(time_t const ts)
{
    RTC_set_time(ts);
    return 0;
}

/*
__attribute__((weak))
void RTC_time(struct timeval *tv)
{
    tv->tv_sec = 0;
    tv->tv_usec = 0;
}
*/
