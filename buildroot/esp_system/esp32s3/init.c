#include <string.h>

#include "sdkconfig.h"

#include "esp_chip_info.h"
#include "esp_clk_internal.h"
#include "esp_cpu.h"
// #include "esp_efuse.h"
#include "esp_memprot.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"

#include "esp_rom_efuse.h"
#include "esp_rom_uart.h"
#include "esp_rom_sys.h"
#include "esp_rom_caps.h"

#include "soc/assist_debug_reg.h"
#include "soc/rtc.h"
#include "soc/periph_defs.h"
#include "soc/system_reg.h"

#include "esp32s3/rtc.h"
#include "esp32s3/rom/ets_sys.h"

#include "esp_private/cache_err_int.h"
#include "esp_private/esp_clk.h"
#include "esp_private/sleep_gpio.h"
#include "esp_private/startup_internal.h"
#include "esp_private/system_internal.h"

#if CONFIG_ESP32_TRAX || CONFIG_ESP32S2_TRAX || CONFIG_ESP32S3_TRAX
    #include "esp_private/trax.h"
#endif

static char const *TAG = "system_init";

/****************************************************************************
 *  imports
*****************************************************************************/
extern int _vector_table;

extern int _rtc_bss_start;
extern int _rtc_bss_end;

extern int _bss_start;
extern int _bss_end;
//  CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY
extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;
// CONFIG_ESP32_IRAM_AS_8BIT_ACCESSIBLE_MEMORY
extern int _iram_bss_start;
extern int _iram_bss_end;

/****************************************************************************
 *  local
*****************************************************************************/
static void core_intr_matrix_clear(void);
static void core_cpu1_entry(void);

