#include <string.h>

#include "sdkconfig.h"

#include "esp_rom_caps.h"
#include "esp_rom_efuse.h"
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"

#include "esp_clk_internal.h"
#include "esp_cpu.h"
#include "esp_heap_caps_init.h"
// #include "esp_efuse.h"
#include "esp_log.h"
#include "esp_memprot.h"
#include "esp_newlib.h"
#include "esp_timer.h"

#include "soc/assist_debug_reg.h"
#include "soc/rtc.h"
#include "soc/periph_defs.h"

#include "esp32s3/rtc.h"
#include "esp32s3/rom/ets_sys.h"

#include "esp_private/cache_err_int.h"
#include "esp_private/esp_clk.h"
#include "esp_private/startup_internal.h"
#include "esp_private/system_internal.h"

#if CONFIG_ESP32_TRAX || CONFIG_ESP32S2_TRAX || CONFIG_ESP32S3_TRAX
    #include "esp_private/trax.h"
#endif

#if CONFIG_VFS_SUPPORT_IO
    #include "esp_vfs_dev.h"
    #include "esp_vfs_console.h"
#endif

static char const *TAG = "system_init";
uint64_t g_startup_time = 0;

/****************************************************************************
 *  imports
*****************************************************************************/
extern int _vector_table;

extern int _rtc_bss_start;
extern int _rtc_bss_end;

#if CONFIG_BOOT_ROM_LOG_ALWAYS_OFF
    #define ROM_LOG_MODE                ESP_EFUSE_ROM_LOG_ALWAYS_OFF
#elif CONFIG_BOOT_ROM_LOG_ON_GPIO_LOW
    #define ROM_LOG_MODE                ESP_EFUSE_ROM_LOG_ON_GPIO_LOW
#elif CONFIG_BOOT_ROM_LOG_ON_GPIO_HIGH
    #define ROM_LOG_MODE                ESP_EFUSE_ROM_LOG_ON_GPIO_HIGH
#endif

/****************************************************************************
 *  local
*****************************************************************************/
static void core_intr_matrix_clear(void);
static void core_other_cpu_init(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void SystemInit(void)
{
    // Move exception vectors to IRAM
    esp_cpu_intr_set_ivt_addr(&_vector_table);

    soc_reset_reason_t reset_reason = esp_rom_get_reset_reason(0);

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
    /*
    if (RESET_REASON_CORE_DEEP_SLEEP == reset_reason)
        esp_deep_sleep_wakeup_io_reset();
    */

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

    #ifdef ROM_LOG_MODE
        esp_efuse_set_rom_log_scheme(ROM_LOG_MODE);
    #endif

    #ifdef CONFIG_SECURE_FLASH_ENC_ENABLED
        esp_flash_encryption_init_checks();
    #endif

    heap_caps_init();

    esp_timer_early_init();
    esp_newlib_init();

    #if CONFIG_VFS_SUPPORT_IO
        esp_vfs_console_register();
    #endif

    #if defined(CONFIG_VFS_SUPPORT_IO) && !defined(CONFIG_ESP_CONSOLE_NONE)
        static char const *default_stdio_dev = "/dev/console/";
        esp_reent_init(_GLOBAL_REENT);
        _GLOBAL_REENT->_stdin  = fopen(default_stdio_dev, "r");
        _GLOBAL_REENT->_stdout = fopen(default_stdio_dev, "w");
        _GLOBAL_REENT->_stderr = fopen(default_stdio_dev, "w");
        #if ESP_ROM_NEEDS_SWSETUP_WORKAROUND
            /*
            - This workaround for printf functions using 32-bit time_t after the 64-bit time_t upgrade
            - The 32-bit time_t usage is triggered through ROM Newlib functions printf related functions calling __swsetup_r() on
            the first call to a particular file pointer (i.e., stdin, stdout, stderr)
            - Thus, we call the toolchain version of __swsetup_r() now (before any printf calls are made) to setup all of the
            file pointers. Thus, the ROM newlib code will never call the ROM version of __swsetup_r().
            - See IDFGH-7728 for more details
            */
            extern int __swsetup_r(struct _reent *, FILE *);
            __swsetup_r(_GLOBAL_REENT, _GLOBAL_REENT->_stdout);
            __swsetup_r(_GLOBAL_REENT, _GLOBAL_REENT->_stderr);
            __swsetup_r(_GLOBAL_REENT, _GLOBAL_REENT->_stdin);
        #endif
    #else
        _REENT_SMALL_CHECK_INIT(_GLOBAL_REENT);
    #endif
}

ESP_SYSTEM_INIT_FN(init_components0, BIT(0), 200)
{
    #if CONFIG_ESP_DEBUG_STUBS_ENABLE
        extern void esp_dbg_stubs_init(void);
        esp_dbg_stubs_init();
    #endif

    #if defined(CONFIG_PM_ENABLE)
        extern void esp_pm_impl_init(void);
        esp_pm_impl_init();
    #endif

    #if SOC_APB_BACKUP_DMA
        extern void esp_apb_backup_dma_lock_init(void);
        esp_apb_backup_dma_lock_init();
    #endif

    #if CONFIG_SW_COEXIST_ENABLE || CONFIG_EXTERNAL_COEX_ENABLE
        esp_coex_adapter_register(&g_coex_adapter_funcs);
        coex_pre_init();
    #endif

    return ESP_OK;
}

ESP_SYSTEM_INIT_FN(startup_other_cores, BIT(1), 201)
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

    return ESP_OK;
}

/****************************************************************************
 *  local
*****************************************************************************/
static void core_intr_matrix_clear(void)
{
    uint32_t core_id = esp_cpu_get_core_id();

    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);
    }
}
