/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_compiler.h"

#include "esp_private/system_internal.h"

#include "esp_cpu.h"
#include "soc/rtc.h"
#include "hal/timer_hal.h"

#include "esp_private/panic_internal.h"
#include "port/panic_funcs.h"
#include "esp_rom_sys.h"

#include "sdkconfig.h"

#if CONFIG_ESP_COREDUMP_ENABLE
#include "esp_core_dump.h"
#endif

#if CONFIG_APPTRACE_ENABLE
#include "esp_app_trace.h"
#if CONFIG_APPTRACE_SV_ENABLE
#include "SEGGER_RTT.h"
#endif

#if CONFIG_APPTRACE_ONPANIC_HOST_FLUSH_TMO == -1
#define APPTRACE_ONPANIC_HOST_FLUSH_TMO   ESP_APPTRACE_TMO_INFINITE
#else
#define APPTRACE_ONPANIC_HOST_FLUSH_TMO   (1000*CONFIG_APPTRACE_ONPANIC_HOST_FLUSH_TMO)
#endif
#endif // CONFIG_APPTRACE_ENABLE

#if !CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT
#include "hal/uart_hal.h"
#endif

#if CONFIG_ESP_SYSTEM_PANIC_GDBSTUB
#include "esp_gdbstub.h"
#endif

bool g_panic_abort = false;
static char *s_panic_abort_details = NULL;

#if !CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT

static uart_hal_context_t s_panic_uart = { .dev = &UART0 };

static void panic_print_char_uart(char const c)
{
    uint32_t sz = 0;
    while (!uart_hal_get_txfifo_len(&s_panic_uart));
    uart_hal_write_txfifo(&s_panic_uart, (uint8_t *) &c, 1, &sz);
}


void panic_print_char(char const c)
{
    panic_print_char_uart(c);
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
#endif  // CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT

static void print_abort_details(const void *f)
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

#if CONFIG_APPTRACE_ENABLE
    // disable_all_wdts();
#if CONFIG_APPTRACE_SV_ENABLE
    SEGGER_RTT_ESP_FlushNoLock(CONFIG_APPTRACE_POSTMORTEM_FLUSH_THRESH, APPTRACE_ONPANIC_HOST_FLUSH_TMO);
#else
    esp_apptrace_flush_nolock(ESP_APPTRACE_DEST_TRAX, CONFIG_APPTRACE_POSTMORTEM_FLUSH_THRESH,
                              APPTRACE_ONPANIC_HOST_FLUSH_TMO);
#endif

#endif // CONFIG_APPTRACE_ENABLE

#if CONFIG_ESP_SYSTEM_PANIC_GDBSTUB
    panic_print_str("Entering gdb stub now.\r\n");
    esp_gdbstub_panic_handler((void *)info->frame);
#else
#if CONFIG_ESP_COREDUMP_ENABLE
    static bool s_dumping_core;
    if (s_dumping_core)
    {
        panic_print_str("Re-entered core dump! Exception happened during core dump!\r\n");
    }
    else
    {
        s_dumping_core = true;
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
        esp_core_dump_to_flash(info);
#endif
#if CONFIG_ESP_COREDUMP_ENABLE_TO_UART && !CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT
        esp_core_dump_to_uart(info);
#endif
        s_dumping_core = false;
    }
#endif /* CONFIG_ESP_COREDUMP_ENABLE */

#if CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT || CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT

    if (esp_reset_reason_get_hint() == ESP_RST_UNKNOWN) {
        switch (info->exception) {
        case PANIC_EXCEPTION_IWDT:
            esp_reset_reason_set_hint(ESP_RST_INT_WDT);
            break;
        case PANIC_EXCEPTION_TWDT:
            esp_reset_reason_set_hint(ESP_RST_TASK_WDT);
            break;
        case PANIC_EXCEPTION_ABORT:
        case PANIC_EXCEPTION_FAULT:
        default:
            esp_reset_reason_set_hint(ESP_RST_PANIC);
            break; // do not touch the previously set reset reason hint
        }
    }

    panic_print_str("Rebooting...\r\n");
    panic_restart();
#else /* CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT || CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT */
    panic_print_str("CPU halted.\r\n");
    while (1);
#endif /* CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT || CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT */
#endif /* CONFIG_ESP_SYSTEM_PANIC_GDBSTUB */
}


void IRAM_ATTR __attribute__((noreturn, no_sanitize_undefined)) panic_abort(char const *details)
{
    g_panic_abort = true;
    s_panic_abort_details = (char *) details;

#if CONFIG_APPTRACE_ENABLE
#if CONFIG_APPTRACE_SV_ENABLE
    SEGGER_RTT_ESP_FlushNoLock(CONFIG_APPTRACE_POSTMORTEM_FLUSH_THRESH, APPTRACE_ONPANIC_HOST_FLUSH_TMO);
#else
    esp_apptrace_flush_nolock(ESP_APPTRACE_DEST_TRAX, CONFIG_APPTRACE_POSTMORTEM_FLUSH_THRESH,
                              APPTRACE_ONPANIC_HOST_FLUSH_TMO);
#endif
#endif

    *((volatile int *) 0) = 0; // NOLINT(clang-analyzer-core.NullDereference) should be an invalid operation on targets
    while (1);
}

/* Weak versions of reset reason hint functions.
 * If these weren't provided, reset reason code would be linked into the app
 * even if the app never called esp_reset_reason().
 */
void IRAM_ATTR __attribute__((weak)) esp_reset_reason_set_hint(esp_reset_reason_t hint)
{
}

esp_reset_reason_t IRAM_ATTR  __attribute__((weak)) esp_reset_reason_get_hint(void)
{
    return ESP_RST_UNKNOWN;
}
