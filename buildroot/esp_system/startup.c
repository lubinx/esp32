/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal/cache_hal.h"
#include "hal/mmu_hal.h"
#include "hal/wdt_hal.h"

#include "esp_heap_caps_init.h"
#include "esp_system.h"
#include "esp_newlib.h"
#include "esp_private/system_internal.h"

// #include "esp_rom_sys.h"

// #include "esp_log.h"
#include "esp_rom_sys.h"

/****************************************************************************
 *  import
*****************************************************************************/
extern int _vector_table;

extern int _data_start;
extern int _data_end;
extern int _bss_start;
extern int _bss_end;

extern int main(void);

/****************************************************************************
 *  local declaration
*****************************************************************************/
static void startup_disable_wdt(void);

/****************************************************************************
 * export
*****************************************************************************/
void __attribute__((noreturn)) call_start_cpu0(void)
{
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    /**
     *  setup instruction/data cache
     *  configure:
     *      CONFIG_FREERTOS_UNICORE
     *      SOC_SHARED_IDCACHE_SUPPORTED
    */
    cache_hal_init();

    /**
     *  setup mmu to allow run code in external-flash
     *  configure:
     *      CONFIG_FREERTOS_UNICORE
    */
    mmu_hal_init();

    /* Initialize heap allocator. WARNING: This *needs* to happen *after* the app cpu has booted.
       If the heap allocator is initialized first, it will put free memory linked list items into
       memory also used by the ROM. Starting the app cpu will let its ROM initialize that memory,
       corrupting those linked lists. Initializing the allocator *after* the app cpu has booted
       works around this problem.
       With SPI RAM enabled, there's a second reason: half of the SPI RAM will be managed by the
       app CPU, and when that is not up yet, the memory will be inaccessible and heap_caps_init may
       fail initializing it properly. */
    // heap_caps_init();

    // disable wdt for now
    startup_disable_wdt();

    esp_newlib_init();
    esp_newlib_time_init();

    // direct call to main, ignore gcc
    main();

    // ops
    esp_restart_noos();
}

// __attribute__((weak))
// struct _reent *__getreent(void)
// {
//     return _GLOBAL_REENT;
// }

// int esp_app_get_elf_sha256(char* dst, size_t size)
// {
//     return 0;
// }

int __attribute__((weak)) main(void)
{
    esp_rom_printf("\nweak linked main() in startup.c.\n");

    for (int i = 10; i >= 0; i--) {
        printf("countdown in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    esp_rom_printf("loop forever...\n");
    while (1)
    {
    };
}

/****************************************************************************
 * local implemenation
*****************************************************************************/
static void startup_disable_wdt(void)
{
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
    REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, 0);

    wdt_hal_context_t rwdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};
    wdt_hal_write_protect_disable(&rwdt_ctx);
    wdt_hal_set_flashboot_en(&rwdt_ctx, false);
    wdt_hal_write_protect_enable(&rwdt_ctx);

    //Disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
    wdt_hal_write_protect_disable(&mwdt_ctx);
    wdt_hal_set_flashboot_en(&mwdt_ctx, false);
    wdt_hal_write_protect_enable(&mwdt_ctx);
}
