#include "sdkconfig.h"

#include "esp_cpu.h"
#include "esp_heap_caps_init.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_xt_wdt.h"
#include "esp_timer.h"

#include "soc/system_reg.h"
#include "hal/wdt_hal.h"
#include "esp32s3/rom/ets_sys.h"

#include "esp_private/startup_internal.h"

static char const *TAG = "startup_handler";

/****************************************************************************
 *  imports
*****************************************************************************/
// App entry point for core 0
extern void esp_startup_start_app(void);
// App entry point for core [1..X]
extern void esp_startup_start_app_other_cores(void);

extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;

// gcc ctors
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);
// esp system extends ctors
extern esp_system_init_fn_t _esp_system_init_fn_array_start;
extern esp_system_init_fn_t _esp_system_init_fn_array_end;

/****************************************************************************
 *  local
*****************************************************************************/
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
    extern void SystemInit(void);
    SystemInit();

    do_global_ctors();
    do_system_init_fn();

    #if CONFIG_ESP_XT_WDT
        esp_xt_wdt_config_t cfg = {
            .timeout                = CONFIG_ESP_XT_WDT_TIMEOUT,
            .auto_backup_clk_enable = CONFIG_ESP_XT_WDT_BACKUP_CLK_ENABLE,
        };
        err = esp_xt_wdt_init(&cfg);
        assert(err == ESP_OK && "Failed to init xtwdt");
    #endif

    #ifndef CONFIG_BOOTLOADER_WDT_DISABLE_IN_USER_CODE
        #if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2
            wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &LP_WDT};
        #else
            wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};
        #endif
        wdt_hal_write_protect_disable(&rtc_wdt_ctx);
        wdt_hal_disable(&rtc_wdt_ctx);
        wdt_hal_write_protect_enable(&rtc_wdt_ctx);
    #endif

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

    for (void (**p)(void) = &__init_array_end - 1; p >= &__init_array_start; --p)
        (*p)();
}

static void do_system_init_fn(void)
{
    int core_id = esp_cpu_get_core_id();

    for (esp_system_init_fn_t *p = &_esp_system_init_fn_array_start;
        p < &_esp_system_init_fn_array_end;
        p ++)
    {
        if (p->cores & BIT(core_id))
        {
            esp_err_t err = (*(p->fn))();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "init function %p has failed (0x%x), aborting", p->fn, err);
                abort();
            }
        }
    }
}

static void startup_other_cores(void)
{
    do_system_init_fn();
    esp_startup_start_app_other_cores();
}

__attribute__((weak, noreturn))
void  esp_startup_start_app_other_cores(void)
{
    while (1)
        esp_rom_delay_us(UINT32_MAX);
}
