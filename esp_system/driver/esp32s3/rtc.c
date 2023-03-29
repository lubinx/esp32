#include "hw/hal/rtc_hal.h"

void RTC_HAL_get_time(struct timeval *tv)
{
    tv->tv_sec = 0;
    tv->tv_usec = 0;
}

void RTC_HAL_set_time(time_t ts)
{
}
