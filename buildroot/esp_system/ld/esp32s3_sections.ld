/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Default entry point */
ENTRY(call_start_cpu0);

_diram_i_start = 0x40378000;

SECTIONS
{
    /* Send .iram0 code to iram */
    .iram0.vectors :
    {
        _iram_start = ABSOLUTE(.);
        /* Vectors go to IRAM */
        _vector_table = ABSOLUTE(.);
        . = 0x0;
        KEEP(*(.WindowVectors.text));
        . = 0x180;
        KEEP(*(.Level2InterruptVector.text));
        . = 0x1c0;
        KEEP(*(.Level3InterruptVector.text));
        . = 0x200;
        KEEP(*(.Level4InterruptVector.text));
        . = 0x240;
        KEEP(*(.Level5InterruptVector.text));
        . = 0x280;
        KEEP(*(.DebugExceptionVector.text));
        . = 0x2c0;
        KEEP(*(.NMIExceptionVector.text));
        . = 0x300;
        KEEP(*(.KernelExceptionVector.text));
        . = 0x340;
        KEEP(*(.UserExceptionVector.text));
        . = 0x3C0;
        KEEP(*(.DoubleExceptionVector.text));
        . = 0x400;
        _invalid_pc_placeholder = ABSOLUTE(.);
        *(.*Vector.literal)

        *(.UserEnter.literal);
        *(.UserEnter.text);
        . = ALIGN (16);
        *(.entry.text)
        *(.init.literal)
        *(.init)
        _init_end = ABSOLUTE(.);
    } > SRAM_text_seg

    .iram0.text :
    {
        /* Code marked as running out of IRAM */
        _iram_text_start = ABSOLUTE(.);

        mapping[iram0_text]

    } > SRAM_text_seg

    /**
    * This section is required to skip .iram0.text area because SRAM_text_seg and
    * SRAM_data_seg reflect the same address space on different buses.
    */
    .dram0.dummy (NOLOAD):
    {
        . = ORIGIN(SRAM_data_seg) + MAX(_iram_end - _diram_i_start, 0);
    } > SRAM_data_seg

    .dram0.data :
    {
        _data_start = ABSOLUTE(.);
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(.gnu.linkonce.s2.*)
        *(.jcr)

        mapping[dram0_data]

        _data_end = ABSOLUTE(.);
        . = ALIGN(4);
    } > SRAM_data_seg

    /**
    * This section holds data that should not be initialized at power up.
    * The section located in Internal SRAM memory region. The macro _NOINIT
    * can be used as attribute to place data into this section.
    * See the "esp_attr.h" file for more information.
    */
    .noinit (NOLOAD):
    {
        . = ALIGN(4);
        _noinit_start = ABSOLUTE(.);
        *(.noinit .noinit.*)
        . = ALIGN(4) ;
        _noinit_end = ABSOLUTE(.);
    } > SRAM_data_seg

    /* Shared RAM */
    .dram0.bss (NOLOAD) :
    {
        . = ALIGN (8);
        _bss_start = ABSOLUTE(.);

        mapping[dram0_bss]

        *(.dynsbss)
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
        *(.sbss2)
        *(.sbss2.*)
        *(.gnu.linkonce.sb2.*)
        *(.dynbss)
        *(.share.mem)
        *(.gnu.linkonce.b.*)

        . = ALIGN (8);
        _bss_end = ABSOLUTE(.);
    } > SRAM_data_seg

    ASSERT(((_bss_end - ORIGIN(SRAM_data_seg)) <= LENGTH(SRAM_data_seg)), "DRAM segment data does not fit.")

    .flash.text :
    {
        _stext = .;
        _instruction_reserved_start = ABSOLUTE(.);  /* This is a symbol marking the flash.text start, this can be used for mmu driver to maintain virtual address */
        _text_start = ABSOLUTE(.);

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
        . += 16;

        _text_end = ABSOLUTE(.);
        _instruction_reserved_end = ABSOLUTE(.);  /* This is a symbol marking the flash.text end, this can be used for mmu driver to maintain virtual address */
        _etext = .;

        /**
        * Similar to _iram_start, this symbol goes here so it is
        * resolved by addr2line in preference to the first symbol in
        * the flash.text segment.
        */
        _flash_cache_start = ABSOLUTE(0);
    } > FLASH_text_seg

    /**
    * This dummy section represents the .flash.text section but in FLASH_data_seg.
    * Thus, it must have its alignment and (at least) its size.
    */
    .flash_rodata_dummy (NOLOAD):
    {
        _flash_rodata_dummy_start = ABSOLUTE(.);
        /* Start at the same alignment constraint than .flash.text */
        . = ALIGN(ALIGNOF(.flash.text));
        /* Create an empty gap as big as .flash.text section */
        . = . + SIZEOF(.flash.text);
        /* Prepare the alignment of the section above. Few bytes (0x20) must be
        * added for the mapping header. */
        . = ALIGN(0x10000) + 0x20;
        _rodata_reserved_start = .;  /* This is a symbol marking the flash.rodata start, this can be used for mmu driver to maintain virtual address */
    } > FLASH_data_seg

    .flash.appdesc : ALIGN(0x10)
    {
        _rodata_start = ABSOLUTE(.);

        *(.rodata_desc .rodata_desc.*)               /* Should be the first.  App version info.        DO NOT PUT ANYTHING BEFORE IT! */
        *(.rodata_custom_desc .rodata_custom_desc.*) /* Should be the second. Custom app version info. DO NOT PUT ANYTHING BEFORE IT! */

        /* Create an empty gap within this section. Thanks to this, the end of this
        * section will match .flah.rodata's begin address. Thus, both sections
        * will be merged when creating the final bin image. */
        . = ALIGN(ALIGNOF(.flash.rodata));
    } >FLASH_data_seg

    .flash.rodata : ALIGN(0x10)
    {
        _flash_rodata_start = ABSOLUTE(.);

        mapping[flash_rodata]

        *(.irom1.text) /* catch stray ICACHE_RODATA_ATTR */
        *(.gnu.linkonce.r.*)
        *(.rodata1)
        __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
        *(.xt_except_table)
        *(.gcc_except_table .gcc_except_table.*)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)
        . = (. + 3) & ~ 3;
        __eh_frame = ABSOLUTE(.);
        KEEP(*(.eh_frame))
        . = (. + 7) & ~ 3;
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
        __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
        *(.xt_except_desc)
        *(.gnu.linkonce.h.*)
        __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
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
        _rodata_end = ABSOLUTE(.);
        /* Literals are also RO data. */
        _lit4_start = ABSOLUTE(.);
        *(*.lit4)
        *(.lit4.*)
        *(.gnu.linkonce.lit4.*)
        _lit4_end = ABSOLUTE(.);
        . = ALIGN(4);
        _thread_local_start = ABSOLUTE(.);
        *(.tdata)
        *(.tdata.*)
        *(.tbss)
        *(.tbss.*)
        _thread_local_end = ABSOLUTE(.);
        _rodata_reserved_end = ABSOLUTE(.);  /* This is a symbol marking the flash.rodata end, this can be used for mmu driver to maintain virtual address */
        . = ALIGN(4);
    } > FLASH_data_seg

    _flash_rodata_align = ALIGNOF(.flash.rodata);

    .flash.rodata_noload (NOLOAD) :
    {
        . = ALIGN (4);
        mapping[rodata_noload]
    } > FLASH_data_seg

