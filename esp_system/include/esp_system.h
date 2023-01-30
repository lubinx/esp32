#pragma once

#include <features.h>
#include <stdint.h>

#include "esp_attr.h"
#include "esp_err.h"

__BEGIN_DECLS

extern __attribute__((nothrow, noreturn))
    void esp_restart(void);
extern __attribute__((nothrow, noreturn))
    void esp_system_abort(char const *details);

typedef void (*shutdown_handler_t)(void);

extern __attribute__((nothrow))
    esp_err_t esp_register_shutdown_handler(shutdown_handler_t handle);
extern __attribute__((nothrow))
    esp_err_t esp_unregister_shutdown_handler(shutdown_handler_t handle);

typedef enum
{
    ESP_RST_UNKNOWN,    //!< Reset reason can not be determined
    ESP_RST_POWERON,    //!< Reset due to power-on event
    ESP_RST_EXT,        //!< Reset by external pin (not applicable for ESP32)
    ESP_RST_SW,         //!< Software reset via esp_restart
    ESP_RST_PANIC,      //!< Software reset due to exception/panic
    ESP_RST_INT_WDT,    //!< Reset (software or hardware) due to interrupt watchdog
    ESP_RST_TASK_WDT,   //!< Reset due to task watchdog
    ESP_RST_WDT,        //!< Reset due to other watchdogs
    ESP_RST_DEEPSLEEP,  //!< Reset after exiting deep sleep mode
    ESP_RST_BROWNOUT,   //!< Brownout reset (software or hardware)
    ESP_RST_SDIO,       //!< Reset over SDIO
} esp_reset_reason_t;

extern __attribute__((nothrow, const))
    esp_reset_reason_t esp_reset_reason(void);

extern __attribute__((nothrow, const))
    uint32_t esp_get_free_heap_size(void);
extern __attribute__((nothrow, const))
    uint32_t esp_get_free_internal_heap_size(void);
extern __attribute__((nothrow, const))
    uint32_t esp_get_minimum_free_heap_size(void);

__END_DECLS
