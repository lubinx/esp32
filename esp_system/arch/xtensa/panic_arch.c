#include <xtensa/xtensa_context.h>

#include "soc.h"
#include "esp_debug_helpers.h"
#include "esp_private/panic_internal.h"
#include "esp_private/panic_reason.h"

#include "sdkconfig.h"

#if ! CONFIG_IDF_TARGET_ESP32
    #include "soc/extmem_reg.h"
    #include "soc/ext_mem_defs.h"
    #include "soc/rtc_cntl_reg.h"
#endif

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"

void panic_print_registers(void const *f, int core)
{
    XtExcFrame *frame = (XtExcFrame *)f;
    int *regs = (int *)frame;
    (void)regs;

    char const *sdesc[] = {
        "PC      ", "PS      ", "A0      ", "A1      ", "A2      ", "A3      ", "A4      ", "A5      ",
        "A6      ", "A7      ", "A8      ", "A9      ", "A10     ", "A11     ", "A12     ", "A13     ",
        "A14     ", "A15     ", "SAR     ", "EXCCAUSE", "EXCVADDR", "LBEG    ", "LEND    ", "LCOUNT  "
    };

    /* only dump registers for 'real' crashes, if crashing via abort()
       the register window is no longer useful.
    */
    panic_print_str("Core ");
    panic_print_dec(core);
    panic_print_str(" register dump:");

    for (int x = 0; x < 24; x += 4) {
        panic_print_str("\r\n");
        for (int y = 0; y < 4; y++) {
            if (sdesc[x + y][0] != 0) {
                panic_print_str(sdesc[x + y]);
                panic_print_str(": 0x");
                panic_print_hex(regs[x + y + 1]);
                panic_print_str("  ");
            }
        }
    }

    // If the core which triggers the interrupt watchpoint was in ISR context, dump the epc registers.
    if ((core == 0 && frame->exccause == PANIC_RSN_INTWDT_CPU0) || (core == 1 && frame->exccause == PANIC_RSN_INTWDT_CPU1))
    {
        panic_print_str("\r\n");

        uint32_t __value;
        panic_print_str("Core ");
        panic_print_dec(core);
        panic_print_str(" was running in ISR context:\r\n");

        __asm__("rsr.epc1 %0" : "=a"(__value));
        panic_print_str("EPC1    : 0x");
        panic_print_hex(__value);

        __asm__("rsr.epc2 %0" : "=a"(__value));
        panic_print_str("  EPC2    : 0x");
        panic_print_hex(__value);

        __asm__("rsr.epc3 %0" : "=a"(__value));
        panic_print_str("  EPC3    : 0x");
        panic_print_hex(__value);

        __asm__("rsr.epc4 %0" : "=a"(__value));
        panic_print_str("  EPC4    : 0x");
        panic_print_hex(__value);
    }
}

static void print_illegal_instruction_details(void const *f)
{
    XtExcFrame *frame = (XtExcFrame *)f;
    /* Print out memory around the instruction word */
    uint32_t epc = frame->pc;
    epc = (epc & ~0x3) - 4;

    /* check that the address was sane */
    if (epc < SOC_IROM_MASK_LOW || epc >= SOC_IROM_HIGH) {
        return;
    }
    volatile uint32_t *pepc = (uint32_t *)epc;
    (void)pepc;

    panic_print_str("Memory dump at 0x");
    panic_print_hex(epc);
    panic_print_str(": ");

    panic_print_hex(*pepc);
    panic_print_str(" ");
    panic_print_hex(*(pepc + 1));
    panic_print_str(" ");
    panic_print_hex(*(pepc + 2));
}


static void print_debug_exception_details(void const *f)
{
    int debug_rsn;
    asm("rsr.debugcause %0":"=r"(debug_rsn));
    panic_print_str("Debug exception reason: ");
    if (debug_rsn & XCHAL_DEBUGCAUSE_ICOUNT_MASK) {
        panic_print_str("SingleStep ");
    }
    if (debug_rsn & XCHAL_DEBUGCAUSE_IBREAK_MASK) {
        panic_print_str("HwBreakpoint ");
    }
    if (debug_rsn & XCHAL_DEBUGCAUSE_DBREAK_MASK) {
        //Unlike what the ISA manual says, this core seemingly distinguishes from a DBREAK
        //reason caused by watchdog 0 and one caused by watchdog 1 by setting bit 8 of the
        //debugcause if the cause is watchpoint 1 and clearing it if it's watchpoint 0.
        if (debug_rsn & (1 << 8))
            panic_print_str("Watchpoint 1 triggered ");
        else
            panic_print_str("Watchpoint 0 triggered ");
    }
    if (debug_rsn & XCHAL_DEBUGCAUSE_BREAK_MASK) {
        panic_print_str("BREAK instr ");
    }
    if (debug_rsn & XCHAL_DEBUGCAUSE_BREAKN_MASK) {
        panic_print_str("BREAKN instr ");
    }
    if (debug_rsn & XCHAL_DEBUGCAUSE_DEBUGINT_MASK) {
        panic_print_str("DebugIntr ");
    }
}

