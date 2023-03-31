/**
 *  its odd they put lots of things nothing related to RTC into RTCCTRL register
*/
#ifndef __ESP32S3_RTC_H
#define __ESP32S3_RTC_H                 1

#include <features.h>
#include <sys/_timespec.h>

__BEGIN_DECLS

__attribute__((nothrow, pure))
    uint64_t RTCCNTL_get_tick(void);

/*
typedef enum
{
    ESP_RST_UNKNOWN,
    ESP_RST_POWERON,
    ESP_RST_EXT,
    ESP_RST_SW,
    ESP_RST_PANIC,
    ESP_RST_INT_WDT,
    ESP_RST_TASK_WDT,
    ESP_RST_WDT,
    ESP_RST_DEEPSLEEP,
    ESP_RST_BROWNOUT,
    ESP_RST_SDIO,
} esp_reset_reason_t;
*/
// no implemented yet, don't know how to deal with it
extern __attribute__((nothrow, pure))
    unsigned RTCCNTL_get_reset_reasion(unsigned core_id);

__END_DECLS
#endif
