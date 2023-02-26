#include "esp_system.h"

#include "soc.h"
#include "clk_tree.h"

#include "esp_compiler.h"
#include "esp_cpu.h"
#include "esp_private/cache_err_int.h"

#include "esp_rom_sys.h"
#include "esp32s3/rom/ets_sys.h"

#include "sdkconfig.h"

static char const *TAG = "startup";

/****************************************************************************
 *  imports
*****************************************************************************/
extern void *_vector_table;
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;

// gcc ctors
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);
// esp system extends ctors
extern struct __esp_init_fn _esp_system_init_fn_array_start;
extern struct __esp_init_fn _esp_system_init_fn_array_end;

// App entry point for core 0
extern __attribute__((noreturn)) void esp_startup_start_app(void);
// App entry point for core [1..X]
extern __attribute__((noreturn)) void esp_startup_start_app_other_cores(void);

/****************************************************************************
 *  local
*****************************************************************************/
static void core_intr_matrix_clear(void);
static void core_other_cpu_init(void);

static void do_global_ctors(void);
static void do_system_init_fn(void);
static void startup_other_cores(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void Startup_Handler(void)
{
    /* copy table loaded by bootlader
    struct copy_table_t
    {
        uint32_t const *src;
        uint32_t *dst;
        uint32_t  size;
    };

    for (struct copy_table_t const *tbl = (struct copy_table_t const *)&__copy_table_start__;
        tbl < (struct copy_table_t const *)&__copy_table_end__;
        tbl ++)
    {
        for(uint32_t i = 0; i < tbl->size / sizeof(uint32_t); i ++)
            tbl->dst[i] = tbl->src[i];
    }
    */

    struct zero_table_t
    {
        uint32_t *dst;
        uint32_t size;
    };

    for (struct zero_table_t const *tbl = (struct zero_table_t const *)&__zero_table_start__;
        tbl < (struct zero_table_t const *)&__zero_table_end__;
        tbl ++)
    {
        for(uint32_t i = 0; i < tbl->size / sizeof(*tbl->dst); i ++)
            tbl->dst[i] = 0;
    }

    // Move exception vectors to IRAM
    __set_VECBASE(&_vector_table);

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

    CLK_TREE_initialize();

    // Clear interrupt matrix for PRO CPU core
    core_intr_matrix_clear();

    // TODO: on FPGA it should be possible to configure this, not currently working with APB_CLK_FREQ changed
    #ifndef CONFIG_IDF_ENV_FPGA
        uint32_t clock_hz;

        #if ESP_ROM_UART_CLK_IS_XTAL
            clock_hz = CLK_TREE_sclk_freq(SOC_SCLK_XTAL);
        #else
            clock_hz = CLK_TREE_apb_freq();
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

    do_global_ctors();
    do_system_init_fn();

    esp_cpu_unstall(1);
    if (! REG_GET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN))
    {
        REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN);
        REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RUNSTALL);

        REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETING);
        REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETING);
    }

    ets_set_appcpu_boot_addr((uint32_t)startup_other_cores);
    esp_startup_start_app();
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
    __set_VECBASE(&_vector_table);

    REG_WRITE(ASSIST_DEBUG_CORE_1_RCD_PDEBUGENABLE_REG, 1);
    REG_WRITE(ASSIST_DEBUG_CORE_1_RCD_RECORDING_REG, 1);

    core_intr_matrix_clear();
    esp_cache_err_int_init();

    #if (CONFIG_IDF_TARGET_ESP32 && CONFIG_ESP32_TRAX_TWOBANKS) || (CONFIG_IDF_TARGET_ESP32S3 && CONFIG_ESP32S3_TRAX_TWOBANKS)
        trax_start_trace(TRAX_DOWNCOUNT_WORDS);
    #endif

    return ESP_OK;
}

/****************************************************************************
 *  local
*****************************************************************************/
static void core_intr_matrix_clear(void)
{
    uint32_t core_id = __get_CORE_ID();

    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++)
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);
}

static void do_global_ctors(void)
{
    #ifdef CONFIG_COMPILER_CXX_EXCEPTIONS
        struct object { long placeholder[10]; };
        extern void __register_frame_info (const void *begin, struct object *ob);
        extern char __eh_frame[];

        static struct object ob;
        __register_frame_info(__eh_frame, &ob);
    #endif

    for (void (**p)(void) = &__init_array_start;
        p < &__init_array_end;
        p ++)
    {
        (*p)();
    }
}

static void do_system_init_fn(void)
{
    int core_id = __get_CORE_ID();

    for (struct __esp_init_fn *p = &_esp_system_init_fn_array_start;
        p < &_esp_system_init_fn_array_end;
        p ++)
    {
        if (p->cores & BIT(core_id))
        {
            esp_err_t err = (*(p->fn))();
            if (ESP_OK != err) abort();
        }
    }
}

static void startup_other_cores(void)
{
    do_system_init_fn();
    esp_startup_start_app_other_cores();
}

void __attribute__((weak)) esp_startup_start_app_other_cores(void)
{
    while (1);
}
