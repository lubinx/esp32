/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __XTENSA__
    #include "xtensa/xtensa_api.h"
    #include "xt_utils.h"
#elif __riscv
    #include "riscv/rv_utils.h"
#else
    #pragma GCC error "unknown arch"
#endif

#include "esp_intr_alloc.h"
#include "esp_err.h"
// #include "soc/soc_caps.h"

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
/* --------------------------------------------------- CPU Control -----------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */

/**
 * @brief Stall a CPU core
 *
 * @param core_id  The core's ID
 */
extern __attribute__((nothrow))
    void esp_cpu_stall(int core_id);

/**
 * @brief Resume a previously stalled CPU core
 *
 * @param core_id The core's ID
 */
extern __attribute__((nothrow))
    void esp_cpu_unstall(int core_id);

extern __attribute__((nothrow))
    void esp_cpu_reset(int core_id);

extern __attribute__((nothrow))
    void esp_cpu_wait_for_intr(void);

/* -------------------------------------------------- CPU Registers ----------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */
static inline __attribute__((always_inline, pure))
    int esp_cpu_get_core_id(void)
    {
        //Note: Made "pure" to optimize for single core target
    #ifdef __XTENSA__
        return (int)xt_utils_get_core_id();
    #else
        return (int)rv_utils_get_core_id();
    #endif
}

static inline __attribute__((always_inline))
    void *esp_cpu_get_sp(void)
    {
    #ifdef __XTENSA__
        return xt_utils_get_sp();
    #else
        return rv_utils_get_sp();
    #endif
    }

    typedef uint32_t esp_cpu_cycle_count_t;

static inline __attribute__((always_inline, pure))
    esp_cpu_cycle_count_t esp_cpu_get_cycle_count(void)
    {
    #ifdef __XTENSA__
        return (esp_cpu_cycle_count_t)xt_utils_get_cycle_count();
    #else
        return (esp_cpu_cycle_count_t)rv_utils_get_cycle_count();
    #endif
    }

static inline __attribute__((always_inline))
    void esp_cpu_set_cycle_count(esp_cpu_cycle_count_t cycle_count)
    {
    #ifdef __XTENSA__
        xt_utils_set_cycle_count((uint32_t)cycle_count);
    #else
        rv_utils_set_cycle_count((uint32_t)cycle_count);
    #endif
    }

/**
 * @brief Convert a program counter (PC) value to address
 *
 * If the architecture does not store the true virtual address in the CPU's PC
 * or return addresses, this function will convert the PC value to a virtual
 * address. Otherwise, the PC is just returned
 *
 * @param pc PC value
 * @return Virtual address
 */
static inline __attribute__((always_inline, pure))
    void *esp_cpu_pc_to_addr(uint32_t pc)
    {
    #ifdef __XTENSA__
        // Xtensa stores window rotation in PC[31:30]
        return (void *)((pc & 0x3fffffffU) | 0x40000000U);
    #else
        return (void *)pc;
    #endif
    }

/* ------------------------------------------------- CPU Interrupts ----------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */

// ---------------- Interrupt Descriptors ------------------
    typedef enum {
        ESP_CPU_INTR_TYPE_LEVEL = 0,
        ESP_CPU_INTR_TYPE_EDGE,
        ESP_CPU_INTR_TYPE_NA,
    } esp_cpu_intr_type_t;

    /**
     * @brief CPU interrupt descriptor
     *
     * Each particular CPU interrupt has an associated descriptor describing that
     * particular interrupt's characteristics. Call esp_cpu_intr_get_desc() to get
     * the descriptors of a particular interrupt.
     */
    typedef struct {
        int priority;               /**< Priority of the interrupt if it has a fixed priority, (-1) if the priority is configurable. */
        esp_cpu_intr_type_t type;   /**< Whether the interrupt is an edge or level type interrupt, ESP_CPU_INTR_TYPE_NA if the type is configurable. */
        uint32_t flags;             /**< Flags indicating extra details. */
    } esp_cpu_intr_desc_t;

/**
 * @brief Get a CPU interrupt's descriptor
 *
 * Each CPU interrupt has a descriptor describing the interrupt's capabilities
 * and restrictions. This function gets the descriptor of a particular interrupt
 * on a particular CPU.
 *
 * @param[in] core_id The core's ID
 * @param[in] intr_num Interrupt number
 * @param[out] intr_desc_ret The interrupt's descriptor
 */
extern __attribute__((nothrow))
    void esp_cpu_intr_get_desc(int core_id, int intr_num, esp_cpu_intr_desc_t *intr_desc_ret);

