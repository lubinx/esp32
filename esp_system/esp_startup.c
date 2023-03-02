#include "soc.h"
#include "esp_system.h"

// TODO: remove these reqiured by ets_set_appcpu_boot_addr()
#include "esp_rom_sys.h"
#include "esp32s3/rom/ets_sys.h"

#include "sdkconfig.h"

static char const *TAG = "startup";

/****************************************************************************
 *  imports
*****************************************************************************/
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

    SOC_initialize();

    extern void __libc_retarget_init(void); //  _retarget_init.c
    __libc_retarget_init();

    do_global_ctors();

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

    do_system_init_fn();

    SOC_core_unstall(1);
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

/****************************************************************************
 *  local
*****************************************************************************/
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
    SOC_initialize();

    #if (CONFIG_IDF_TARGET_ESP32 && CONFIG_ESP32_TRAX_TWOBANKS) || (CONFIG_IDF_TARGET_ESP32S3 && CONFIG_ESP32S3_TRAX_TWOBANKS)
        trax_start_trace(TRAX_DOWNCOUNT_WORDS);
    #endif

    do_system_init_fn();
    esp_startup_start_app_other_cores();
}

void __attribute__((weak)) esp_startup_start_app_other_cores(void)
{
    while (1);
}
