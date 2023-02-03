#include <string.h>

#include "sdkconfig.h"

#include "esp_clk_internal.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "clk_tree.h"

#include "esp_rom_caps.h"
#include "esp_rom_sys.h"

#include "soc/assist_debug_reg.h"
#include "soc/periph_defs.h"

// #include "soc/rtc.h"
#include "esp32s3/rom/ets_sys.h"

#include "esp_private/cache_err_int.h"
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

    // Clear interrupt matrix for PRO CPU core
    core_intr_matrix_clear();

    // TODO: on FPGA it should be possible to configure this, not currently working with APB_CLK_FREQ changed
    #ifndef CONFIG_IDF_ENV_FPGA
        #ifdef CONFIG_ESP_CONSOLE_UART
            uint32_t clock_hz = clk_tree_apb_freq();
            #if ESP_ROM_UART_CLK_IS_XTAL
                clock_hz = clk_tree_xtal_freq(); // From esp32-s3 on, UART clock source is selected to XTAL in ROM
            #endif
            // esp_rom_uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);
            // esp_rom_uart_set_clock_baudrate(CONFIG_ESP_CONSOLE_UART_NUM, clock_hz, CONFIG_ESP_CONSOLE_UART_BAUDRATE);
        #endif
    #endif

    // Need to unhold the IOs that were hold right before entering deep sleep, which are used as wakeup pins
    /*
    if (RESET_REASON_CORE_DEEP_SLEEP == esp_rom_get_reset_reason(0))
        esp_deep_sleep_wakeup_io_reset();
    */

    esp_cache_err_int_init();

    extern void __libc_retarget_init(void); //  _retarget_init.c
    __libc_retarget_init();

    /* TODO: enablie it somehow conflict with vfs => uart driver, somehting outside the memory protect area
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
    */
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
