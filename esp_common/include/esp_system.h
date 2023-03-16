#pragma once

#include <features.h>
#include <stdint.h>

__BEGIN_DECLS

// avoid esp_err.h
typedef int esp_err_t;

/****************************************************************************
 *  rtos
*****************************************************************************/
extern __attribute__((noreturn))
    void esp_rtos_bootstrap(void);

    /**
     *  this function will called before enter main()
     *  NOTE: rtos context here must be initialized
    */
extern __attribute__((nothrow))
    void __esp_rtos_initialize(void);

/****************************************************************************
 *  for esp-idf's freertos porting
*****************************************************************************/
extern __attribute__((nothrow))
    esp_err_t esp_register_shutdown_handler(void (*func_ptr)(void));
extern __attribute__((nothrow))
    esp_err_t esp_unregister_shutdown_handler(void (*func_ptr)(void));

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

extern __attribute__((nothrow, const))
    esp_reset_reason_t esp_reset_reason(void);
*/

struct __esp_init_fn
{
    esp_err_t (*fn)(void);
    uint32_t cores;
};

#define ESP_SYSTEM_INIT_FN(f, c, priority, ...) \
    static esp_err_t __VA_ARGS__ __esp_system_init_fn_##f(void);    \
    static __attribute__((used, section(".esp_system_init_fn." #priority)))    \
        struct __esp_init_fn esp_system_init_fn_##f = { .fn = ( __esp_system_init_fn_##f), .cores = (c) };  \
    static esp_err_t __esp_system_init_fn_##f(void)

#define ESP_SYSTEM_INIT_ALL_CORES ((1 << SOC_CPU_CORES_NUM) - 1)

__END_DECLS
