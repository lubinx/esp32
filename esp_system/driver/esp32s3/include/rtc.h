#ifndef __ESP32S3_RTC_H
#define __ESP32S3_RTC_H                 1

#include <features.h>
#include <stdint.h>

// #include "soc/rtc_cntl_struct.h"

__BEGIN_DECLS

__attribute__((nothrow))
    uint64_t RTC_tickcount(void);

__END_DECLS

#endif
