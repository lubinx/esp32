#include <time.h>
#include <sys/time.h>

#include "hw/hal/rtc_hal.h"

time_t time(time_t *timep)
{
    struct timeval tv;
    RTC_HAL_get_time(&tv);

    if (timep)
        *timep = tv.tv_sec;

    return tv.tv_sec;
}

int stime(time_t const *timeptr)
{
    RTC_HAL_set_time(*timeptr);
    return 0;
}

int gettimeofday(struct timeval *tv, void *_tz)
{
    RTC_HAL_get_time(tv);

    ARG_UNUSED(_tz);
    return 0;
}

int _gettimeofday_r(struct _reent *r, struct timeval *tv, void *_tz)
{
    RTC_HAL_get_time(tv);

    ARG_UNUSED(r, _tz);
    return 0;
}

__attribute__((weak))
void RTC_HAL_get_time(struct timeval *tv)
{
    tv->tv_sec = 0;
    tv->tv_usec = 0;
}