#if CONFIG_IDF_TARGET_ESP32S2
    static inline void print_cache_err_details(void const *f)
    {
        uint32_t vaddr = 0, size = 0;
        uint32_t status[2];
        status[0] = REG_READ(EXTMEM_CACHE_DBG_STATUS0_REG);
        status[1] = REG_READ(EXTMEM_CACHE_DBG_STATUS1_REG);
        for (int i = 0; i < 32; i++) {
            switch (status[0] & BIT(i)) {
            case EXTMEM_IC_SYNC_SIZE_FAULT_ST:
                vaddr = REG_READ(EXTMEM_PRO_ICACHE_MEM_SYNC0_REG);
                size = REG_READ(EXTMEM_PRO_ICACHE_MEM_SYNC1_REG);
                panic_print_str("Icache sync parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_IC_PRELOAD_SIZE_FAULT_ST:
                vaddr = REG_READ(EXTMEM_PRO_ICACHE_PRELOAD_ADDR_REG);
                size = REG_READ(EXTMEM_PRO_ICACHE_PRELOAD_SIZE_REG);
                panic_print_str("Icache preload parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_ICACHE_REJECT_ST:
                vaddr = REG_READ(EXTMEM_PRO_ICACHE_REJECT_VADDR_REG);
                panic_print_str("Icache reject error occurred while accessing the address 0x");
                panic_print_hex(vaddr);

                if (REG_READ(EXTMEM_PRO_CACHE_MMU_FAULT_CONTENT_REG) & MMU_INVALID) {
                    panic_print_str(" (invalid mmu entry)");
                }
                panic_print_str("\r\n");
                break;
            default:
                break;
            }
            switch (status[1] & BIT(i)) {
            case EXTMEM_DC_SYNC_SIZE_FAULT_ST:
                vaddr = REG_READ(EXTMEM_PRO_DCACHE_MEM_SYNC0_REG);
                size = REG_READ(EXTMEM_PRO_DCACHE_MEM_SYNC1_REG);
                panic_print_str("Dcache sync parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_DC_PRELOAD_SIZE_FAULT_ST:
                vaddr = REG_READ(EXTMEM_PRO_DCACHE_PRELOAD_ADDR_REG);
                size = REG_READ(EXTMEM_PRO_DCACHE_PRELOAD_SIZE_REG);
                panic_print_str("Dcache preload parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_DCACHE_WRITE_FLASH_ST:
                panic_print_str("Write back error occurred while dcache tries to write back to flash\r\n");
                break;
            case EXTMEM_DCACHE_REJECT_ST:
                vaddr = REG_READ(EXTMEM_PRO_DCACHE_REJECT_VADDR_REG);
                panic_print_str("Dcache reject error occurred while accessing the address 0x");
                panic_print_hex(vaddr);

                if (REG_READ(EXTMEM_PRO_CACHE_MMU_FAULT_CONTENT_REG) & MMU_INVALID) {
                    panic_print_str(" (invalid mmu entry)");
                }
                panic_print_str("\r\n");
                break;
            case EXTMEM_MMU_ENTRY_FAULT_ST:
                vaddr = REG_READ(EXTMEM_PRO_CACHE_MMU_FAULT_VADDR_REG);
                panic_print_str("MMU entry fault error occurred while accessing the address 0x");
                panic_print_hex(vaddr);

                if (REG_READ(EXTMEM_PRO_CACHE_MMU_FAULT_CONTENT_REG) & MMU_INVALID) {
                    panic_print_str(" (invalid mmu entry)");
                }
                panic_print_str("\r\n");
                break;
            default:
                break;
            }
        }
    }
#elif CONFIG_IDF_TARGET_ESP32S3
    static inline void print_cache_err_details(void const *f)
    {
        uint32_t vaddr = 0, size = 0;
        uint32_t status;
        status = REG_READ(EXTMEM_CACHE_ILG_INT_ST_REG);
        for (int i = 0; i < 32; i++) {
            switch (status & BIT(i)) {
            case EXTMEM_ICACHE_SYNC_OP_FAULT_ST:
                //TODO, which size should fetch
                //vaddr = REG_READ(EXTMEM_ICACHE_MEM_SYNC0_REG);
                //size = REG_READ(EXTMEM_ICACHE_MEM_SYNC1_REG);
                panic_print_str("Icache sync parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_ICACHE_PRELOAD_OP_FAULT_ST:
                //TODO, which size should fetch
                vaddr = REG_READ(EXTMEM_ICACHE_PRELOAD_ADDR_REG);
                size = REG_READ(EXTMEM_ICACHE_PRELOAD_SIZE_REG);
                panic_print_str("Icache preload parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_DCACHE_SYNC_OP_FAULT_ST:
                //TODO, which size should fetch
                //vaddr = REG_READ(EXTMEM_DCACHE_MEM_SYNC0_REG);
                //size = REG_READ(EXTMEM_DCACHE_MEM_SYNC1_REG);
                panic_print_str("Dcache sync parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_DCACHE_PRELOAD_OP_FAULT_ST:
                //TODO, which size should fetch
                vaddr = REG_READ(EXTMEM_DCACHE_PRELOAD_ADDR_REG);
                size = REG_READ(EXTMEM_DCACHE_PRELOAD_SIZE_REG);
                panic_print_str("Dcache preload parameter configuration error, the error address and size is 0x");
                panic_print_hex(vaddr);
                panic_print_str("(0x");
                panic_print_hex(size);
                panic_print_str(")\r\n");
                break;
            case EXTMEM_DCACHE_WRITE_FLASH_ST:
                panic_print_str("Write back error occurred while dcache tries to write back to flash\r\n");
                break;
            case EXTMEM_MMU_ENTRY_FAULT_ST:
                vaddr = REG_READ(EXTMEM_CACHE_MMU_FAULT_VADDR_REG);
                panic_print_str("MMU entry fault error occurred while accessing the address 0x");
                panic_print_hex(vaddr);

                if (REG_READ(EXTMEM_CACHE_MMU_FAULT_CONTENT_REG) & MMU_INVALID) {
                    panic_print_str(" (invalid mmu entry)");
                }
                panic_print_str("\r\n");
                break;
            default:
                break;
            }
        }
        panic_print_str("\r\n");
    }
#endif


void panic_arch_fill_info(void *f, panic_info_t *info)
{
    XtExcFrame *frame = (XtExcFrame *)f;
    static char const *reason[] = {
        "IllegalInstruction", "Syscall", "InstructionFetchError", "LoadStoreError",
        "Level1Interrupt", "Alloca", "IntegerDivideByZero", "PCValue",
        "Privileged", "LoadStoreAlignment", "res", "res",
        "InstrPDAddrError", "LoadStorePIFDataError", "InstrPIFAddrError", "LoadStorePIFAddrError",
        "InstTLBMiss", "InstTLBMultiHit", "InstFetchPrivilege", "res",
        "InstrFetchProhibited", "res", "res", "res",
        "LoadStoreTLBMiss", "LoadStoreTLBMultihit", "LoadStorePrivilege", "res",
        "LoadProhibited", "StoreProhibited", "res", "res",
        "Cp0Dis", "Cp1Dis", "Cp2Dis", "Cp3Dis",
        "Cp4Dis", "Cp5Dis", "Cp6Dis", "Cp7Dis"
    };

    if (frame->exccause < (long)(sizeof(reason) / sizeof(char *))) {
        info->reason = (reason[frame->exccause]);
    }
    else {
        info->reason = "Unknown";
    }

    info->description = "Exception was unhandled.";

    if (frame->exccause == EXCCAUSE_ILLEGAL) {
        info->details = print_illegal_instruction_details;
    }

    info->addr = ((void *)((XtExcFrame *)frame)->pc);
}

void panic_soc_fill_info(void *f, panic_info_t *info)
{
    // [refactor-todo] this should be in the common port panic_handler.c, once
    // these special exceptions are supported in there.
    XtExcFrame *frame = (XtExcFrame *)f;
    if (frame->exccause == PANIC_RSN_INTWDT_CPU0)
    {
        info->core = 0;
        info->exception = PANIC_EXCEPTION_IWDT;
    }
    else if (frame->exccause == PANIC_RSN_INTWDT_CPU1)
    {
        info->core = 1;
        info->exception = PANIC_EXCEPTION_IWDT;
    }
    else if (frame->exccause == PANIC_RSN_CACHEERR)
    {
        info->core = __cache_err_core_id();
    }
    else {}

    //Please keep in sync with PANIC_RSN_* defines
    static char const *pseudo_reason[] =
    {
        "Unknown reason",
        "Unhandled debug exception",
        "Double exception",
        "Unhandled kernel exception",
        "Coprocessor exception",
        "Interrupt wdt timeout on CPU0",
        "Interrupt wdt timeout on CPU1",
        "Cache disabled but cached memory region accessed",
    };

    info->reason = pseudo_reason[0];
    info->description = NULL;

    if (frame->exccause <= PANIC_RSN_MAX)
        info->reason = pseudo_reason[frame->exccause];

    if (frame->exccause == PANIC_RSN_DEBUGEXCEPTION)
    {
        info->details = print_debug_exception_details;
        info->exception = PANIC_EXCEPTION_DEBUG;
    }

    //MV note: ESP32S3 PMS handling?
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    if (frame->exccause == PANIC_RSN_CACHEERR)
        info->details = print_cache_err_details;
#endif
}

uint32_t panic_get_address(void const *f)
{
    return ((XtExcFrame *)f)->pc;
}

uint32_t panic_get_cause(void const *f)
{
    return ((XtExcFrame *)f)->exccause;
}

void panic_set_address(void *f, uint32_t addr)
{
    ((XtExcFrame *)f)->pc = addr;
}

void panic_print_backtrace(void const *f, int core)
{
    XtExcFrame *xt_frame = (XtExcFrame *)f;
    esp_backtrace_frame_t frame = { .pc = xt_frame->pc, .sp = xt_frame->a1, .next_pc = xt_frame->a0, .exc_frame = xt_frame };
    esp_backtrace_print_from_frame(100, &frame, true);
}
