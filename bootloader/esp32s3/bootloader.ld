ENTRY(Reset_Handler);

/* alias */
PROVIDE(esp_log_printf = ets_printf);
PROVIDE (esp_rom_uart_set_as_console = uart_tx_switch);
/**
 *  Appendix: Memory Usage of ROM bootloader
 *
 *      0x3FCD7E00 ------------------> _dram0_0_start
 *                 |               |
 *                 |               |
 *                 |               |   1. Large buffers that are only used in certain boot modes, see shared_buffers.h
 *                 |               |
 *                 |               |
 *      0x3FCE9710 ------------------> __stack_sentry
 *                 |               |
 *                 |               |   2. Startup pro cpu stack (freed when IDF app is running)
 *                 |               |
 *      0x3FCEB710 ------------------> __stack (pro cpu)
 *                 |               |
 *                 |               |      Startup app cpu stack
 *                 |               |
 *      0x3FCED710 ------------------> __stack_app (app cpu)
 *                 |               |
 *                 |               |
 *                 |               |   3. Shared memory only used in startup code or nonos/early boot*
 *                 |               |      (can be freed when IDF runs)
 *                 |               |
 *                 |               |
 *      0x3FCEEE34 ------------------> _dram0_rtos_reserved_start
 *                 |               |
 *                 |               |
 *                 |               |   4. Shared memory used in startup code and when IDF runs
 *                 |               |
 *                 |               |
 *      0x3FCEF770 ------------------> _dram0_rtos_reserved_end
 *                 |               |
 *      0x3FCEF81C ------------------> _data_start_interface
 *                 |               |
 *                 |               |   5. End of DRAM is the 'interface' data with constant addresses (ECO compatible)
 *                 |               |
 *      0x3FCF0000 ------------------> _data_end_interface
 */
MEMORY
{
    BOOT_data_seg (RW) :        org = 0x3FCD7E00, len = 32K
    BOOT_text_seg (RWX) :       org = 0x403C7E00 + 32K, len = 32K
}

SECTIONS
{
    .data :
    {
        *(.gnu.linkonce.d.*)
        *(.data*)
        *(.gnu.linkonce.s.*)
        *(.gnu.linkonce.s2.*)
        *(.sdata*)
        *(.jcr)
    } > BOOT_data_seg

    .data.ro :
    {
    /* read only data */
        *(.gnu.version_r)
        *(.eh_frame)

        *(.gnu.linkonce.r.*)
        *(.rodata*)
        *(.sdata2*)

    /* C++ constructor and destructor tables, properly ordered: */
PROVIDE_HIDDEN(__init_array_start = ABSOLUTE(.));
        KEEP (*crtbegin.*(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.*) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
PROVIDE_HIDDEN(__init_array_end = ABSOLUTE(.));
        KEEP (*crtbegin.*(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.*) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    /* C++ exception handlers table:  */
        *(.gnu.version_d)
        *(.gnu.linkonce.e.*)
        *(.gnu.linkonce.h.*)
        *(.gcc_except_table)
        *(.xt_except_desc)
        *(.xt_except_table)
        *(.xt_except_desc_end)
        *(.dynamic)
    /* Literals are also RO data. */
        *(*.lit4*)
        *(.gnu.linkonce.lit4.*)

        . = ALIGN(4);
    } > BOOT_data_seg

    .data.zi (NOLOAD) :
    {
PROVIDE(__bss_start__ = ABSOLUTE(.));

        *(.dynsbss)
        *(.dynbss)
        *(.scommon)
        *(.gnu.linkonce.b.*)
        *(.bss*)
        *(.gnu.linkonce.sb.*)
        *(.gnu.linkonce.sb2.*)
        *(.sbss*)
        *(COMMON)

        . = ALIGN (8);
PROVIDE(__bss_end__ = ABSOLUTE(.));
PROVIDE(end = ABSOLUTE(.));
    } > BOOT_data_seg

    .text :
    {
        *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)

        *(.iram*)
        *(.literal* .text*)

        *(.fini.literal)
        *(.fini)
        *(.gnu.version)

        *(.entry.text)
        *(.init.literal)
        *(.init)
    } > BOOT_text_seg

    .xt.prop 0 :
    {
        KEEP (*(.xt.prop .gnu.linkonce.prop.*))
    }

    .xt.lit 0 :
    {
        KEEP (*(.xt.lit .gnu.linkonce.p.*))
    }
}
