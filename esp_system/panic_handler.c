/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "esp_compiler.h"
#include "esp_cpu.h"

#include "esp_ipc_isr.h"
#include "esp_private/system_internal.h"

#include "soc/soc_memory_layout.h"
#include "soc/soc_caps.h"
#include "soc/rtc.h"

#include "esp_private/cache_err_int.h"

#include "sdkconfig.h"
#include "esp_rom_sys.h"

#include "esp_private/panic_internal.h"
#include "esp_private/panic_reason.h"

extern int _invalid_pc_placeholder;

extern void esp_panic_handler(panic_info_t *);
void *g_exc_frames[SOC_CPU_CORES_NUM] = { NULL };

/*
  Panic handlers; these get called when an unhandled exception occurs or the assembly-level
  task switching / interrupt code runs into an unrecoverable error. The default task stack
  overflow handler and abort handler are also in here.
*/

/*
  Note: The linker script will put everything in this file in IRAM/DRAM, so it also works with flash cache disabled.
*/
static void print_state_for_core(const void *f, int core)
{
    /* On Xtensa (with Window ABI), register dump is not required for backtracing.
     * Don't print it on abort to reduce clutter.
     * On other architectures, register values need to be known for backtracing.
     */
#if (CONFIG_IDF_TARGET_ARCH_XTENSA && defined(XCHAL_HAVE_WINDOWED)) || \
    (CONFIG_IDF_TARGET_ARCH_RISCV && CONFIG_ESP_SYSTEM_USE_EH_FRAME)
    if (!g_panic_abort) {
#else
    if (true) {
#endif
        panic_print_registers(f, core);
        panic_print_str("\r\n");
    }
    panic_print_backtrace(f, core);
    }

static void print_state(const void *f)
{
    int err_core = f == g_exc_frames[0] ? 0 : 1;

    print_state_for_core(f, err_core);
    panic_print_str("\r\n");

    // If there are other frame info, print them as well
    for (int i = 0; i < SOC_CPU_CORES_NUM; i++)
    {
        // `f` is the frame for the offending core, see note above.
        if (err_core != i && g_exc_frames[i] != NULL) {
            print_state_for_core(g_exc_frames[i], i);
            panic_print_str("\r\n");
        }
    }
}

static void frame_to_panic_info(void *frame, panic_info_t * info, bool pseudo_excause)
{
    info->core = __get_CORE_ID();
    info->exception = PANIC_EXCEPTION_FAULT;
    info->details = NULL;
    info->reason = "Unknown";
    info->pseudo_excause = pseudo_excause;

    if (pseudo_excause) {
        panic_soc_fill_info(frame, info);
    }
    else {
        panic_arch_fill_info(frame, info);
    }

    info->state = print_state;
    info->frame = frame;
}

static void panic_handler(void *frame, bool pseudo_excause)
{
    panic_info_t info = { 0 };

    /*
     * Setup environment and perform necessary architecture/chip specific
     * steps here prior to the system panic handler.
     * */
    int core_id = __get_CORE_ID();

    // If multiple cores arrive at panic handler, save frames for all of them
    g_exc_frames[core_id] = frame;

    // These are cases where both CPUs both go into panic handler. The following code ensures
    // only one core proceeds to the system panic handler.
    if (pseudo_excause)
    {
        while (panic_get_cause(frame) == PANIC_RSN_INTWDT_CPU0 && core_id == 1 ||
            panic_get_cause(frame) == PANIC_RSN_INTWDT_CPU1 && core_id == 0
        );

        // For cache error, pause the non-offending core - offending core handles panic
        if (panic_get_cause(frame) == PANIC_RSN_CACHEERR && core_id != esp_cache_err_get_cpuid())
        {
            // Only print the backtrace for the offending core in case of the cache error
            g_exc_frames[core_id] = NULL;
            while (1);
        }
    }

    // Stall all other cores
    for (uint32_t i = 0; i < SOC_CPU_CORES_NUM; i++) {
        if (i != core_id) {
            esp_cpu_stall(i);
        }
    }

    esp_ipc_isr_stall_abort();

    if (__dbgr_is_attached())
    {
    #if __XTENSA__
        #define esp_cpu_pc_to_addr(pc)  ((void *)((pc & 0x3fffffffU) | 0x40000000U))

        if (!(esp_ptr_executable(esp_cpu_pc_to_addr(panic_get_address(frame))) && (panic_get_address(frame) & 0xC0000000U))) {
            /* Xtensa ABI sets the 2 MSBs of the PC according to the windowed call size
             * Incase the PC is invalid, GDB will fail to translate addresses to function names
             * Hence replacing the PC to a placeholder address in case of invalid PC
             */
            panic_set_address(frame, (uint32_t)&_invalid_pc_placeholder);
        }
    #endif
    }

    // Convert architecture exception frame into abstracted panic info
    frame_to_panic_info(frame, &info, pseudo_excause);

    // Call the system panic handler
    esp_panic_handler(&info);
}

void IRAM_ATTR panicHandler(void *frame)
{
    // This panic handler gets called for when the double exception vector,
    // kernel exception vector gets used; as well as handling interrupt-based
    // faults cache error, wdt expiry. EXCAUSE register gets written with
    // one of PANIC_RSN_* values.
    panic_handler(frame, true);
}

void IRAM_ATTR xt_unhandled_exception(void *frame)
{
    panic_handler(frame, false);
}

void __attribute__((noreturn)) panic_restart(void)
{
    bool digital_reset_needed = false;

#ifdef CONFIG_IDF_TARGET_ESP32
    // On the ESP32, cache error status can only be cleared by system reset
    if (esp_cache_err_get_cpuid() != -1) {
        digital_reset_needed = true;
    }
#endif

    if (digital_reset_needed)
        esp_restart_noos_dig();
    else
        esp_restart_noos();
}
