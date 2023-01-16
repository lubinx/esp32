/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_attr.h"
#include "esp_bit_defs.h"
#include "soc/soc_caps.h"

// TODO: remove this
extern uint64_t g_startup_time;   // Startup time that serves as the point of origin for system time. Should be set by the entry
                                  // function in the port layer. May be 0 as well if this is not backed by a persistent counter, in which case
                                  // startup time = system time = 0 at the point the entry function sets this variable.

/**
 * @brief Define a system initialization function which will be executed on the specified cores
 *
 * @param f  function name (identifier)
 * @param c  bit mask of cores to execute the function on (ex. if BIT0 is set, the function
 *           will be executed on CPU 0, if BIT1 is set - on CPU 1, and so on)
 * @param priority  integer, priority of the initialization function. Higher values mean that
 *                  the function will be executed later in the process.
 * @param (varargs)  optional, additional attributes for the function declaration (such as IRAM_ATTR)
 *
 * The function defined using this macro must return ESP_OK on success. Any other value will be
 * logged and the startup process will abort.
 *
 * Initialization functions should be placed in a compilation unit where at least one other
 * symbol is referenced in another compilation unit. This means that the reference should not itself
 * get optimized out by the compiler or discarded by the linker if the related feature is used.
 * It is, on the other hand, a good practice to make sure the initialization function does get
 * discarded if the related feature is not used.
 */
struct __esp_init_fn
{
    esp_err_t (*fn)(void);
    uint32_t cores;
};


#define ESP_SYSTEM_INIT_FN(f, c, priority, ...) \
    static esp_err_t __VA_ARGS__ __esp_system_init_fn_##f(void);    \
    static __attribute__((used)) _SECTION_ATTR_IMPL(".esp_system_init_fn", priority)    \
        struct __esp_init_fn esp_system_init_fn_##f = { .fn = ( __esp_system_init_fn_##f), .cores = (c) };  \
    static esp_err_t __esp_system_init_fn_##f(void)

#define ESP_SYSTEM_INIT_ALL_CORES (BIT(SOC_CPU_CORES_NUM) - 1)