// --------------- Interrupt Configuration -----------------
static inline __attribute__((always_inline))
    void esp_cpu_intr_set_ivt_addr(const void *ivt_addr)
    {
    #ifdef __XTENSA__
        xt_utils_set_vecbase((uint32_t)ivt_addr);
    #else
        rv_utils_set_mtvec((uint32_t)ivt_addr);
    #endif
    }

#if SOC_CPU_HAS_FLEXIBLE_INTC
    static inline __attribute__((always_inline))
        void esp_cpu_intr_set_type(int intr_num, esp_cpu_intr_type_t intr_type)
        {
            esprv_intc_int_set_type(intr_num,
                (ESP_CPU_INTR_TYPE_LEVEL == intr_type) ? INTR_TYPE_LEVEL : INTR_TYPE_EDGE);
        }

    /**
     * @brief Get the current configured type of a particular interrupt
     *
     * Get the currently configured type (i.e., level or edge) of a particular
     * interrupt on the current CPU.
     *
     * @param intr_num Interrupt number (from 0 to 31)
     * @return Interrupt type
     */
    static inline __attribute__((always_inline))
        esp_cpu_intr_type_t esp_cpu_intr_get_type(int intr_num)
        {
            enum intr_type type = esprv_intc_int_get_type(intr_num);
            return (type == INTR_TYPE_LEVEL) ? ESP_CPU_INTR_TYPE_LEVEL : ESP_CPU_INTR_TYPE_EDGE;
        }

    static inline __attribute__((always_inline))
        void esp_cpu_intr_set_priority(int intr_num, int intr_priority)
        {
            esprv_intc_int_set_priority(intr_num, intr_priority);
        }

    static inline __attribute__((always_inline))
        int esp_cpu_intr_get_priority(int intr_num)
        {
            return esprv_intc_int_get_priority(intr_num);
        }
#endif

static inline __attribute__((always_inline))
    bool esp_cpu_intr_has_handler(int intr_num)
    {
    #ifdef __XTENSA__
        return  xt_int_has_handler(intr_num, esp_cpu_get_core_id());
    #else
        return intr_handler_get(intr_num);
    #endif
    }

static inline __attribute__((always_inline))
    void esp_cpu_intr_set_handler(int intr_num, esp_cpu_intr_handler_t handler, void *arg)
    {
    #ifdef __XTENSA__
        xt_set_interrupt_handler(intr_num, (xt_handler)handler, arg);
    #else
        intr_handler_set(intr_num, (intr_handler_t)handler, arg);
    #endif
    }

static inline __attribute__((always_inline))
    void *esp_cpu_intr_get_handler_arg(int intr_num)
    {
    #ifdef __XTENSA__
        return xt_get_interrupt_handler_arg(intr_num);
    #else
        return  intr_handler_get_arg(intr_num);
    #endif
    }

// ------------------ Interrupt Control --------------------
static inline __attribute__((always_inline))
    void esp_cpu_intr_enable(uint32_t intr_mask)
    {
    #ifdef __XTENSA__
        xt_ints_on(intr_mask);
    #else
        rv_utils_intr_enable(intr_mask);
    #endif
    }

static inline __attribute__((always_inline))
    void esp_cpu_intr_disable(uint32_t intr_mask)
    {
    #ifdef __XTENSA__
        xt_ints_off(intr_mask);
    #else
        rv_utils_intr_disable(intr_mask);
    #endif
    }

static inline __attribute__((always_inline))
    uint32_t esp_cpu_intr_get_enabled_mask(void)
    {
    #ifdef __XTENSA__
        return xt_utils_intr_get_enabled_mask();
    #else
        return rv_utils_intr_get_enabled_mask();
    #endif
    }

/**
 * @brief Acknowledge an edge interrupt
 *
 * @param intr_num Interrupt number (from 0 to 31)
 */
static inline __attribute__((always_inline))
    void esp_cpu_intr_edge_ack(int intr_num)
    {
    #ifdef __XTENSA__
        xthal_set_intclear(1 << intr_num);
    #else
        rv_utils_intr_edge_ack(intr_num);
    #endif
    }

/* -------------------------------------------------- Memory Ports -----------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */

/**
 * @brief Configure the CPU to disable access to invalid memory regions
 */
void esp_cpu_configure_region_protection(void);

/* ---------------------------------------------------- Debugging ------------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */

// --------------- Breakpoints/Watchpoints -----------------

