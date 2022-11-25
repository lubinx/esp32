/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "hal/cache_hal.h"
#include "hal/mmu_hal.h"
#include "hal/wdt_hal.h"
// #include "esp_rom_sys.h"

// #include "esp_log.h"
// #include "esp_rom_sys.h"

/****************************************************************************
 *  import
*****************************************************************************/
extern int _bss_start;
extern int _bss_end;
extern int _data_start;
extern int _data_end;

/****************************************************************************
 *  local declaration
*****************************************************************************/
static void startup_disable_wdt(void);

/****************************************************************************
 * export
*****************************************************************************/
void __attribute__((noreturn)) call_start_cpu0(void)
{
    startup_disable_wdt();
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    /**
     *  configure:
     *      SOC_SHARED_IDCACHE_SUPPORTED
     *      CONFIG_FREERTOS_UNICORE
    */
    cache_hal_init();

    /**
     *  configure:
     *      CONFIG_FREERTOS_UNICORE
    */
    mmu_hal_init();

    // if (ESP_OK != bootloader_init())
    //     bootloader_reset();

    while (1)
    {
    };
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
