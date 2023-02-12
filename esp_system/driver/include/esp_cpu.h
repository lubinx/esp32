/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <features.h>
#include <stdbool.h>
#include <stdint.h>
#include "soc/soc_caps.h"

#ifdef __XTENSA__
    #include "xtensa/xtensa_api.h"
    #include "xt_utils.h"
#elif __riscv
    #include "riscv/rv_utils.h"
#endif

// #include "esp_intr_alloc.h"
// #include "soc/soc_caps.h"

// #include "esp_err.h"
typedef int esp_err_t;

    /**
     * @brief Interrupt descriptor flags of esp_cpu_intr_desc_t
     */
    #define ESP_CPU_INTR_DESC_FLAG_SPECIAL      0x01    /**< The interrupt is a special interrupt (e.g., a CPU timer interrupt) */
    #define ESP_CPU_INTR_DESC_FLAG_RESVD        0x02    /**< The interrupt is reserved for internal use */

    /**
     * @brief CPU interrupt handler type
     */
    typedef void (*esp_cpu_intr_handler_t)(void *arg);

__BEGIN_DECLS

/***************************************************************************/
/** Atomic
****************************************************************************/
extern __attribute__((nonnull, nothrow))
    bool esp_cpu_compare_and_set(volatile uint32_t *addr, uint32_t compare_value, uint32_t new_value);

/***************************************************************************/
/** CPU Control
****************************************************************************/
extern __attribute__((nothrow))
    void esp_cpu_stall(int core_id);

extern __attribute__((nothrow))
    void esp_cpu_unstall(int core_id);

extern __attribute__((nothrow))
    void esp_cpu_reset(int core_id);

/***************************************************************************/
/** CPU Registers
****************************************************************************/
    typedef uint32_t esp_cpu_cycle_count_t;

#ifdef __XTENSA__
    #define esp_cpu_get_core_id()       \
        (int)xt_utils_get_core_id()
    #define esp_cpu_get_sp()            \
        xt_utils_get_sp()

    #define esp_cpu_get_cycle_count()   \
        xt_utils_get_cycle_count()
    #define esp_cpu_set_cycle_count(cycle_count)    \
        xt_utils_set_cycle_count(cycle_count)
#else
    #define esp_cpu_get_core_id()       \
        (int)rv_utils_get_core_id()
    #define esp_cpu_get_sp()            \
        rv_utils_get_sp()

    #define esp_cpu_get_cycle_count()   \
        rv_utils_get_cycle_count()
    #define esp_cpu_set_cycle_count(cycle_count)    \
        rv_utils_set_cycle_count(cycle_count)
#endif

/***************************************************************************/
/** CPU Interrupts
****************************************************************************/
    enum esp_cpu_intr_type_t
    {
        ESP_CPU_INTR_TYPE_LEVEL     = 0,
        ESP_CPU_INTR_TYPE_EDGE,
        ESP_CPU_INTR_TYPE_NA,
    };
    typedef enum esp_cpu_intr_type_t esp_cpu_intr_type_t;

    /**
     * @brief CPU interrupt descriptor
     *
     * Each particular CPU interrupt has an associated descriptor describing that
     * particular interrupt's characteristics. Call esp_cpu_intr_get_desc() to get
     * the descriptors of a particular interrupt.
     */
    struct esp_cpu_intr_desc_t
    {
        int priority;               /**< Priority of the interrupt if it has a fixed priority, (-1) if the priority is configurable. */
        esp_cpu_intr_type_t type;   /**< Whether the interrupt is an edge or level type interrupt, ESP_CPU_INTR_TYPE_NA if the type is configurable. */
        uint32_t flags;             /**< Flags indicating extra details. */
    };
    typedef struct esp_cpu_intr_desc_t esp_cpu_intr_desc_t;

    /**
     * @brief Get a CPU interrupt's descriptor
     *
     * Each CPU interrupt has a descriptor describing the interrupt's capabilities
     * and restrictions. This function gets the descriptor of a particular interrupt
     * on a particular CPU.
     *
     * @param[in] core_id The core's ID
     * @param[in] intr_nb Interrupt number
     * @param[out] intr_desc_ret The interrupt's descriptor
     */
extern __attribute__((nonnull, nothrow))
    void esp_cpu_intr_get_desc(int core_id, int intr_nb, esp_cpu_intr_desc_t *intr_desc_ret);

// ---- Interrupt Configuration
#ifdef __XTENSA__
    #define esp_cpu_intr_set_ivt_addr(ivt)  \
        xt_utils_set_vecbase((uintptr_t)ivt)

    #define esp_cpu_intr_has_handler(intr_nb)   \
        xt_int_has_handler(intr_nb, esp_cpu_get_core_id())
    #define esp_cpu_intr_set_handler(intr_nb, hdr, arg) \
        xt_set_interrupt_handler(intr_nb, (xt_handler)hdr, arg);
    #define esp_cpu_intr_get_handler_arg(intr_nb)   \
        xt_get_interrupt_handler_arg(intr_nb)
