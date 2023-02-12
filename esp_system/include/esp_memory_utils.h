/*
 * SPDX-FileCopyrightText: 2010-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "soc/soc.h"
#include "soc/soc_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline bool esp_ptr_in_iram(void *p)
{
    return ((intptr_t)p >= SOC_IRAM_LOW && (intptr_t)p < SOC_IRAM_HIGH);
}

static inline bool esp_ptr_in_dram(void *p)
{
    return ((intptr_t)p >= SOC_DRAM_LOW && (intptr_t)p < SOC_DRAM_HIGH);
}

static inline bool esp_ptr_in_diram_dram(void *p)
{
    return ((intptr_t)p >= SOC_DIRAM_DRAM_LOW && (intptr_t)p < SOC_DIRAM_DRAM_HIGH);
}

static inline bool esp_ptr_in_diram_iram(void *p)
{
    // TODO: IDF-5980 esp32c6 D/I RAM share the same address
    #if SOC_DIRAM_IRAM_LOW == SOC_DIRAM_DRAM_LOW
        return false;
    #else
        return ((intptr_t)p >= SOC_DIRAM_IRAM_LOW && (intptr_t)p < SOC_DIRAM_IRAM_HIGH);
    #endif
}

static inline bool esp_ptr_in_rtc_iram_fast(void *p)
{
    #if SOC_RTC_FAST_MEM_SUPPORTED
        return ((intptr_t)p >= SOC_RTC_IRAM_LOW && (intptr_t)p < SOC_RTC_IRAM_HIGH);
    #else
        return false;
    #endif
}

static inline bool esp_ptr_in_rtc_dram_fast(void *p)
{
    #if SOC_RTC_FAST_MEM_SUPPORTED
        return ((intptr_t)p >= SOC_RTC_DRAM_LOW && (intptr_t)p < SOC_RTC_DRAM_HIGH);
    #else
        return false;
    #endif
}

static inline bool esp_ptr_in_rtc_slow(void *p)
{
    #if SOC_RTC_SLOW_MEM_SUPPORTED
        return ((intptr_t)p >= SOC_RTC_DATA_LOW && (intptr_t)p < SOC_RTC_DATA_HIGH);
    #else
        return false;
    #endif
}


static inline void *esp_ptr_diram_dram_to_iram(void *p)
{
    #if SOC_DIRAM_INVERTED
        return (void *)(SOC_DIRAM_IRAM_LOW + (SOC_DIRAM_DRAM_HIGH - (intptr_t)p) - 4);
    #else
        return (void *)(SOC_DIRAM_IRAM_LOW + ((intptr_t)p - SOC_DIRAM_DRAM_LOW));
    #endif
}

static inline void *esp_ptr_diram_iram_to_dram(void *p)
{
    #if SOC_DIRAM_INVERTED
        return (void *)(SOC_DIRAM_DRAM_LOW + (SOC_DIRAM_IRAM_HIGH - (intptr_t)p) - 4);
    #else
        return (void *)(SOC_DIRAM_DRAM_LOW + ((intptr_t)p - SOC_DIRAM_IRAM_LOW));
    #endif
}

static inline bool esp_ptr_dma_capable(void *p)
{
    return (intptr_t)p >= SOC_DMA_LOW && (intptr_t)p < SOC_DMA_HIGH;
}

static inline bool esp_ptr_executable(void *p)
{
    return ((intptr_t)p >= SOC_IROM_LOW && (intptr_t)p < SOC_IROM_HIGH) ||
        ((intptr_t)p >= SOC_IRAM_LOW && (intptr_t)p < SOC_IRAM_HIGH) ||
        ((intptr_t)p >= SOC_IROM_MASK_LOW && (intptr_t)p < SOC_IROM_MASK_HIGH) ||
    #if SOC_RTC_FAST_MEM_SUPPORTED
        ((intptr_t)p >= SOC_RTC_IRAM_LOW && (intptr_t)p < SOC_RTC_IRAM_HIGH) ||
    #endif
        0;
}

/*
static inline bool esp_ptr_internal(void *p)
{
    bool r;
    r = ((intptr_t)p >= SOC_MEM_INTERNAL_LOW && (intptr_t)p < SOC_MEM_INTERNAL_HIGH);

    #if SOC_RTC_SLOW_MEM_SUPPORTED
        r |= ((intptr_t)p >= SOC_RTC_DATA_LOW && (intptr_t)p < SOC_RTC_DATA_HIGH);
    #endif

    #if CONFIG_ESP_SYSTEM_ALLOW_RTC_FAST_MEM_AS_HEAP
        // For ESP32 case, RTC fast memory is accessible to PRO cpu only and hence
        //  for single core configuration (where it gets added to system heap) following
        //  additional check is required
        r |= ((intptr_t)p >= SOC_RTC_DRAM_LOW && (intptr_t)p < SOC_RTC_DRAM_HIGH);
    #endif
    return r;
}
*/

static inline bool esp_ptr_in_drom(void *p)
{
    return ((intptr_t)p >= SOC_DROM_LOW && (intptr_t)p < SOC_DROM_HIGH);
}

static inline bool esp_stack_ptr_in_dram(uint32_t sp)
{
    //Check if stack ptr is in between SOC_DRAM_LOW and SOC_DRAM_HIGH, and 16 byte aligned.
    return !(sp < SOC_DRAM_LOW || sp > SOC_DRAM_HIGH || ((sp & 0xF) != 0));
}

static inline bool esp_stack_ptr_is_sane(uint32_t sp)
{
    return esp_stack_ptr_in_dram(sp) ||
        // esp_stack_ptr_in_extram(sp) ||
        esp_ptr_in_rtc_dram_fast((void*)sp) ||
        0;
}

#ifdef __cplusplus
}
#endif
