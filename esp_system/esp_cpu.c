#include <stdint.h>
#include <assert.h>

#include "esp_cpu.h"
#include "esp_err.h"

#include "soc/soc.h"
#include "soc/soc_caps.h"
#include "soc/rtc_cntl_reg.h"

/* --------------------------------------------------- CPU Control -----------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */
void esp_cpu_stall(int core_id)
{
    assert(core_id >= 0 && core_id < SOC_CPU_CORES_NUM);
#if SOC_CPU_CORES_NUM > 1   // We don't allow stalling of the current core
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
#endif
}

void esp_cpu_unstall(int core_id)
{
    assert(core_id >= 0 && core_id < SOC_CPU_CORES_NUM);
#if SOC_CPU_CORES_NUM > 1   // We don't allow stalling of the current core
    /*
    We need to write clear the value "0x86" to unstall a particular core. The location of this value is split into
    two separate bit fields named "c0" and "c1", and the two fields are located in different registers. Each core has
    its own pair of "c0" and "c1" bit fields.

    Note: This function can be called when the cache is disabled. We use "ternary if" instead of an array so that the
    "rodata" of the register masks/shifts will be stored in this function's "rodata" section, instead of the source
    file's "rodata" section (see IDF-5214).
    */
    int rtc_cntl_c0_m = (core_id == 0) ? RTC_CNTL_SW_STALL_PROCPU_C0_M : RTC_CNTL_SW_STALL_APPCPU_C0_M;
    int rtc_cntl_c1_m = (core_id == 0) ? RTC_CNTL_SW_STALL_PROCPU_C1_M : RTC_CNTL_SW_STALL_APPCPU_C1_M;
    CLEAR_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, rtc_cntl_c0_m);
    CLEAR_PERI_REG_MASK(RTC_CNTL_SW_CPU_STALL_REG, rtc_cntl_c1_m);
#endif
}

void esp_cpu_reset(int core_id)
{
#if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2// TODO: IDF-5645
    SET_PERI_REG_MASK(LP_AON_CPUCORE0_CFG_REG, LP_AON_CPU_CORE0_SW_RESET);
#else
    assert(core_id >= 0 && core_id < SOC_CPU_CORES_NUM);
#if SOC_CPU_CORES_NUM > 1
    /*
    Note: This function can be called when the cache is disabled. We use "ternary if" instead of an array so that the
    "rodata" of the register masks/shifts will be stored in this function's "rodata" section, instead of the source
    file's "rodata" section (see IDF-5214).
    */
    int rtc_cntl_rst_m = (core_id == 0) ? RTC_CNTL_SW_PROCPU_RST_M : RTC_CNTL_SW_APPCPU_RST_M;
#else // SOC_CPU_CORES_NUM > 1
    int rtc_cntl_rst_m = RTC_CNTL_SW_PROCPU_RST_M;
#endif // SOC_CPU_CORES_NUM > 1
    SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, rtc_cntl_rst_m);
#endif
}

// ---------------- Interrupt Descriptors ------------------

#if SOC_CPU_HAS_FLEXIBLE_INTC
    static bool is_intr_nb_resv(int intr_nb)
    {
        // Workaround to reserve interrupt number 1 for Wi-Fi, 5,8 for Bluetooth, 6 for "permanently disabled interrupt"
        // [TODO: IDF-2465]
        uint32_t reserved = BIT(1) | BIT(5) | BIT(6) | BIT(8);

        // int_num 0,3,4,7 are inavaliable for PULP cpu
        #if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2// TODO: IDF-5728 replace with a better macro name
            reserved |= BIT(0) | BIT(3) | BIT(4) | BIT(7);
        #endif

        if (reserved & BIT(intr_nb))
            return true;

        extern int _vector_table;
        extern int _interrupt_handler;
        intptr_t const pc = (intptr_t)(&_vector_table + intr_nb);

        /* JAL instructions are relative to the PC there are executed from. */
        intptr_t const destination = pc + riscv_decode_offset_from_jal_instruction(pc);

        return destination != (intptr_t)&_interrupt_handler;
    }

    void esp_cpu_intr_get_desc(int core_id, int intr_nb, esp_cpu_intr_desc_t *intr_desc_ret)
    {
        intr_desc_ret->priority = 1;    //Todo: We should make this -1
        intr_desc_ret->type = ESP_CPU_INTR_TYPE_NA;
        #if __riscv
            intr_desc_ret->flags = is_intr_nb_resv(intr_nb) ? ESP_CPU_INTR_DESC_FLAG_RESVD : 0;
        #else
            intr_desc_ret->flags = 0;
        #endif
    }
