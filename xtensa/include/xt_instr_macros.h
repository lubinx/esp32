/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "xtensa/xtruntime.h"

#define EXTRA_SAVE_AREA_SIZE            32
#define BASE_SAVE_AREA_SIZE             16
#define SAVE_AREA_OFFSET                (EXTRA_SAVE_AREA_SIZE + BASE_SAVE_AREA_SIZE)
#define BASE_AREA_SP_OFFSET             12

static inline __attribute__((always_inline))
    unsigned __RSR(unsigned reg)
    {
        unsigned ret;
        asm volatile ("rsr %0, %1" : "=r" (ret) : "i" (reg));
        return ret;
    }

static inline __attribute__((always_inline))
    void __WSR(unsigned reg, unsigned val)
    {
        asm volatile ("wsr %0, %1" : : "r" (val), "i" (reg));
    }


static inline __attribute__((always_inline))
    unsigned __RER(unsigned reg)
    {
        unsigned ret;
        asm volatile ("rer %0, %1" : "=r" (ret) : "r" (reg));
        return ret;
    }

static inline __attribute__((always_inline))
    void __WER(unsigned reg, unsigned val)
    {
        asm volatile ("wer %0, %1" :: "r" (val), "r" (reg));
    }


// TODO: deprecated these.. or move to freertos private include?
#define RSR(reg, at)                    (at = __RSR(reg))
#define WSR(reg, val)                    __WSR(reg, val)

#define RER(reg, at)                    (at = __RER(reg))
#define XSR(reg, at)                    asm volatile ("xsr %0, %1" : "+r" (at) : "i" (reg))

#define WITLB(at, as)                   asm volatile ("witlb  %0, %1; \n isync \n " : : "r" (at), "r" (as))
#define WDTLB(at, as)                   asm volatile ("wdtlb  %0, %1; \n dsync \n " : : "r" (at), "r" (as))
// -----------------------------------------------

static inline __attribute__((always_inline))
    void __set_VECBASE(void *vecbase)
    {
        asm volatile ("wsr %0, vecbase" :: "r" (vecbase));
    }

    /**
     *  get CCOUNT, this is esp32 special REG?
    */
static inline __attribute__((always_inline))
    unsigned __get_CCOUNT(void)
    {
        return __RSR(CCOUNT);
    }

    /**
     *  set CCOUNT
    */
static inline __attribute__((always_inline))
    void __set_CCOUNT(unsigned ccount)
    {
        __WSR(CCOUNT, ccount);
    }


static inline __attribute__((always_inline, pure))
    unsigned __get_CORE_ID(void)
    {
    #if XCHAL_HAVE_PRID
        unsigned id;
        asm volatile
        (
            "rsr.prid %0\n"
            "extui %0, %0, 13, 1" : "=r" (id)
        );
        return id;
    #else
        return 0;
    #endif
    }

static inline __attribute__((always_inline, pure))
    unsigned __get_RAW_CORE_ID(void)
    {
    #if XCHAL_HAVE_PRID
        // Read the raw value of special register PRID
        unsigned id;
        asm volatile ("rsr.prid %0" : "=r" (id));
        return id;
    #else
        return 0;
    #endif
    }

    /**
     *  waitfor interrupt
    */
static inline __attribute__((always_inline))
    void __WFI(void)
    {
        asm volatile ("waiti 0");
    }

    /**
     *  get interrupt processor status
    */
static inline __attribute__((always_inline))
    unsigned __get_IPSR(void)
    {
        return __RSR(PS) & PS_INTLEVEL_MASK;
    }

    /**
     *  get stack-top
    */
static inline __attribute__((always_inline))
    void *__get_SP(void)
    {
        void *sp;
        asm volatile ("mov %0, sp;" : "=r" (sp));
        return sp;
    }

/* The SET_STACK implements a setting a new stack pointer (sp or a1).
 * to do this the need reset PS_WOE, reset WINDOWSTART, update SP, and return PS_WOE.
 *
 * In addition, if a windowOverflow8/12 happens the exception handler expects to be able to look at
 * the previous frames stackpointer to find the extra save area. So this function will reserve space
 * for this area as well as initialise the previous sp that points to it
 *
 * Note: It has 2 implementations one for using in assembler files (*.S) and one for using in C.
 *
 * C code prototype for SET_STACK:
 *   uint32_t ps_reg;
 *   uint32_t w_base;
 *
 *   uint32_t sp = (uint32_t)new_sp - SAVE_AREA_OFFSET; \
    *(uint32_t*)(sp - 12) = (uint32_t)new_sp; \

 *   RSR(PS, ps_reg);
 *   ps_reg &= ~(PS_WOE_MASK | PS_OWB_MASK | PS_CALLINC_MASK);
 *   WSR(PS, ps_reg);
 *
 *   RSR(WINDOWBASE, w_base);
 *   WSR(WINDOWSTART, (1 << w_base));
 *
 *   asm volatile ( "movi sp, "XTSTR( (SOC_DRAM_LOW + (SOC_DRAM_HIGH - SOC_DRAM_LOW) / 2) )"");
 *
 *   RSR(PS, ps_reg);
 *   ps_reg |= (PS_WOE_MASK);
 *   WSR(PS, ps_reg);
*/
#ifdef __ASSEMBLER__
    .macro SET_STACK  new_sp tmp1 tmp2
        addi tmp1, new_sp, -SAVE_AREA_OFFSET
        addi tmp2, tmp1, -BASE_AREA_SP_OFFSET
        s32i new_sp, tmp2, 0
        addi new_sp, tmp1, 0
        rsr.ps \tmp1
        movi \tmp2, ~(PS_WOE_MASK | PS_OWB_MASK | PS_CALLINC_MASK)
        and \tmp1, \tmp1, \tmp2
        wsr.ps \tmp1
        rsync

        rsr.windowbase \tmp1
        ssl	\tmp1
        movi \tmp1, 1
        sll	\tmp1, \tmp1
        wsr.windowstart \tmp1
        rsync

        mov sp, \new_sp

        rsr.ps \tmp1
        movi \tmp2, (PS_WOE)
        or \tmp1, \tmp1, \tmp2
        wsr.ps \tmp1
        rsync
    .endm
#else
    #define SET_STACK(new_sp)           do { \
        uint32_t sp = (uint32_t)new_sp - SAVE_AREA_OFFSET; \
        *(uint32_t*)(sp - BASE_AREA_SP_OFFSET) = (uint32_t)new_sp; \
        const uint32_t mask = ~(PS_WOE_MASK | PS_OWB_MASK | PS_CALLINC_MASK); \
        uint32_t tmp1 = 0, tmp2 = 0; \
        asm volatile ( \
            "rsr.ps %1 \n"\
            "and %1, %1, %3 \n"\
            "wsr.ps %1 \n"\
            "rsync \n"\
            " \n"\
            "rsr.windowbase %1 \n"\
            "ssl	%1 \n"\
            "movi %1, 1 \n"\
            "sll	%1, %1 \n"\
            "wsr.windowstart %1 \n"\
            "rsync \n"\
            " \n"\
            "movi a0, 0\n" \
            "mov sp, %0 \n"\
            "rsr.ps %1 \n"\
            " \n"\
            "movi %2, " XTSTR( PS_WOE_MASK ) "\n"\
            " \n"\
            "or %1, %1, %2 \n"\
            "wsr.ps %1 \n"\
            "rsync \n"\
            : "+r"(sp), "+r"(tmp1), "+r"(tmp2) : "r"(mask)); \
    } while (0);
#endif // __ASSEMBLER__
