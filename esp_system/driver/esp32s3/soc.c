#include "soc.h"
#include "clk_tree.h"
#include "esp_system.h"
#include "esp_log.h"

#include "cache_ll.h"

// TODO: remove these reqiured by ets_set_appcpu_boot_addr()
#include "esp_rom_sys.h"
#include "esp32s3/rom/ets_sys.h"

static char const *TAG = "SoC";

/****************************************************************************
 *  imports
*****************************************************************************/
extern void *_vector_table;
// esp system extends ctors
extern struct __esp_init_fn _esp_system_init_fn_array_start;
extern struct __esp_init_fn _esp_system_init_fn_array_end;

/****************************************************************************
 *  @internal
 ****************************************************************************/
static void esp_cache_err_int_init(int core_id);

/****************************************************************************
 *  @implements
 ****************************************************************************/
void SOC_initialize(void)
{
    __set_VECBASE(&_vector_table);

    int core_id = __get_CORE_ID();
    // clear intr matrix?
    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++)
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);

    esp_cache_err_int_init(core_id);

    if (0 == core_id)
    {
        CLK_TREE_initialize();

        /*
        REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_PDEBUGENABLE_REG, 1);
        REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_RECORDING_REG, 1);
        */
    }
    else
    {
        /*
        REG_WRITE(ASSIST_DEBUG_CORE_1_RCD_PDEBUGENABLE_REG, 1);
        REG_WRITE(ASSIST_DEBUG_CORE_1_RCD_RECORDING_REG, 1);
        */
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

    do_system_init_fn();
    esp_rtos_bootstrap();
}

void __esp_rtos_initialize(void)
{
    #if CONFIG_SW_COEXIST_ENABLE || CONFIG_EXTERNAL_COEX_ENABLE
        esp_coex_adapter_register(&g_coex_adapter_funcs);
        coex_pre_init();
    #endif

    do_system_init_fn();

    SOC_acquire_core(1);
    ets_set_appcpu_boot_addr((uint32_t)startup_other_cores);
}


unsigned SOC_cache_err_core_id(void)
{
    if (cache_ll_l1_get_access_error_intr_status(0, CACHE_LL_L1_ACCESS_EVENT_MASK))
        return PRO_CPU_NUM;

    if (cache_ll_l1_get_access_error_intr_status(1, CACHE_LL_L1_ACCESS_EVENT_MASK))
        return APP_CPU_NUM;

    return -1;
}

void SOC_reset(void)
{
    RTCCNTL.options0.sw_sys_rst = 1;
    while (1);
}

void SOC_reset_core(int core_id)
{
    assert((unsigned)core_id < SOC_CPU_CORES_NUM);
    /*
    Note: This function can be called when the cache is disabled. We use "ternary if" instead of an array so that the
    "rodata" of the register masks/shifts will be stored in this function's "rodata" section, instead of the source
    file's "rodata" section (see IDF-5214).
    */
    int rtc_cntl_rst_m = (core_id == 0) ? RTC_CNTL_SW_PROCPU_RST_M : RTC_CNTL_SW_APPCPU_RST_M;
    SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, rtc_cntl_rst_m);
}

void SOC_acquire_core(int core_id)
{
    assert((unsigned)core_id < SOC_CPU_CORES_NUM);
    /*
    We need to write clear the value "0x86" to unstall a particular core. The location of this value is split into
    two separate bit fields named "c0" and "c1", and the two fields are located in different registers. Each core has
    its own pair of "c0" and "c1" bit fields.

    Note: This function can be called when the cache is disabled. We use "ternary if" instead of an array so that the
    "rodata" of the register masks/shifts will be stored in this function's "rodata" section, instead of the source
    file's "rodata" section (see IDF-5214).
    */
    if (0 == core_id)
    {
        RTCCNTL.options0.sw_stall_procpu_c0 = RTC_CNTL_SW_STALL_PROCPU_C0_V;
        RTCCNTL.sw_cpu_stall.procpu_c1 = RTC_CNTL_SW_STALL_PROCPU_C1_V;
    }
    else
    {
        RTCCNTL.options0.sw_stall_appcpu_c0 = RTC_CNTL_SW_STALL_APPCPU_C0_V;
        RTCCNTL.sw_cpu_stall.appcpu_c1 = RTC_CNTL_SW_STALL_APPCPU_C1_V;

        if (! SYSTEM.core_1_control_0.control_core_1_clkgate_en)
        {
            SYSTEM.core_1_control_0.control_core_1_clkgate_en = 1;
            SYSTEM.core_1_control_0.control_core_1_runstall = 0;

            SYSTEM.core_1_control_0.control_core_1_reseting = 1;
            SYSTEM.core_1_control_0.control_core_1_reseting = 0;
        }
    }
}