/****************************************************************************
 *  exports
*****************************************************************************/
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void __attribute__((weak)) app_main(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("infinite loop...\n");
    while (1)
    {
        esp_rom_printf("*");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void startup_resume_other_cores(void)
{
}

void SystemInit(void)
{
    // Move exception vectors to IRAM
    esp_cpu_intr_set_ivt_addr(&_vector_table);

    //Clear BSS. Please do not attempt to do any complex stuff (like early logging) before this.
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    #if defined(CONFIG_IDF_TARGET_ESP32) && defined(CONFIG_ESP32_IRAM_AS_8BIT_ACCESSIBLE_MEMORY)
        // Clear IRAM BSS
        memset(&_iram_bss_start, 0, (&_iram_bss_end - &_iram_bss_start) * sizeof(_iram_bss_start));
    #endif

    soc_reset_reason_t reset_reason = esp_rom_get_reset_reason(0);

    #if SOC_RTC_FAST_MEM_SUPPORTED || SOC_RTC_SLOW_MEM_SUPPORTED
        /* Unless waking from deep sleep (implying RTC memory is intact), clear RTC bss */
        if (RESET_REASON_CORE_DEEP_SLEEP != reset_reason)
            memset(&_rtc_bss_start, 0, (&_rtc_bss_end - &_rtc_bss_start) * sizeof(_rtc_bss_start));
    #endif

    /*
    #if CONFIG_ESPTOOLPY_OCT_FLASH && !CONFIG_ESPTOOLPY_FLASH_MODE_AUTO_DETECT
        bool efuse_opflash_en = efuse_ll_get_flash_type();
        if (!efuse_opflash_en) {
            ESP_EARLY_LOGE(TAG, "Octal Flash option selected, but EFUSE not configured!");
            abort();
        }
    #endif

    esp_mspi_pin_init();
    spi_flash_init_chip_state();
    #if CONFIG_IDF_TARGET_ESP32S3
        //On other chips, this feature is not provided by HW, or hasn't been tested yet.
        spi_timing_flash_tuning();
    #endif
    */

    #if CONFIG_SPIRAM_BOOT_INIT
        if (esp_psram_init() != ESP_OK)
        {
        #if CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY
            ESP_EARLY_LOGE(TAG, "Failed to init external RAM, needed for external .bss segment");
            abort();
        #endif

        #if CONFIG_SPIRAM_IGNORE_NOTFOUND
            ESP_EARLY_LOGI(TAG, "Failed to init external RAM; continuing without it.");
        #else
            ESP_EARLY_LOGE(TAG, "Failed to init external RAM!");
            abort();
        #endif
        }
    #endif

    #if CONFIG_SPIRAM_MEMTEST
        if (esp_psram_is_initialized()) {
            bool ext_ram_ok = esp_psram_extram_test();
            if (!ext_ram_ok) {
                ESP_EARLY_LOGE(TAG, "External RAM failed memory test!");
                abort();
            }
        }
    #endif  //CONFIG_SPIRAM_MEMTEST

    #if CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY
        memset(&_ext_ram_bss_start, 0, (&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));
    #endif

    // Enable trace memory and immediately start trace.
    #if CONFIG_ESP32_TRAX || CONFIG_ESP32S2_TRAX || CONFIG_ESP32S3_TRAX
        #if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
            #if CONFIG_ESP32_TRAX_TWOBANKS || CONFIG_ESP32S3_TRAX_TWOBANKS
                trax_enable(TRAX_ENA_PRO_APP);
            #else
                trax_enable(TRAX_ENA_PRO);
            #endif
        #elif CONFIG_IDF_TARGET_ESP32S2
            trax_enable(TRAX_ENA_PRO);
        #endif
        trax_start_trace(TRAX_DOWNCOUNT_WORDS);
    #endif

    esp_clk_init();
    esp_perip_clk_init();
    ESP_EARLY_LOGI(TAG, "protocol cpu running...");

    // Now that the clocks have been set-up, set the startup time from RTC
    // and default RTC-backed system time provider.
    g_startup_time = esp_rtc_get_time_us();

    // Clear interrupt matrix for PRO CPU core
    core_intr_matrix_clear();

    // TODO: on FPGA it should be possible to configure this, not currently working with APB_CLK_FREQ changed
    #ifndef CONFIG_IDF_ENV_FPGA
        #ifdef CONFIG_ESP_CONSOLE_UART
            uint32_t clock_hz = esp_clk_apb_freq();
            #if ESP_ROM_UART_CLK_IS_XTAL
                clock_hz = esp_clk_xtal_freq(); // From esp32-s3 on, UART clock source is selected to XTAL in ROM
            #endif
            esp_rom_uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);
            esp_rom_uart_set_clock_baudrate(CONFIG_ESP_CONSOLE_UART_NUM, clock_hz, CONFIG_ESP_CONSOLE_UART_BAUDRATE);
        #endif
    #endif

    // Need to unhold the IOs that were hold right before entering deep sleep, which are used as wakeup pins
    if (RESET_REASON_CORE_DEEP_SLEEP == reset_reason)
        esp_deep_sleep_wakeup_io_reset();

    esp_cache_err_int_init();

    #if CONFIG_ESP_SYSTEM_MEMPROT_FEATURE && !CONFIG_ESP_SYSTEM_MEMPROT_TEST
        // Memprot cannot be locked during OS startup as the lock-on prevents any PMS changes until a next reboot
        // If such a situation appears, it is likely an malicious attempt to bypass the system safety setup -> print error & reset

        bool is_locked = false;
        if (esp_mprot_is_conf_locked_any(&is_locked) != ESP_OK || is_locked)
        {
            ESP_EARLY_LOGE(TAG, "Memprot feature locked after the system reset! Potential safety corruption, rebooting.");
            esp_restart_noos_dig();
        }

        esp_memp_config_t memp_cfg = ESP_MEMPROT_DEFAULT_CONFIG();
        #if ! CONFIG_ESP_SYSTEM_MEMPROT_FEATURE_LOCK
            memp_cfg.lock_feature = false;
        #endif

        esp_err_t memp_err = esp_mprot_set_prot(&memp_cfg);
        if (memp_err != ESP_OK)
        {
            ESP_EARLY_LOGE(TAG, "Failed to set Memprot feature (0x%08X: %s), rebooting.", memp_err, esp_err_to_name(memp_err));
            esp_restart_noos_dig();
        }
    #endif

    /*
    #if CONFIG_SPI_FLASH_SIZE_OVERRIDE
        int app_flash_size = esp_image_get_flash_size(fhdr.spi_size);
        if (app_flash_size < 1 * 1024 * 1024) {
            ESP_EARLY_LOGE(TAG, "Invalid flash size in app image header.");
            abort();
        }
        bootloader_flash_update_size(app_flash_size);
    #endif
    */

    ESP_EARLY_LOGI(TAG, "Starting application cpu, entry point is %p", core_cpu1_entry);
    esp_cpu_unstall(1);

    if (! REG_GET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN))
    {
        REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN);
        REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RUNSTALL);
        REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETING);
        REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETING);
    }
    ets_set_appcpu_boot_addr((uint32_t)core_cpu1_entry);

    SYS_STARTUP_FN();
}

static void core_intr_matrix_clear(void)
{
    uint32_t core_id = esp_cpu_get_core_id();

    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);
    }
}

static void core_cpu1_entry(void)
{
    ets_set_appcpu_boot_addr(0);
    esp_cpu_intr_set_ivt_addr(&_vector_table);

    #if CONFIG_ESP_CONSOLE_NONE
        esp_rom_install_channel_putc(1, NULL);
        esp_rom_install_channel_putc(2, NULL);
    #else
        esp_rom_install_uart_printf();
        esp_rom_uart_set_as_console(CONFIG_ESP_CONSOLE_UART_NUM);
    #endif

    REG_WRITE(ASSIST_DEBUG_CORE_1_RCD_PDEBUGENABLE_REG, 1);
    REG_WRITE(ASSIST_DEBUG_CORE_1_RCD_RECORDING_REG, 1);

    core_intr_matrix_clear();
    esp_cache_err_int_init();

    #if (CONFIG_IDF_TARGET_ESP32 && CONFIG_ESP32_TRAX_TWOBANKS) || \
        (CONFIG_IDF_TARGET_ESP32S3 && CONFIG_ESP32S3_TRAX_TWOBANKS)
        trax_start_trace(TRAX_DOWNCOUNT_WORDS);
    #endif

    SYS_STARTUP_FN();
}