#else
    typedef struct
    {
        int priority;
        esp_cpu_intr_type_t type;
        uint32_t flags[SOC_CPU_CORES_NUM];
    } intr_desc_t;

    #if SOC_CPU_CORES_NUM > 1
        // Note: We currently only have dual core targets, so the table initializer is hard coded
        static intr_desc_t const intr_desc_table[SOC_CPU_INTR_NUM] =
        {
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //0
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //1
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //2
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //3
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      0                               }}, //4
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //5
        #if CONFIG_FREERTOS_CORETIMER_0
            {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //6
        #else
            {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //6
        #endif
            {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //7
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //8
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //9
            {1, ESP_CPU_INTR_TYPE_EDGE,     {0,                                 0                               }}, //10
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //11
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0, 0}}, //12
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0, 0}}, //13
            {7, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //14, NMI
        #if CONFIG_FREERTOS_CORETIMER_1
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //15
        #else
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //15
        #endif
            {5, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //16
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //17
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //18
            {2, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //19
            {2, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //20
            {2, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //21
            {3, ESP_CPU_INTR_TYPE_EDGE,     {ESP_CPU_INTR_DESC_FLAG_RESVD,      0                               }}, //22
            {3, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //23
            {4, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      0                               }}, //24
            {4, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //25
            {5, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //26
            {3, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //27
            {4, ESP_CPU_INTR_TYPE_EDGE,     {0,                                 0                               }}, //28
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //29
            {4, ESP_CPU_INTR_TYPE_EDGE,     {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //30
            {5, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //31
        };
    #else
        static intr_desc_t const intr_desc_table[SOC_CPU_INTR_NUM] =
        {
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //0
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //1
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //2
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //3
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //4
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //5
        #if CONFIG_FREERTOS_CORETIMER_0
            {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //6
        #else
            {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL }},     //6
        #endif
            {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL }},     //7
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //8
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //9
            {1, ESP_CPU_INTR_TYPE_EDGE,     {0                               }},    //10
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL }},     //11
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //12
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //13
            {7, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //14, NMI
        #if CONFIG_FREERTOS_CORETIMER_1
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //15
        #else
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL }},     //15
        #endif
            {5, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL }},     //16
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //17
            {1, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //18
            {2, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //19
            {2, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //20
            {2, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //21
            {3, ESP_CPU_INTR_TYPE_EDGE,     {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //22
            {3, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //23
            {4, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //24
            {4, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //25
            {5, ESP_CPU_INTR_TYPE_LEVEL,    {0                               }},    //26
            {3, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //27
            {4, ESP_CPU_INTR_TYPE_EDGE,     {0                               }},    //28
            {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL }},     //29
            {4, ESP_CPU_INTR_TYPE_EDGE,     {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //30
            {5, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD    }},    //31
        };
    #endif

    void esp_cpu_intr_get_desc(int core_id, int intr_nb, esp_cpu_intr_desc_t *intr_desc_ret)
    {
        assert(core_id >= 0 && core_id < SOC_CPU_CORES_NUM);
    #if SOC_CPU_CORES_NUM == 1
        core_id = 0;    //  If this is a single core target, hard code CPU ID to 0
    #endif

        intr_desc_ret->priority = intr_desc_table[intr_nb].priority;
        intr_desc_ret->type = intr_desc_table[intr_nb].type;
        intr_desc_ret->flags = intr_desc_table[intr_nb].flags[core_id];
    }
#endif

/* ---------------------------------------------------- Debugging ------------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */
esp_err_t esp_cpu_set_breakpoint(int bp_nb, const void *bp_addr)
{
    assert(SOC_CPU_BREAKPOINTS_NUM > 0);

    /*
    Todo:
    - Check that bp_nb is in range
    */
#if __XTENSA__
    xt_utils_set_breakpoint(bp_nb, (uint32_t)bp_addr);
#else
    if (__dbgr_is_attached())
    {
        /* If we want to set breakpoint which when hit transfers control to debugger
        * we need to set `action` in `mcontrol` to 1 (Enter Debug Mode).
        * That `action` value is supported only when `dmode` of `tdata1` is set.
        * But `dmode` can be modified by debugger only (from Debug Mode).
        *
        * So when debugger is connected we use special syscall to ask it to set breakpoint for us.
        */
        long args[] = {true, bp_nb, (long)bp_addr};

        if (0 == semihosting_call_noerrno(ESP_SEMIHOSTING_SYS_BREAKPOINT_SET, args))
            return ESP_ERR_INVALID_RESPONSE;
    }
    rv_utils_set_breakpoint(bp_nb, (uint32_t)bp_addr);
#endif
    return ESP_OK;
}

esp_err_t esp_cpu_clear_breakpoint(int bp_nb)
{
    /*
    Todo:
    - Check if the bp_nb is valid
    */
#if __XTENSA__
    xt_utils_clear_breakpoint(bp_nb);
#else
    if (__dbgr_is_attached())
    {
        // See description in esp_cpu_set_breakpoint()
        long args[] = {false, bp_nb};

        if (0 == semihosting_call_noerrno(ESP_SEMIHOSTING_SYS_BREAKPOINT_SET, args);)
            return ESP_ERR_INVALID_RESPONSE;
    }
    rv_utils_clear_breakpoint(bp_nb);
#endif
    return ESP_OK;
}

esp_err_t esp_cpu_set_watchpoint(int wp_nb, const void *wp_addr, size_t size, esp_cpu_watchpoint_trigger_t trigger)
{
    assert(SOC_CPU_WATCHPOINTS_NUM > 0);

    /*
    Todo:
    - Check that wp_nb is in range
    - Check if the wp_nb is already in use
    */
    // Check if size is 2^n, where n is in [0...6]
    if (size < 1 || size > 64 || (size & (size - 1)) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    int on_read = (trigger == ESP_CPU_WATCHPOINT_LOAD || trigger == ESP_CPU_WATCHPOINT_ACCESS);
    int on_write = (trigger == ESP_CPU_WATCHPOINT_STORE || trigger == ESP_CPU_WATCHPOINT_ACCESS);
#if __XTENSA__
    xt_utils_set_watchpoint(wp_nb, (uint32_t)wp_addr, size, on_read, on_write);
#else
    if (__dbgr_is_attached()) {
        // See description in esp_cpu_set_breakpoint()
        long args[] = {true, wp_nb, (long)wp_addr, (long)size,
            (long)((on_read ? ESP_SEMIHOSTING_WP_FLG_RD : 0) | (on_write ? ESP_SEMIHOSTING_WP_FLG_WR : 0))
        };

        if (0 == semihosting_call_noerrno(ESP_SEMIHOSTING_SYS_WATCHPOINT_SET, args))
            return ESP_ERR_INVALID_RESPONSE;
    }
    rv_utils_set_watchpoint(wp_nb, (uint32_t)wp_addr, size, on_read, on_write);
#endif
    return ESP_OK;
}

esp_err_t esp_cpu_clear_watchpoint(int wp_nb)
{
    /*
    Todo:
    - Check if the wp_nb is valid
    */
#if __XTENSA__
    xt_utils_clear_watchpoint(wp_nb);
#else
    if (__dbgr_is_attached())
    {
        // See description in __dbgr_is_attached()
        long args[] = {false, wp_nb};

        if (0 == semihosting_call_noerrno(ESP_SEMIHOSTING_SYS_WATCHPOINT_SET, args))
            return ESP_ERR_INVALID_RESPONSE;
    }
    rv_utils_clear_watchpoint(wp_nb);
#endif // __XTENSA__
    return ESP_OK;
}
