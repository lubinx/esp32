#include <soc/rtc_cntl_struct.h>
#include <clk-tree.h>
#include "hw/hal/rtc_hal.h"

/***************************************************************************/
/** @internal
****************************************************************************/
static uint64_t RTCC_tick_offset = 0;
static uint32_t RTCC_tick_freq = 0;

/***************************************************************************/
/** constructor
****************************************************************************/
__attribute__((constructor))
void RTC_init()
{
    RTCC_tick_offset = (time_t)RTCCNTL.store6 << 32 | RTCCNTL.store7;
    RTCC_tick_freq = CLK_rtc_freq();
}

/***************************************************************************/
/** @implements
****************************************************************************/
uint64_t RTC_tick(void)
{
    RTCCNTL.time_update.update = 1;
    return (uint64_t)RTCCNTL.time_high0.rtc_timer_value0_high << 32 | RTCCNTL.time_low0;
}

void RTC_HAL_get_time(struct timeval *tv)
{
    uint64_t tick = RTC_tick() + RTCC_tick_offset;

    tv->tv_sec = (time_t)(tick / RTCC_tick_freq);
    tv->tv_usec = (suseconds_t)((tick % RTCC_tick_freq) * _MHZ / RTCC_tick_freq);
}

void RTC_HAL_set_time(time_t ts)
{
    ARG_UNUSED(ts);
}
