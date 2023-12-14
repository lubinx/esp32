#ifndef __HW_RTC_HAL_H
#define __HW_RTC_HAL_H                  1

#include <features.h>
#include <time.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    void RTC_timeval(struct timeval *tv);

extern __attribute__((nothrow))
    void RTC_set_epoch_time(time_t ts);

__END_DECLS
#endif
