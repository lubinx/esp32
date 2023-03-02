#ifndef __ESP_ARCH__H
#define __ESP_ARCH__H                   1

#ifdef __XTENSA__
    #include "xt_utils.h"
    #include "xtensa/xtensa_api.h"

    typedef xt_handler esp_intr_handler_t;
#elif __riscv
    #include "riscv/rv_utils.h"

    typedef intr_handler_t esp_intr_handler_t;

    #if SOC_CPU_HAS_FLEXIBLE_INTC
        #define esp_cpu_intr_get_type(intr_nb)  \
            (INTR_TYPE_LEVEL == esprv_intc_int_get_type(intr_nb) ? ESP_CPU_INTR_TYPE_LEVEL : ESP_CPU_INTR_TYPE_EDGE)
        #define esp_cpu_intr_set_type(intr_nb, intr_type)   \
            esprv_intc_int_set_type(intr_nb, ESP_CPU_INTR_TYPE_LEVEL == intr_type ? INTR_TYPE_LEVEL : INTR_TYPE_EDGE)

        #define esp_cpu_intr_get_priority(intr_nb)  \
            esprv_intc_int_get_priority(intr_nb)
        #define esp_cpu_intr_set_priority(intr_nb, intr_prio)   \
            esprv_intc_int_set_priority(intr_nb, intr_prio)
    #endif
#endif

static inline
    bool SOC_intr_is_handled(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        return xt_int_has_handler(intr_nb, __get_CORE_ID());
    #else
        return (void *)0 != intr_handler_get(intr_nb);
    #endif
    }

static inline
    esp_intr_handler_t SOC_intr_set_handler(unsigned intr_nb, esp_intr_handler_t handler, void* arg)
    {
    #ifdef __XTENSA__
        return xt_set_interrupt_handler(intr_nb, handler, arg);
    #else
        void *retval = intr_handler_get(intr_nb, handler, arg);
        intr_handler_set(intr_nb, handler, arg);
        return retval;
    #endif
    }

static inline
    void *SOC_intr_handler_arg(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        return xt_get_interrupt_handler_arg(intr_nb);
    #else
        return intr_handler_get_arg(intr_nb);
    #endif
    }

    /**
     *  entable interrupt
    */
static inline
    void SOC_intr_enable_nb(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        xt_ints_on(1 << intr_nb);
    #else
        rv_utils_intr_enable(1 << intr_nb);
    #endif
    }

    // NOTE: esp-idf perfer using mask to enable/disable interrupts, this consider is dangers~!
static inline __attribute__((deprecated("using SOC_intr_enable_nb() instead")))
    void SOC_intr_enable_mask(unsigned intr_mask)
    {
    #ifdef __XTENSA__
        xt_ints_on(intr_mask);
    #else
        rv_utils_intr_enable(intr_mask);
    #endif
    }

    /**
     *  disable interrrupt
    */
static inline
    void SOC_intr_disable_nb(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        xt_ints_off(1 << intr_nb);
    #else
        rv_utils_intr_disable(1 << intr_nb);
    #endif
    }

    // NOTE: esp-idf perfer using mask to enable/disable interrupts, this consider is dangers~!
static inline __attribute__((deprecated("using SOC_intr_disable_nb() instead")))
    void SOC_intr_disable_mask(unsigned intr_mask)
    {
    #ifdef __XTENSA__
        xt_ints_off(intr_mask);
    #else
        rv_utils_intr_disable(intr_mask);
    #endif
    }

    /*
static inline
    unsigned SOC_enabled_intr(void)
    {
    #ifdef __XTENSA__
        return xt_utils_intr_get_enabled_mask();
    #else
        return rv_utils_intr_get_enabled_mask();
    #endif
    }
    */

    /**
     *  REIVEW: get interrupt mask, this should be __get_IPSR()?
    */

    /**
     *  REVIEW: clear interrupt pending?
    */
static inline
    void SOC_intr_clear_pending(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        xthal_set_intclear(1 << intr_nb);
    #else
        rv_utils_intr_edge_ack(1 << intr_nb);
    #endif
    }

    /**
     *  returns cache error core'id
     *      implement at driver/${target}/soc.c
    */
extern __attribute__((nothrow, const))
    unsigned SOC_cache_err_core_id(void);

/****************************************************************************
 *  debugger
*****************************************************************************/
static inline
    bool __dbgr_is_attached(void)
    {
    #ifdef __XTENSA__
        return xt_utils_dbgr_is_attached();
    #else
        return rv_utils_dbgr_is_attached();
    #endif
    }

static inline
    void __dbgr_break(void)
    {
    #ifdef __XTENSA__
        xt_utils_dbgr_break();
    #else
        rv_utils_dbgr_break();
    #endif
    }

#define __BKPT(value)                   (__dbgr_is_attached() ? __dbgr_break(): (void)value)

// esp-idf
    #define esp_cpu_get_core_id()       \
        __get_CORE_ID()
    #define esp_cpu_intr_enable(mask)   \
        SOC_intr_enable_mask(mask)
    #define esp_cpu_intr_disable(mask)  \
        SOC_intr_disable_mask(mask)
    #define esp_cpu_intr_edge_ack(intr_nb)  \
        SOC_intr_clear_pending(intr_nb)
#endif
