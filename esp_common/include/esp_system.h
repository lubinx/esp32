#pragma once

#include <features.h>
#include <stdint.h>

__BEGIN_DECLS

// avoid esp_err.h
typedef int esp_err_t;

/****************************************************************************
 *  rtos
*****************************************************************************/
    /**
     *  called by esp_startup.c(Startup_Handler())
    */
extern __attribute__((noreturn))
    void __rtos_bootstrap(void);

    /**
     *  callback function: was called before enter main()
     *      .it for replace gcc _start()
     *
     *  NOTE: rtos is initialized when callback to this function.
     *      so the "constructor" functions was inside rtos "main" thread context
     *
     *  NOTE: freertos initialize chain:
     *      1.Startup_Handler() => ZI => SOC_initialize() => __retarget_init() => __rtos_bootstrap()
     *      2.__rtos_bootstrap() => create "main" thread (__freertos_start()) => startting task scheduler
     *      3.__freertos_start() => calling __rtos_start() => calling main()
    */
extern __attribute__((nothrow))
    void __rtos_start(void);

/****************************************************************************
 *  for esp-idf's freertos porting
*****************************************************************************/
extern __attribute__((nothrow))
    esp_err_t esp_register_shutdown_handler(void (*func_ptr)(void));
extern __attribute__((nothrow))
    esp_err_t esp_unregister_shutdown_handler(void (*func_ptr)(void));

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

#define ESP_SYSTEM_INIT_ALL_CORES ((1U << SOC_CPU_CORES_NUM) - 1)

__END_DECLS