#else
    #define esp_cpu_intr_set_ivt_addr(ivt)  \
        rv_utils_set_mtvec((uintptr_t)ivt)

    #define esp_cpu_intr_has_handler(intr_nb)   \
        intr_handler_get(intr_nb)
    #define esp_cpu_intr_set_handler(intr_nb, hdr, arg) \
        intr_handler_set(intr_nb, (intr_handler_t)hdr, arg);
    #define esp_cpu_intr_get_handler_arg(intr_nb)   \
        intr_handler_get_arg(intr_nb)

    // #if SOC_CPU_HAS_FLEXIBLE_INTC
    #define esp_cpu_intr_get_type(intr_nb)  \
        (INTR_TYPE_LEVEL == esprv_intc_int_get_type(intr_nb) ? ESP_CPU_INTR_TYPE_LEVEL : ESP_CPU_INTR_TYPE_EDGE)
    #define esp_cpu_intr_set_type(intr_nb, intr_type)   \
        esprv_intc_int_set_type(intr_nb, ESP_CPU_INTR_TYPE_LEVEL == intr_type ? INTR_TYPE_LEVEL : INTR_TYPE_EDGE)

    #define esp_cpu_intr_get_priority(intr_nb)  \
        esprv_intc_int_get_priority(intr_nb)
    #define esp_cpu_intr_set_priority(intr_nb, intr_prio)   \
        esprv_intc_int_set_priority(intr_nb, intr_prio);
    // #endif
#endif

#ifdef __XTENSA__
    #define esp_cpu_intr_enable(intr_mask)  \
        xt_ints_on(intr_mask)
    #define esp_cpu_intr_disable(intr_mask) \
        xt_ints_off(intr_mask)
    #define esp_cpu_intr_get_enabled_mask() \
        xt_utils_intr_get_enabled_mask()

    #define esp_cpu_intr_waitfor()      \
        xt_utils_wait_for_intr()

    #define esp_cpu_intr_clear(intr_nb) \
        xthal_set_intclear(1 << intr_nb)
#else
    #define esp_cpu_intr_enable(intr_mask)  \
        rv_utils_intr_enable(intr_mask)
    #define esp_cpu_intr_disable(intr_mask) \
        rv_utils_intr_disable(intr_mask)
    #define esp_cpu_intr_get_enabled_mask() \
        rv_utils_intr_get_enabled_mask()

    #define esp_cpu_intr_waitfor()      \
        rv_utils_wait_for_intr()

    #define esp_cpu_intr_clear(intr_nb) \
        rv_utils_intr_edge_ack(1 << intr_nb)
#endif

// cmsis like
    #define __WFI                       esp_cpu_intr_waitfor

// esp-idf
    #define esp_cpu_wait_for_intr       esp_cpu_intr_waitfor
    #define esp_cpu_intr_edge_ack       esp_cpu_intr_clear

/***************************************************************************/
/** Debugger
 *      esp_cpu_dbgr_is_attached()
 *      xt_utils_dbgr_break()
 *      esp_cpu_pc_to_addr()
****************************************************************************/
#ifdef __XTENSA__
    #define esp_cpu_dbgr_is_attached()  \
        xt_utils_dbgr_is_attached()

    #define esp_cpu_dbgr_break()        \
        xt_utils_dbgr_break()

    #define esp_cpu_pc_to_addr(pc)      \
        ((void *)((pc & 0x3fffffffU) | 0x40000000U))

    /* Both Xtensa and RISC-V have 2-byte instructions, so to get this right we
     * should decode the preceding instruction as if it is 2-byte, check if it is a call,
     * else treat it as 3 or 4 byte one. However for the cases where this function is
     * used, being off by one instruction is usually okay, so this is kept simple for now.
    */
    #define esp_cpu_get_caller_addr(return_address) \
        (return_address - 3)
#else
    #define esp_cpu_dbgr_is_attached()  \
        rv_utils_dbgr_is_attached()

    #define esp_cpu_dbgr_break()        \
        rv_utils_dbgr_break()


    #define esp_cpu_pc_to_addr(pc)      \
        ((void *)pc)
    #define esp_cpu_get_caller_addr(return_address) \
        (return_address - 4)
#endif

extern __attribute__((nonnull, nothrow))
    esp_err_t esp_cpu_set_breakpoint(int bp_nb, void const *bp_addr);
extern __attribute__((nothrow))
    esp_err_t esp_cpu_clear_breakpoint(int bp_nb);

    enum esp_cpu_watchpoint_trigger_t
    {
        ESP_CPU_WATCHPOINT_LOAD,
        ESP_CPU_WATCHPOINT_STORE,
        ESP_CPU_WATCHPOINT_ACCESS,
    };
    typedef enum esp_cpu_watchpoint_trigger_t esp_cpu_watchpoint_trigger_t;

extern __attribute__((nonnull, nothrow))
    esp_err_t esp_cpu_set_watchpoint(int wp_nb, void const *wp_addr, size_t size, esp_cpu_watchpoint_trigger_t trigger);
extern __attribute__((nothrow))
    esp_err_t esp_cpu_clear_watchpoint(int wp_nb);

/***************************************************************************/
/** Memory Ports
****************************************************************************/
/**
 * @brief Configure the CPU to disable access to invalid memory regions
 */
extern __attribute__((nothrow))
    void esp_cpu_configure_region_protection(void);

__END_DECLS
