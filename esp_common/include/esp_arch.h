#ifndef __ESP_ARCH__H
#define __ESP_ARCH__H                   1

#ifdef __XTENSA__
    #include "xt_utils.h"
    #include "xtensa/xtensa_api.h"

    typedef xt_handler intr_handler_t;
#elif __riscv
    #include "riscv/rv_utils.h"

    typedef intr_handler_t intr_handler_t;
#endif

static inline
    bool __intr_nb_has_handler(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        return xt_int_has_handler(intr_nb, __get_CORE_ID());
    #else
        return (void *)0 != intr_handler_get(intr_nb);
    #endif
    }

static inline
    intr_handler_t __intr_nb_set_handler(unsigned intr_nb, intr_handler_t handler, void* arg)
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
    void *__intr_nb_get_arg(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        return xt_get_interrupt_handler_arg((int)intr_nb);
    #else
        return intr_handler_get_arg(intr_nb);
    #endif
    }

    /**
     *  entable interrupt
    */
static inline
    void __intr_nb_enable(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        xt_ints_on(1 << intr_nb);
    #else
        rv_utils_intr_enable(1 << intr_nb);
    #endif
    }

    /**
     *  disable interrrupt
    */
static inline
    void __intr_nb_disable(unsigned intr_nb)
    {
    #ifdef __XTENSA__
        xt_ints_off(1 << intr_nb);
    #else
        rv_utils_intr_disable(1 << intr_nb);
    #endif
    }

static inline
    void __intr_nb_clear_pending(unsigned intr_nb)
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
    unsigned __cache_err_core_id(void);

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

/****************************************************************************
 *  esp-idf
*****************************************************************************/
    #define esp_cpu_get_core_id()       \
        __get_CORE_ID()
    #define esp_cpu_intr_edge_ack(intr_nb)  \
        __intr_nb_clear_pending(intr_nb)

static inline
    unsigned esp_cpu_intr_get_enabled_mask(void)
    {
    #ifdef __XTENSA__
        return xt_utils_intr_get_enabled_mask();
    #else
        return rv_utils_intr_get_enabled_mask();
    #endif
    }

    // NOTE: esp-idf perfer to enable/disable interrupts by mask, this consider is dangers~!
static inline __attribute__((deprecated("using __intr_nb_enable() instead")))
    void esp_cpu_intr_enable(unsigned intr_mask)
    {
    #ifdef __XTENSA__
        xt_ints_on(intr_mask);
    #else
        rv_utils_intr_enable(intr_mask);
    #endif
    }

    // NOTE: esp-idf perfer to enable/disable interrupts by mask, this consider is dangers~!
static inline __attribute__((deprecated("using __intr_nb_disable() instead")))
    void esp_cpu_intr_disable(unsigned intr_mask)
    {
    #ifdef __XTENSA__
        xt_ints_off(intr_mask);
    #else
        rv_utils_intr_disable(intr_mask);
    #endif
    }

#endif
