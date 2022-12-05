ENTRY(bootloader_startup);

/**
 *  Appendix: Memory Usage of ROM bootloader
 *
 * 0x3fcd7e00 ------------------> _dram0_0_start
 *            |               |
 *            |               |
 *            |               |   1. Large buffers that are only used in certain boot modes, see shared_buffers.h
 *            |               |
 *            |               |
 * 0x3fce9710 ------------------> __stack_sentry
 *            |               |
 *            |               |   2. Startup pro cpu stack (freed when IDF app is running)
 *            |               |
 * 0x3fceb710 ------------------> __stack (pro cpu)
 *            |               |
 *            |               |      Startup app cpu stack
 *            |               |
 * 0x3fced710 ------------------> __stack_app (app cpu)
 *            |               |
 *            |               |
 *            |               |   3. Shared memory only used in startup code or nonos/early boot*
 *            |               |      (can be freed when IDF runs)
 *            |               |
 *            |               |
 * 0x3fceee34 ------------------> _dram0_rtos_reserved_start
 *            |               |
 *            |               |
 *            |               |   4. Shared memory used in startup code and when IDF runs
 *            |               |
 *            |               |
 * 0x3fcef770 ------------------> _dram0_rtos_reserved_end
 *            |               |
 * 0x3fcef81c ------------------> _data_start_interface
 *            |               |
 *            |               |   5. End of DRAM is the 'interface' data with constant addresses (ECO compatible)
 *            |               |
 * 0x3fcf0000 ------------------> _data_end_interface
 */
MEMORY
{
    BOOT_text_seg (RWX) :       org = 0x403C7E00 + 32K, len = 32K
    BOOT_data_seg (RW) :        org = 0x3FCD7E00, len = 32K
}

SECTIONS
{
    .data :
    {
_dram_start = ABSOLUTE(.);
_data_start = ABSOLUTE(.);
        *(.gnu.linkonce.d.*)
        *(.data*)
        *(.gnu.linkonce.s.*)
        *(.gnu.linkonce.s2.*)
        *(.sdata*)
        *(.jcr)

    /* read only data */
        *(.gnu.linkonce.r.*)
        *(.rodata*)
        *(.sdata2*)

        __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
        *(.xt_except_table)
        *(.gcc_except_table)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)
        *(.eh_frame)

        /* C++ constructor and destructor tables, properly ordered: */
        __init_array_start = ABSOLUTE(.);
        KEEP (*crtbegin.*(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.*) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        __init_array_end = ABSOLUTE(.);
        KEEP (*crtbegin.*(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.*) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        /*  C++ exception handlers table:  */
        __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
        *(.xt_except_desc)
        *(.gnu.linkonce.h.*)
        __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
        *(.xt_except_desc_end)
        *(.dynamic)
        *(.gnu.version_d)
        _rodata_end = ABSOLUTE(.);
        /* Literals are also RO data. */
        _lit4_start = ABSOLUTE(.);
        *(*.lit4)
        *(.lit4.*)
        *(.gnu.linkonce.lit4.*)
        _lit4_end = ABSOLUTE(.);

        . = ALIGN(4);
_data_end = ABSOLUTE(.);
    } > BOOT_data_seg

    .data.zi (NOLOAD) :
    {
_bss_start = ABSOLUTE(.);

        *(.dynsbss)
        *(.dynbss)
        *(.gnu.linkonce.b.*)
        *(.bss*)

        *(.gnu.linkonce.sb.*)
        *(.sbss*)
        *(.gnu.linkonce.sb2.*)
        *(.sbss2*)
        *(.scommon)
        *(COMMON)

        . = ALIGN (8);
_bss_end = ABSOLUTE(.);
_dram_end = ABSOLUTE(.);
    } > BOOT_data_seg

    .text :
    {
_loader_text_start = ABSOLUTE(.);
        *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)

        *(.iram*)
        *(.literal* .text*)

        *(.fini.literal)
        *(.fini)
        *(.gnu.version)

        *(.entry.text)
        *(.init.literal)
        *(.init)

_loader_text_end = ABSOLUTE(.);
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
