/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <features.h>
#include <stdint.h>

#include "esp_arch.h"
#include "esp_attr.h"           // for freertos detail see portmacro.h...has no wahre to put this
#include "soc/soc_caps.h"

// avoid "esp_err.h"
typedef int esp_err_t;
// deprecated
typedef uint32_t esp_cpu_cycle_count_t;

#define esp_cpu_get_core_id()           __get_CORE_ID()

__BEGIN_DECLS
    enum esp_cpu_watchpoint_trigger_t
    {
        ESP_CPU_WATCHPOINT_LOAD,
        ESP_CPU_WATCHPOINT_STORE,
        ESP_CPU_WATCHPOINT_ACCESS,
    };
    typedef enum esp_cpu_watchpoint_trigger_t esp_cpu_watchpoint_trigger_t;

extern __attribute__((nonnull, nothrow))
    esp_err_t esp_cpu_set_watchpoint(int wp_nb, void const *wp_addr, size_t size, esp_cpu_watchpoint_trigger_t trigger);
extern __attribute__((nothrow))
    esp_err_t esp_cpu_clear_watchpoint(int wp_nb);

__END_DECLS
