/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <sys/cdefs.h>

#include "xtensa/config/extreg.h"
#include "xtensa/xtruntime.h"
#include "xt_instr_macros.h"

__BEGIN_DECLS

static inline __attribute__((always_inline))
    unsigned xt_utils_intr_get_enabled_mask(void)
    {
        return __RSR(INTENABLE);
    }

static inline __attribute__((always_inline))
    void xt_utils_set_breakpoint(int bp_num, unsigned bp_addr)
    {
        //Set the breakpoint's address
        if (1 == bp_num)
            __WSR(IBREAKA_1, bp_addr);
        else
            __WSR(IBREAKA_0, bp_addr);

        //Enable the breakpoint
        unsigned brk_ena_reg = __RSR(IBREAKENABLE) | (1U << bp_num);
        __WSR(IBREAKENABLE, brk_ena_reg);
    }

static inline __attribute__((always_inline))
    void xt_utils_clear_breakpoint(int bp_num)
    {
        // Disable the breakpoint using the break enable register
        unsigned bp_en = __RSR(IBREAKENABLE) & ~(1U << bp_num);
        __WSR(IBREAKENABLE, bp_en);
        // Zero the break address register
        unsigned bp_addr = 0;

        if (1 == bp_num)
            __WSR(IBREAKA_1, bp_addr);
        else
            __WSR(IBREAKA_0, bp_addr);
    }

static inline __attribute__((always_inline))
    void xt_utils_set_watchpoint(int wp_num, unsigned wp_addr, unsigned size, int on_read, int on_write)
    {
        // Initialize DBREAKC bits (see Table 4–143 or isa_rm.pdf)
        unsigned dbreakc_reg = 0x3F;
        dbreakc_reg = dbreakc_reg << (__builtin_ffs(size) - 1);
        dbreakc_reg = dbreakc_reg & 0x3F;
        if (on_read)
            dbreakc_reg |= (1U << 30);
        if (on_write)
            dbreakc_reg |= (1U << 31);

        // Enable break address and break control register
        if (1 == wp_num)
        {
            __WSR(DBREAKA_1, (unsigned) wp_addr);
            __WSR(DBREAKC_1, dbreakc_reg);
        }
        else
        {
            __WSR(DBREAKA_0, (unsigned) wp_addr);
            __WSR(DBREAKC_0, dbreakc_reg);
        }
    }

static inline __attribute__((always_inline))
    void xt_utils_clear_watchpoint(int wp_num)
    {
        // Clear both break control and break address register
        if (1 == wp_num)
        {
            __WSR(DBREAKC_1, 0);
            __WSR(DBREAKA_1, 0);
        }
        else
        {
            __WSR(DBREAKC_0, 0);
            __WSR(DBREAKA_0, 0);
        }
    }

static inline __attribute__((always_inline))
    int xt_utils_dbgr_is_attached(void)
    {
        unsigned dcr = 0;
        unsigned reg = DSRSET;
        RER(reg, dcr);
        return 0 < (dcr & 0x1);
    }

static inline __attribute__((always_inline))
    void xt_utils_dbgr_break(void)
    {
        __asm__ ("break 1,15");
    }

#define __dbgr_is_attached()            xt_utils_dbgr_is_attached()
#define __dbgr_break()                  xt_utils_dbgr_break()
#define __BKPT(value)                   (__dbgr_is_attached() ? __dbgr_break(): (void)value)

/*
    use gcc buildin __sync_bool_compare_and_swap instead?
static inline __attribute__((always_inline)) bool xt_utils_compare_and_set(volatile unsigned *addr, unsigned compare_value, unsigned new_value)
{
#if XCHAL_HAVE_S32C1I
    #ifdef __clang_analyzer__
        //Teach clang-tidy that "addr" cannot be const as it can be updated by S32C1I instruction
        volatile unsigned temp;
        temp = *addr;
        *addr = temp;
    #endif
    // Atomic compare and set using S32C1I instruction
    unsigned old_value = new_value;
    __asm__ __volatile__ (
        "WSR    %2, SCOMPARE1 \n"
        "S32C1I %0, %1, 0 \n"
        :"=r"(old_value)
        :"r"(addr), "r"(compare_value), "0"(old_value)
    );
    return (old_value == compare_value);
#else // XCHAL_HAVE_S32C1I
    // Single core target has no atomic CAS instruction. We can achieve atomicity by disabling interrupts
    unsigned intr_level;
    __asm__ __volatile__ ("rsil %0, " XTSTR(XCHAL_EXCM_LEVEL) "\n"
                          : "=r"(intr_level));
    // Compare and set
    unsigned old_value;
    old_value = *addr;
    if (old_value == compare_value) {
        *addr = new_value;
    }
    // Restore interrupts
    __asm__ __volatile__ ("memw \n"
                          "wsr %0, ps\n"
                          :: "r"(intr_level));

    return (old_value == compare_value);
#endif // XCHAL_HAVE_S32C1I
}
*/

__END_DECLS
