/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "soc.h"
#include "esp_attr.h"
#include "esp_err.h"

#include "uart.h"

#include "esp_private/panic_internal.h"

#pragma GCC diagnostic ignored "-Wconversion"

bool g_panic_abort = false;
static char *s_panic_abort_details = NULL;


void panic_print_char(char const c)
{
    UART_fifo_tx(&UART0, c);
}

void panic_print_str(char const *str)
{
    for (int i = 0; str[i] != 0; i++) {
        panic_print_char(str[i]);
    }
}

void panic_print_hex(int h)
{
    int x;
    int c;
    // Does not print '0x', only the digits (8 digits to print)
    for (x = 0; x < 8; x++) {
        c = (h >> 28) & 0xf; // extract the leftmost byte
        if (c < 10) {
            panic_print_char('0' + c);
        } else {
            panic_print_char('a' + c - 10);
        }
        h <<= 4; // move the 2nd leftmost byte to the left, to be extracted next
    }
}

void panic_print_dec(int d)
{
    // can print at most 2 digits!
    int n1, n2;
    n1 = d % 10; // extract ones digit
    n2 = d / 10; // extract tens digit
    if (n2 == 0) {
        panic_print_char(' ');
    } else {
        panic_print_char(n2 + '0');
    }
    panic_print_char(n1 + '0');
}

static void print_abort_details(void const *f)
{
    panic_print_str(s_panic_abort_details);
}

// Control arrives from chip-specific panic handler, environment prepared for
// the 'main' logic of panic handling. This means that chip-specific stuff have
// already been done, and panic_info_t has been filled.
void esp_panic_handler(panic_info_t *info)
{
    // If the exception was due to an abort, override some of the panic info
    if (g_panic_abort) {
        info->description = NULL;
        info->details = s_panic_abort_details ? print_abort_details : NULL;
        info->reason = NULL;
        info->exception = PANIC_EXCEPTION_ABORT;
    }

    /*
      * For any supported chip, the panic handler prints the contents of panic_info_t in the following format:
      *
      *
      * Guru Meditation Error: Core <core> (<exception>). <description>
      * <details>
      *
      * <state>
      *
      * <elf_info>
      *
      *
      * ----------------------------------------------------------------------------------------
      * core - core where exception was triggered
      * exception - what kind of exception occurred
      * description - a short description regarding the exception that occurred
      * details - more details about the exception
      * state - processor state like register contents, and backtrace
      * elf_info - details about the image currently running
      *
      * NULL fields in panic_info_t are not printed.
      *
      * */
    if (info->reason) {
        panic_print_str("Guru Meditation Error: Core ");
        panic_print_dec(info->core);
        panic_print_str(" panic'ed (");
        panic_print_str(info->reason);
        panic_print_str("). ");
    }

    if (info->description) {
        panic_print_str(info->description);
    }

    panic_print_str("\r\n");

    PANIC_INFO_DUMP(info, details);

    panic_print_str("\r\n");


    PANIC_INFO_DUMP(info, state);
    panic_print_str("\r\n");

    /* No matter if we come here from abort or an exception, this variable must be reset.
     * Else, any exception/error occurring during the current panic handler would considered
     * an abort. Do this after PANIC_INFO_DUMP(info, state) as it also checks this variable.
     * For example, if coredump triggers a stack overflow and this variable is not reset,
     * the second panic would be still be marked as the result of an abort, even the previous
     * message reason would be kept. */
    g_panic_abort = false;

    panic_print_str("\r\n");
}


void IRAM_ATTR __attribute__((noreturn, no_sanitize_undefined)) panic_abort(char const *details)
{
    g_panic_abort = true;
    s_panic_abort_details = (char *) details;

    *((volatile int *) 0) = 0; // NOLINT(clang-analyzer-core.NullDereference) should be an invalid operation on targets
    while (1);
}
