#include <stdbool.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

#include "soc/soc.h"
#include "hal/mmu_hal.h"
#include "hal/cache_hal.h"
#include "hal/wdt_hal.h"

// #include "hal/mpu_types.h"
#include "soc/soc_caps.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"


#if CONFIG_IDF_TARGET_ESP32C6
    #include "soc/hp_apm_reg.h"
    #include "soc/lp_apm_reg.h"
    #include "soc/lp_apm0_reg.h"
#endif

#include "bootloader_soc.h"

/****************************************************************************
 *  imports
*****************************************************************************/
extern intptr_t _bss_start;
extern intptr_t _bss_end;

/****************************************************************************
 *  local
*****************************************************************************/
static void bootloader_config_wdt(void);
static void bootloader_load_kernel(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void bootloader_startup(void)
{
#if XCHAL_ERRATUM_572
    uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
    WSR(MEMCTL, memctl);
#endif // XCHAL_ERRATUM_572

    //Enable WDT, BOR, and GLITCH reset
    bootloader_ana_super_wdt_reset_config(true);
    bootloader_ana_bod_reset_config(true);
    bootloader_ana_clock_glitch_reset_config(true);

    // bootloader_super_wdt_auto_feed
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
    REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, 0);

    #if CONFIG_IDF_TARGET_ESP32C6
        // disable apm filter // TODO: IDF-5909
        REG_WRITE(LP_APM_FUNC_CTRL_REG, 0);
        REG_WRITE(LP_APM0_FUNC_CTRL_REG, 0);
        REG_WRITE(HP_APM_FUNC_CTRL_REG, 0);
    #endif

    #ifdef CONFIG_BOOTLOADER_REGION_PROTECTION_ENABLE
        // protect memory region
        esp_cpu_configure_region_protection();
    #endif

    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    #ifdef CONFIG_EFUSE_VIRTUAL
        ESP_LOGW(TAG, "eFuse virtual mode is enabled. If Secure boot or Flash encryption is enabled then it does not provide any security. FOR TESTING ONLY!");
        #ifndef CONFIG_EFUSE_VIRTUAL_KEEP_IN_FLASH
            esp_efuse_init_virtual_mode_in_ram();
        #endif
    #endif

    esp_rom_printf("hello world!\n");

    cache_hal_init();
    mmu_hal_init();

    bootloader_config_wdt();
    bootloader_load_kernel();

    while(1) {}

/*
    if (ESP_OK != ret)
    {
// startup_failure:
        esp_rom_delay_us(2000000);
        esp_rom_software_reset_system();
    }
*/
}

// libc dummy
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;
}

int pthread_setcancelstate(int state, int *oldstate)
{
    return 0;
}

/****************************************************************************
 *  local
*****************************************************************************/
static void bootloader_config_wdt(void)
{
    /*
     * At this point, the flashboot protection of RWDT and MWDT0 will have been
     * automatically enabled. We can disable flashboot protection as it's not
     * needed anymore. If configured to do so, we also initialize the RWDT to
     * protect the remainder of the bootloader process.
     */

    //Disable RWDT flashboot protection.
    wdt_hal_context_t rwdt_ctx =
        #if CONFIG_IDF_TARGET_ESP32C6 // TODO: IDF-5653
            {.inst = WDT_RWDT, .rwdt_dev = &LP_WDT};
        #else
            {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};
        #endif
    wdt_hal_write_protect_disable(&rwdt_ctx);
    wdt_hal_set_flashboot_en(&rwdt_ctx, false);
    wdt_hal_write_protect_enable(&rwdt_ctx);

#ifdef CONFIG_BOOTLOADER_WDT_ENABLE
    wdt_hal_init(&rwdt_ctx, WDT_RWDT, 0, false);
    wdt_hal_write_protect_disable(&rwdt_ctx);

    wdt_hal_config_stage(&rwdt_ctx, WDT_STAGE0,
        (uint32_t)((uint64_t)CONFIG_BOOTLOADER_WDT_TIME_MS * rtc_clk_slow_freq_get_hz() / 1000),
        WDT_STAGE_ACTION_RESET_RTC
    );
    wdt_hal_enable(&rwdt_ctx);
    wdt_hal_write_protect_enable(&rwdt_ctx);
#endif

    // Disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
    wdt_hal_write_protect_disable(&mwdt_ctx);
    wdt_hal_set_flashboot_en(&mwdt_ctx, false);
    wdt_hal_write_protect_enable(&mwdt_ctx);
}

static void bootloader_load_kernel(void)
{
    cache_hal_disable(CACHE_TYPE_ALL);


    /*
        drom_addr: 10020, drom_load_addr: 3c020020, drom_size: 37104
        irom_addr: 20020, irom_load_addr: 42000020, irom_size: 103416
    */

    // {
    //     cache_bus_mask_t bus_mask;
    //     uint32_t length;

    //     mmu_hal_map_region(0, MMU_TARGET_FLASH0,
    //         &_flash_rodata_end - &_flash_rodata_start, _flash_rodata_start,
    //         &_flash_rodata_end - &_flash_rodata_start,
    //         &length);

    //     esp_rom_printf("mapped: %u\n", length);
    // }
}