void SOC_release_core(int core_id)
{
    assert((unsigned)core_id < SOC_CPU_CORES_NUM);
    /*
    We need to write the value "0x86" to stall a particular core. The write location is split into two separate
    bit fields named "c0" and "c1", and the two fields are located in different registers. Each core has its own pair of
    "c0" and "c1" bit fields.

    Note: This function can be called when the cache is disabled. We use "ternary if" instead of an array so that the
    "rodata" of the register masks/shifts will be stored in this function's "rodata" section, instead of the source
    file's "rodata" section (see IDF-5214).
    */
    int rtc_cntl_c0_m = (core_id == 0) ? RTC_CNTL_SW_STALL_PROCPU_C0_M : RTC_CNTL_SW_STALL_APPCPU_C0_M;
    int rtc_cntl_c0_s = (core_id == 0) ? RTC_CNTL_SW_STALL_PROCPU_C0_S : RTC_CNTL_SW_STALL_APPCPU_C0_S;
    int rtc_cntl_c1_m = (core_id == 0) ? RTC_CNTL_SW_STALL_PROCPU_C1_M : RTC_CNTL_SW_STALL_APPCPU_C1_M;
    int rtc_cntl_c1_s = (core_id == 0) ? RTC_CNTL_SW_STALL_PROCPU_C1_S : RTC_CNTL_SW_STALL_APPCPU_C1_S;

    CLEAR_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, rtc_cntl_c0_m);
    SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, 2 << rtc_cntl_c0_s);
    CLEAR_PERI_REG_MASK(RTC_CNTL_SW_CPU_STALL_REG, rtc_cntl_c1_m);
    SET_PERI_REG_MASK(RTC_CNTL_SW_CPU_STALL_REG, 0x21 << rtc_cntl_c1_s);
}

/****************************************************************************
 *  @internal
 ****************************************************************************/
static void esp_cache_err_int_init(int core_id)
{
    SOC_disable_intr_nb(ETS_CACHEERR_INUM);

    // We do not register a handler for the interrupt because it is interrupt
    // level 4 which is not serviceable from C. Instead, xtensa_vectors.S has
    // a call to the panic handler for this interrupt.
    esp_rom_route_intr_matrix(core_id, ETS_CACHE_IA_INTR_SOURCE, ETS_CACHEERR_INUM);

    // Enable invalid cache access interrupt when the cache is disabled.
    // When the interrupt happens, we can not determine the CPU where the
    // invalid cache access has occurred. We enable the interrupt to catch
    // invalid access on both CPUs, but the interrupt is connected to the
    // CPU which happens to call this function.
    // For this reason, panic handler backtrace will not be correct if the
    // interrupt is connected to PRO CPU and invalid access happens on the APP CPU.

    ESP_DRAM_LOGV(TAG, "illegal error intr clr & ena mask is: 0x%x", CACHE_LL_L1_ILG_EVENT_MASK);
    //illegal error intr doesn't depend on cache_id
    cache_ll_l1_clear_illegal_error_intr(0, CACHE_LL_L1_ILG_EVENT_MASK);
    cache_ll_l1_enable_illegal_error_intr(0, CACHE_LL_L1_ILG_EVENT_MASK);

    if (core_id == PRO_CPU_NUM) {
        esp_rom_route_intr_matrix(core_id, ETS_CACHE_CORE0_ACS_INTR_SOURCE, ETS_CACHEERR_INUM);

        /* On the hardware side, stat by clearing all the bits reponsible for
         * enabling cache access error interrupts.  */
        ESP_DRAM_LOGV(TAG, "core 0 access error intr clr & ena mask is: 0x%x", CACHE_LL_L1_ACCESS_EVENT_MASK);
        cache_ll_l1_clear_access_error_intr(0, CACHE_LL_L1_ACCESS_EVENT_MASK);
        cache_ll_l1_enable_access_error_intr(0, CACHE_LL_L1_ACCESS_EVENT_MASK);
    } else {
        esp_rom_route_intr_matrix(core_id, ETS_CACHE_CORE1_ACS_INTR_SOURCE, ETS_CACHEERR_INUM);

        /* On the hardware side, stat by clearing all the bits reponsible for
         * enabling cache access error interrupts.  */
        ESP_DRAM_LOGV(TAG, "core 1 access error intr clr & ena mask is: 0x%x", CACHE_LL_L1_ACCESS_EVENT_MASK);
        cache_ll_l1_clear_access_error_intr(1, CACHE_LL_L1_ACCESS_EVENT_MASK);
        cache_ll_l1_enable_access_error_intr(1, CACHE_LL_L1_ACCESS_EVENT_MASK);
    }

    SOC_enable_intr_nb(ETS_CACHEERR_INUM);
}
