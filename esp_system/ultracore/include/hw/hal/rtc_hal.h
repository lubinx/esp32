#ifndef __RTC_HAL_H
#define __RTC_HAL_H                     1

#include <features.h>
#include <time.h>

__BEGIN_DECLS

/***************************************************************************
 *  ADC HAL configure
 ***************************************************************************/
extern __attribute__((nothrow))
    time_t RTC_HAL_get_time(void);

extern __attribute__((nothrow))
    int RTC_HAL_set_time(time_t ts);

    /// @weak
extern __attribute__((nothrow))
    void RTC_HAL_time_update(time_t before, time_t after);

__END_DECLS
#endif
