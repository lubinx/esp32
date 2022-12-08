#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

#include "soc/soc.h"
#include "soc/rtc.h"

#include "hal/mmu_hal.h"
#include "hal/cache_hal.h"
#include "hal/wdt_hal.h"

#include "bootloader_flash_config.h"
#include "bootloader_flash.h"
#include "bootloader_clock.h"
#include "bootloader_soc.h"
#include "bootloader_mem.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_flash.h"

static const char *TAG = "boot";
static int n1 = 10;
static int n2 = 20;
static int n3 = 30;
static int n4 = 40;
static int n5 = 50;
static int n6 = 60;
static int n7 = 70;
static int n8 = 80;
static int n9 = 90;

/****************************************************************************
 *  imports
*****************************************************************************/
extern __attribute__((noreturn))
    void IRAM_ATTR call_start_cpu0(void);

extern intptr_t _bss_start;
extern intptr_t _bss_end;
extern intptr_t _flash_rodata_start;
extern intptr_t _flash_rodata_end;
extern intptr_t _flash_text_start;
extern intptr_t _flash_text_end;

/****************************************************************************
 *  local
*****************************************************************************/
static void bootloader_config_wdt(void);
static void bootloader_mmap(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void IRAM_ATTR bootloader_startup(void)
{
    esp_rom_printf("\n\n\n1. hello call_start_cpu0 \
        \ttag: %s\n\
        \tn1: %d\n\
        \tn2: %d\n\
        \tn3: %d\n\
        \tn4: %d\n\
        \tn5: %d\n\
        \tn6: %d\n\
        \tn7: %d\n\
        \tn8: %d\n\
        \tn9: %d\n",
        TAG, n1, n2, n3, n4, n5, n6, n7, n8, n9
    );

    extern char const *foobar_text;

    esp_rom_printf("rom test addr: 0x%8x\n\
        \t_flash_rodata_start: 0x%08x\n\
        \t_flash_rodata_end: 0x%08x\n\
        \t_flash_text_start: 0x%08x\n\
        \t_flash_text_end: 0x%08x\n",
        foobar_text,
        &_flash_rodata_start, &_flash_rodata_end,
        &_flash_text_start, &_flash_text_end
    );


    int ret = ESP_OK;

#if XCHAL_ERRATUM_572
    uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
    WSR(MEMCTL, memctl);
#endif // XCHAL_ERRATUM_572

    // protect memory region
    bootloader_init_mem();
    // clear bss section
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));;

    // init eFuse virtual mode (read eFuses to RAM)
#ifdef CONFIG_EFUSE_VIRTUAL
    esp_rom_printf("eFuse virtual mode is enabled. If Secure boot or Flash encryption is enabled then it does not provide any security. FOR TESTING ONLY!\n");
    #ifndef CONFIG_EFUSE_VIRTUAL_KEEP_IN_FLASH
        esp_efuse_init_virtual_mode_in_ram();
    #endif
#endif

    cache_hal_init();
    mmu_hal_init();

    // Enable WDT, BOR, and GLITCH reset
    bootloader_ana_super_wdt_reset_config(true);
    bootloader_ana_bod_reset_config(true);
    bootloader_ana_clock_glitch_reset_config(true);

    // bootloader_super_wdt_auto_feed();
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
    REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, 0);

    bootloader_clock_configure();
    bootloader_flash_update_id();

    // Check and run XMC startup flow
    if ((ret = bootloader_flash_xmc_startup()) != ESP_OK)
    {
        esp_rom_printf("failed when running XMC startup flow, reboot!");
        goto err;
    }

    bootloader_config_wdt();

    if (ESP_OK != ret)
    {
err:
        esp_rom_delay_us(1000); /* Allow last byte to leave FIFO */
        esp_rom_software_reset_system();
    }

    esp_rom_printf("\n\n\n2. hello call_start_cpu0: after bootloader_init()\n\n\n");

    // extern void foobar(void);
    // foobar();

    bootloader_mmap();


    while (1) {}

    // call_start_cpu0();
}

__attribute__((weak))
void app_main(void)
{
    // esp_rom_printf("infinite loop...\n");

    // // printf("infinite loop...\n");

    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);

    // esp_restart();
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
#if CONFIG_IDF_TARGET_ESP32C6 // TODO: IDF-5653
    wdt_hal_context_t rwdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &LP_WDT};
#else
    wdt_hal_context_t rwdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};
#endif
    wdt_hal_write_protect_disable(&rwdt_ctx);
    wdt_hal_set_flashboot_en(&rwdt_ctx, false);
    wdt_hal_write_protect_enable(&rwdt_ctx);

#ifdef CONFIG_BOOTLOADER_WDT_ENABLE
    //Initialize and start RWDT to protect the  for bootloader if configured to do so
    esp_rom_printf("Enabling RTCWDT(%d ms)\n", CONFIG_BOOTLOADER_WDT_TIME_MS);

    wdt_hal_init(&rwdt_ctx, WDT_RWDT, 0, false);
    uint32_t stage_timeout_ticks = (uint32_t)((uint64_t)CONFIG_BOOTLOADER_WDT_TIME_MS * rtc_clk_slow_freq_get_hz() / 1000);
    wdt_hal_write_protect_disable(&rwdt_ctx);
    wdt_hal_config_stage(&rwdt_ctx, WDT_STAGE0, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_RTC);
    wdt_hal_enable(&rwdt_ctx);
    wdt_hal_write_protect_enable(&rwdt_ctx);
#endif

    //Disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
    wdt_hal_write_protect_disable(&mwdt_ctx);
    wdt_hal_set_flashboot_en(&mwdt_ctx, false);
    wdt_hal_write_protect_enable(&mwdt_ctx);
}

static void bootloader_mmap(void)
{
    cache_hal_disable(CACHE_TYPE_ALL);

    /*
        drom_addr: 10020, drom_load_addr: 3c020020, drom_size: 37104
        irom_addr: 20020, irom_load_addr: 42000020, irom_size: 103416
    */

    {
        cache_bus_mask_t bus_mask;
        uint32_t length;

        mmu_hal_map_region(0, MMU_TARGET_FLASH0,
            &_flash_rodata_end - &_flash_rodata_start, _flash_rodata_start,
            &_flash_rodata_end - &_flash_rodata_start,
            &length);

        esp_rom_printf("mapped: %u\n", length);
    }
}