/*
    .ext_ram.dummy (NOLOAD):
    {
        . = ORIGIN(extern_ram_seg) + (_rodata_reserved_end - _flash_rodata_dummy_start);
        . = ALIGN (0x10000);
    } > extern_ram_seg

    .ext_ram.bss (NOLOAD) :
    {
        _ext_ram_bss_start = ABSOLUTE(.);
        mapping[extern_ram]
        . = ALIGN(4);
        _ext_ram_bss_end = ABSOLUTE(.);
    } > extern_ram_seg
*/

    /* Marks the end of IRAM code segment */
    .iram0.text_end (NOLOAD) :
    {
        /* iram_end_test section exists for use by memprot unit tests only */
        *(.iram_end_test)
        /* ESP32-S3 memprot requires 16B padding for possible CPU prefetch and 256B alignment for PMS split lines */
        . += 16;
        . = ALIGN(256);
        _iram_text_end = ABSOLUTE(.);
    } > SRAM_text_seg

    .iram0.data :
    {
        . = ALIGN(4);
        _iram_data_start = ABSOLUTE(.);

        mapping[iram0_data]

        _iram_data_end = ABSOLUTE(.);
    } > SRAM_text_seg

    .iram0.bss (NOLOAD) :
    {
        . = ALIGN(4);
        _iram_bss_start = ABSOLUTE(.);

        mapping[iram0_bss]

        _iram_bss_end = ABSOLUTE(.);
        . = ALIGN(4);
        _iram_end = ABSOLUTE(.);
    } > SRAM_text_seg

    /* Marks the end of data, bss and possibly rodata  */
    .dram0.heap_start (NOLOAD) :
    {
        . = ALIGN (8);
        _heap_start = ABSOLUTE(.);
    } > SRAM_data_seg

    .rtc.text :
    {
        _rtc_fast_start = ABSOLUTE(.);
        _rtc_text_start = ABSOLUTE(.);
        *(.rtc.entry.text)

        mapping[rtc_text]

        *rtc_wake_stub*.*(.literal .text .literal.* .text.*)
        *(.rtc_text_end_test)

        /* 16B padding for possible CPU prefetch and 4B alignment for PMS split lines */
        . += 16;

        . = ALIGN(4);
        _rtc_text_end = ABSOLUTE(.);
    } > RTC_fast_seg

    .rtc.force_fast :
    {
        _rtc_force_fast_start = ABSOLUTE(.);

        mapping[rtc_force_fast]
        *(.rtc.force_fast .rtc.force_fast.*)

        . = ALIGN(4);
        _rtc_force_fast_end = ABSOLUTE(.);
    } > RTC_fast_seg

    /* CONFIG_ESP32S3_RTCDATA_IN_FAST_MEM option. */
    .rtc.data :
    {
        _rtc_data_start = ABSOLUTE(.);

        mapping[rtc_data]
        *rtc_wake_stub*.*(.data .rodata .data.* .rodata.* .bss .bss.*)

        . = ALIGN(4);
        _rtc_data_end = ABSOLUTE(.);
    } > RTC_fast_seg

    /* RTC bss, from any source file named rtc_wake_stub*.c */
    .rtc.bss (NOLOAD) :
    {
        _rtc_bss_start = ABSOLUTE(.);
        *rtc_wake_stub*.*(.bss .bss.*)
        *rtc_wake_stub*.*(COMMON)

        mapping[rtc_bss]

        . = ALIGN(4);
        _rtc_bss_end = ABSOLUTE(.);
    } > RTC_fast_seg

    .rtc_noinit (NOLOAD):
    {
        _rtc_noinit_start = ABSOLUTE(.);
        *(.rtc_noinit .rtc_noinit.*)

        . = ALIGN(4);
        _rtc_noinit_end = ABSOLUTE(.);
    } > RTC_fast_seg

    .rtc.force_slow :
    {
        _rtc_force_slow_start = ABSOLUTE(.);
        *(.rtc.force_slow .rtc.force_slow.*)

        . = ALIGN(4);
        _rtc_force_slow_end = ABSOLUTE(.);
    } > RTC_slow_seg

    .xt.prop 0 :
    {
        KEEP (*(.xt.prop .gnu.linkonce.prop.*))
    }

    .xt.lit 0 :
    {
        KEEP (*(.xt.lit .gnu.linkonce.p.*))
    }
}
