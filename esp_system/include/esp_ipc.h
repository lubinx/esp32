/*
 * Inter-processor call APIs
 *
 *  FreeRTOS provides several APIs which can be used to communicate between different tasks, including tasks running on
 *  different CPUs. This module provides additional APIs to run some code on the other CPU. These APIs can only be used
 *  when FreeRTOS scheduler is running.
 */
/**
 * NOTE: ...well this very very stpuid idea
*/
#pragma once

#include <features.h>
#include "esp_err.h"

#define esp_ipc_isr_stall_other_cpu()
#define esp_ipc_isr_release_other_cpu()
#define esp_ipc_isr_stall_pause()
#define esp_ipc_isr_stall_abort()
#define esp_ipc_isr_stall_resume()

typedef void (*esp_ipc_func_t)(void *arg);

__BEGIN_DECLS

static inline
    esp_err_t esp_ipc_call(uint32_t cpu_id, esp_ipc_func_t func, void *arg)
    {
        func(arg);

        ARG_UNUSED(cpu_id);
        return ESP_OK;
    }

static inline
    esp_err_t esp_ipc_call_blocking(uint32_t cpu_id, esp_ipc_func_t func, void *arg)
    {
        func(arg);

        ARG_UNUSED(cpu_id);
        return ESP_OK;
    }

__END_DECLS
