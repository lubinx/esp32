
/*
 * Copyright (c) 2015-2019 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Xtensa-specific interrupt and exception functions for RTOS ports.
 * Also see xtensa_intr_asm.S.
 */

#include <stdlib.h>

#include <xtensa/config/core.h>
#include "xtensa/xtensa_api.h"
#include "xt_instr_macros.h"

#include "soc/soc_caps.h"

#if XCHAL_HAVE_EXCEPTIONS
    /* Handler table is in xtensa_intr_asm.S */
    extern xt_exc_handler _xt_exception_table[];

    /*
    Default handler for unhandled exceptions.
    CHANGED: We do this in panic.c now
    */

    //void xt_unhandled_exception(XtExcFrame *frame)
    //{
        //exit(-1);
    //}
    extern void xt_unhandled_exception(XtExcFrame *frame);

    /*
    This function registers a handler for the specified exception.
    The function returns the address of the previous handler.
    On error, it returns 0.
    */
    xt_exc_handler xt_set_exception_handler(unsigned int intr, xt_exc_handler f)
    {
        xt_exc_handler old;

        if (intr >= XCHAL_EXCCAUSE_NUM)
            return 0;

        /* Convert exception number to _xt_exception_table name */
        intr = SOC_CPU_CORES_NUM * intr + __get_CORE_ID();
        old = _xt_exception_table[intr];

        if (f)
            _xt_exception_table[intr] = f;
        else
            _xt_exception_table[intr] = &xt_unhandled_exception;

        return ((old == &xt_unhandled_exception) ? 0 : old);
    }
#endif

#if XCHAL_HAVE_INTERRUPTS
    /* Handler table is in xtensa_intr_asm.S */
    struct xt_handler_table_entry
    {
        void *handler;
        void *arg;
    };
    typedef struct xt_handler_table_entry   xt_handler_table_entry;

    extern xt_handler_table_entry _xt_interrupt_table[XCHAL_NUM_INTERRUPTS * SOC_CPU_CORES_NUM];

    /*
    Default handler for unhandled interrupts.
    */
    __attribute__((weak))
    void xt_unhandled_interrupt(void *arg)
    {
        (void)arg;
        // esp_rom_printf("Unhandled interrupt %d on cpu %d!\n", (int)arg, __get_CORE_ID());
    }

    //Returns true if handler for interrupt is not the default unhandled interrupt handler
    bool xt_int_has_handler(unsigned int intr, unsigned int cpu)
    {
        return (_xt_interrupt_table[intr * SOC_CPU_CORES_NUM + cpu].handler != xt_unhandled_interrupt);
    }

    /*
    This function registers a handler for the specified interrupt. The "arg"
    parameter specifies the argument to be passed to the handler when it is
    invoked. The function returns the address of the previous handler.
    On error, it returns 0.
    */
    xt_handler xt_set_interrupt_handler(unsigned int n, xt_handler f, void *arg)
    {
        xt_handler_table_entry * entry;
        xt_handler               old;

        if (n >= XCHAL_NUM_INTERRUPTS)
            return 0;       /* invalid interrupt number */
        if (Xthal_intlevel[n] > XCHAL_EXCM_LEVEL)
            return 0;       /* priority level too high to safely handle in C */

        /* Convert exception number to _xt_exception_table name */
        n = n * SOC_CPU_CORES_NUM + __get_CORE_ID();

        entry = _xt_interrupt_table + n;
        old   = entry->handler;

        if (f)
        {
            entry->handler = f;
            entry->arg     = arg;
        }
        else
        {
            entry->handler = &xt_unhandled_interrupt;
            entry->arg     = (void*)n;
        }

        return ((old == &xt_unhandled_interrupt) ? 0 : old);
    }
#endif
