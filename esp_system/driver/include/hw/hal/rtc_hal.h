#ifndef __HW_RTC_HAL_H
#define __HW_RTC_HAL_H                  1

#include <features.h>
#include <time.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    void RTC_HAL_get_time(struct timeval *tv);

extern __attribute__((nothrow))
    void RTC_HAL_set_time(time_t ts);

__END_DECLS
#endif
