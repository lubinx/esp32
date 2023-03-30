#ifndef __ESP32S3_RTC_H
#define __ESP32S3_RTC_H                 1

#include <features.h>
#include <sys/_timespec.h>

__BEGIN_DECLS

__attribute__((nothrow, pure))
    uint64_t RTC_tick(void);

__END_DECLS
#endif