#if SOC_CPU_BREAKPOINTS_NUM > 0
    /**
     * @brief Set and enable a hardware breakpoint on the current CPU
     *
     * @note This function is meant to be called by the panic handler to set a
     * breakpoint for an attached debugger during a panic.
     * @note Overwrites previously set breakpoint with same breakpoint number.
     * @param bp_num Hardware breakpoint number [0..SOC_CPU_BREAKPOINTS_NUM - 1]
     * @param bp_addr Address to set a breakpoint on
     * @return ESP_OK if breakpoint is set. Failure otherwise
     */
extern __attribute__((nothrow))
    esp_err_t esp_cpu_set_breakpoint(int bp_num, const void *bp_addr);

    /**
     * @brief Clear a hardware breakpoint on the current CPU
     *
     * @note Clears a breakpoint regardless of whether it was previously set
     * @param bp_num Hardware breakpoint number [0..SOC_CPU_BREAKPOINTS_NUM - 1]
     * @return ESP_OK if breakpoint is cleared. Failure otherwise
     */
extern __attribute__((nothrow))
    esp_err_t esp_cpu_clear_breakpoint(int bp_num);

#endif

/**
 * @brief Set and enable a hardware watchpoint on the current CPU
 *
 * Set and enable a hardware watchpoint on the current CPU, specifying the
 * memory range and trigger operation. Watchpoints will break/panic the CPU when
 * the CPU accesses (according to the trigger type) on a certain memory range.
 *
 * @note Overwrites previously set watchpoint with same watchpoint number.
 * @param wp_num Hardware watchpoint number [0..SOC_CPU_WATCHPOINTS_NUM - 1]
 * @param wp_addr Watchpoint's base address
 * @param size Size of the region to watch. Must be one of 2^n, with n in [0..6].
 * @param trigger Trigger type
 * @return ESP_ERR_INVALID_ARG on invalid arg, ESP_OK otherwise
 */
    typedef enum {
        ESP_CPU_WATCHPOINT_LOAD,
        ESP_CPU_WATCHPOINT_STORE,
        ESP_CPU_WATCHPOINT_ACCESS,
    } esp_cpu_watchpoint_trigger_t;

extern __attribute__((nothrow))
    esp_err_t esp_cpu_set_watchpoint(int wp_num, const void *wp_addr, size_t size, esp_cpu_watchpoint_trigger_t trigger);

/**
 * @brief Clear a hardware watchpoint on the current CPU
 *
 * @note Clears a watchpoint regardless of whether it was previously set
 * @param wp_num Hardware watchpoint number [0..SOC_CPU_WATCHPOINTS_NUM - 1]
 * @return ESP_OK if watchpoint was cleared. Failure otherwise.
 */
extern __attribute__((nothrow))
    esp_err_t esp_cpu_clear_watchpoint(int wp_num);

// ---------------------- Debugger -------------------------
static inline __attribute__((always_inline))
    bool esp_cpu_dbgr_is_attached(void)
    {
    #ifdef __XTENSA__
        return xt_utils_dbgr_is_attached();
    #else
        return rv_utils_dbgr_is_attached();
    #endif
    }

static inline __attribute__((always_inline))
    void esp_cpu_dbgr_break(void)
    {
    #ifdef __XTENSA__
        xt_utils_dbgr_break();
    #else
        rv_utils_dbgr_break();
    #endif
    }

// ---------------------- Instructions -------------------------
/**
 * @brief Given the return address, calculate the address of the preceding call instruction
 * This is typically used to answer the question "where was the function called from?"
 * @param return_address  The value of the return address register.
 *                        Typically set to the value of __builtin_return_address(0).
 * @return Address of the call instruction preceding the return address.
 */
static inline __attribute__((always_inline))
    intptr_t esp_cpu_get_call_addr(intptr_t return_address)
    {
        /* Both Xtensa and RISC-V have 2-byte instructions, so to get this right we
        * should decode the preceding instruction as if it is 2-byte, check if it is a call,
        * else treat it as 3 or 4 byte one. However for the cases where this function is
        * used, being off by one instruction is usually okay, so this is kept simple for now.
        */
    #ifdef __XTENSA__
        return return_address - 3;
    #else
        return return_address - 4;
    #endif
    }

/* ------------------------------------------------------ Misc ---------------------------------------------------------
 *
 * ------------------------------------------------------------------------------------------------------------------ */
/**
 *  Atomic compare-and-set operation
 */
extern __attribute__((nothrow))
    bool esp_cpu_compare_and_set(volatile uint32_t *addr, uint32_t compare_value, uint32_t new_value);

__END_DECLS
