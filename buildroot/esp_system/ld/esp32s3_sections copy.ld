/* bootloader_support/bootloader_start.c */
ENTRY(bootloader_startup)
/* ENTRY(call_start_cpu0); */

SECTIONS
{
    .text :
    {
PROVIDE(_iram_text_start = ABSOLUTE(.));
        *(.UserEnter.literal);
        *(.UserEnter.text);

        . = ALIGN(16);
        *(.entry.text)
        *(.init.literal)
        *(.init)

        mapping[iram0_text]
    } > SRAM_text_seg

    .flash.text :
    {
PROVIDE(_stext = ABSOLUTE(.));
PROVIDE(_instruction_reserved_start = ABSOLUTE(.)); /* This is a symbol marking the flash.text start, this can be used for mmu driver to maintain virtual address */

        mapping[flash_text]

        *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
        *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
        *(.fini.literal)
        *(.fini)
        *(.gnu.version)

        /** CPU will try to prefetch up to 16 bytes of
         * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
         * safe access to up to 16 bytes after the last real instruction, add
         * dummy bytes to ensure this
        */
        . += 15;
        . = ALIGN(16);
/* This is a symbol marking the flash.text end, this can be used for mmu driver to maintain virtual address */
PROVIDE(_instruction_reserved_end = ABSOLUTE(.));
PROVIDE(_etext = ABSOLUTE(.));
    } > default_code_seg

    .flash.rodata_dummy (NOLOAD):
    {
        _flash_rodata_dummy_start = ABSOLUTE(.);
        /* Create an empty gap as big as .flash.text section */
        . = ABSOLUTE(.) + SIZEOF(.flash.text);
        . = ALIGN(256);
/* This is a symbol marking the flash.rodata start, this can be used for mmu driver to maintain virtual address */
PROVIDE(_rodata_reserved_start = ABSOLUTE(.));
    } > default_rodata_seg

    .flash.rodata : ALIGN(0x10)
    {
PROVIDE(_rodata_start = ABSOLUTE(.));

        *(.rodata_desc .rodata_desc.*)               /* Should be the first.  App version info.        DO NOT PUT ANYTHING BEFORE IT! */
        *(.rodata_custom_desc .rodata_custom_desc.*) /* Should be the second. Custom app version info. DO NOT PUT ANYTHING BEFORE IT! */

        /* Create an empty gap within this section. Thanks to this, the end of this
         * section will match .flah.rodata's begin address. Thus, both sections
         * will be merged when creating the final bin image.
        */
        . = ALIGN(ALIGNOF(.flash.rodata));
PROVIDE(_flash_rodata_start = ABSOLUTE(.));
        mapping[rodata_noload]
        mapping[flash_rodata]

        *(.irom1.text) /* catch stray ICACHE_RODATA_ATTR */
        *(.gnu.linkonce.r.*)
        *(.rodata1)

        /* __XT_EXCEPTION_TABLE_ = ABSOLUTE(.); */
        *(.xt_except_table)
        *(.gcc_except_table .gcc_except_table.*)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)

        . = ALIGN(4);   /* WTF? . = (. + 3) & ~ 3; */
        __eh_frame = ABSOLUTE(.);
        KEEP(*(.eh_frame))
        . = ALIGN(8);   /* . = (. + 7) & ~ 3; */

        /* C++ constructor and destructor tables */
        /* Don't include anything from crtbegin.o or crtend.o, as IDF doesn't use toolchain crt */
        __init_array_start = ABSOLUTE(.);
        KEEP (*(EXCLUDE_FILE (*crtend.* *crtbegin.*) .ctors SORT(.ctors.*)))
        __init_array_end = ABSOLUTE(.);
        KEEP (*crtbegin.*(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.*) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))

        /* C++ exception handlers table: */
        /* __XT_EXCEPTION_DESCS_ = ABSOLUTE(.); */
        *(.xt_except_desc)
        *(.gnu.linkonce.h.*)
        /* __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.); */
        *(.xt_except_desc_end)
        *(.dynamic)
        *(.gnu.version_d)

        /* Addresses of memory regions reserved via SOC_RESERVE_MEMORY_REGION() */
        soc_reserved_memory_region_start = ABSOLUTE(.);
        KEEP (*(.reserved_memory_address))
        soc_reserved_memory_region_end = ABSOLUTE(.);

        /* System init functions registered via ESP_SYSTEM_INIT_FN */
        _esp_system_init_fn_array_start = ABSOLUTE(.);
        KEEP (*(SORT_BY_INIT_PRIORITY(.esp_system_init_fn.*)))
        _esp_system_init_fn_array_end = ABSOLUTE(.);
PROVIDE(_rodata_end = ABSOLUTE(.));

        /* Literals are also RO data. */
        _lit4_start = ABSOLUTE(.);
        *(*.lit4)
        *(.lit4.*)
        *(.gnu.linkonce.lit4.*)
        _lit4_end = ABSOLUTE(.);

        . = ALIGN(4);
PROVIDE(_thread_local_start = ABSOLUTE(.));
        *(.tdata)
        *(.tdata.*)
        *(.tbss)
        *(.tbss.*)

        . = ALIGN(4);
PROVIDE(_thread_local_end = ABSOLUTE(.));
/* This is a symbol marking the flash.rodata end, this can be used for mmu driver to maintain virtual address */
PROVIDE(_rodata_reserved_end = ABSOLUTE(.));
    } > default_rodata_seg

PROVIDE(_flash_rodata_align = ALIGNOF(.flash.rodata));


}
