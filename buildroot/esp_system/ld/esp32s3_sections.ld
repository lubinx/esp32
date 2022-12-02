/* bootloader_support/bootloader_start.c */
ENTRY(bootloader_startup)
/* ENTRY(call_start_cpu0); */

_diram_i_start = 0x40378000;

SECTIONS
{
    .intr.vectors :
    {
        _iram_start = ABSOLUTE(.);
PROVIDE(_vector_table = ABSOLUTE(.));
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
PROVIDE(_invalid_pc_placeholder = ABSOLUTE(.));
        *(.*Vector.literal)

        PROVIDE(_init_end = ABSOLUTE(.));
    } > iram0_0_seg

    .startup.text :
    {
        *fpga_overrides.*(.literal.bootloader_fill_random .text.bootloader_fill_random)
        *libbootloader_support.a:bootloader_clock_loader.*(.literal* .text*)
        *libbootloader_support.a:bootloader_common_loader.*(.literal* .text*)
        *libbootloader_support.a:bootloader_flash.*(.literal* .text*)
        *libbootloader_support.a:bootloader_random.*(.literal* .text*)
        *libbootloader_support.a:bootloader_random*.*(.literal.bootloader_random_disable .text.bootloader_random_disable)
        *libbootloader_support.a:bootloader_efuse.*(.literal* .text*)
        *libbootloader_support.a:bootloader_utility.*(.literal* .text*)
        *libbootloader_support.a:bootloader_sha.*(.literal* .text*)
        *libbootloader_support.a:bootloader_console_loader.*(.literal* .text*)
        *libbootloader_support.a:bootloader_panic.*(.literal* .text*)
        *libbootloader_support.a:bootloader_soc.*(.literal* .text*)
        *libbootloader_support.a:esp_image_format.*(.literal* .text*)
        *libbootloader_support.a:flash_encrypt.*(.literal* .text*)
        *libbootloader_support.a:flash_encryption_secure_features.*(.literal* .text*)
        *libbootloader_support.a:flash_partitions.*(.literal* .text*)
        *libbootloader_support.a:secure_boot.*(.literal* .text*)
        *libbootloader_support.a:secure_boot_secure_features.*(.literal* .text*)
        *libbootloader_support.a:secure_boot_signatures_bootloader.*(.literal* .text*)

        *libefuse.a:*.*(.literal* .text*)
        *libesp_hw_support.a:rtc_clk.*(.literal* .text*)
        *libesp_hw_support.a:rtc_time.*(.literal* .text*)
        *libesp_hw_support.a:regi2c_ctrl.*(.literal* .text*)

        *libhal.a:wdt_hal_iram.*(.literal* .text*)
        *libhal.a:mmu_hal.*(.literal* .text*)
        *libhal.a:cache_hal.*(.literal* .text*)
        *libhal.a:efuse_hal.*(.literal* .text*)

        *libgcc.a:(.literal* .text*)
        *libmicro-ecc.a:*.*(.literal* .text*)
        *libspi_flash.a:*.*(.literal* .text*)
    } > iram0_0_seg

    .__overlapped.iram_text (NOLOAD):
    {
        . = ORIGIN(dram0_0_seg) + MAX(_iram_end - _diram_i_start, 0);
    } > dram0_0_seg

    /**
     *  before entry.bootloader_startup()
     *      all segment except internal SRAM is inaccessable
     */
    .startup.data :
    {
        *bootloader_start.*(.data* .rodata*)
        *libesp_hw_support.a:(.data* .rodata*)
        *libbootloader_support.a:(.data* .rodata*)
        *libhal.a:(.data* .rodata*)

        *libefuse.a:(.data* .rodata*)
        *liblog.a:(.data* .rodata*)
        *libgcc.a:(.data* .rodata*)
        *libmicro-ecc.a:(.data* .rodata*)
        *libspi_flash.a:(.data* .rodata*)
    } > dram0_0_seg

    .iram0.text :
    {
        /* Code marked as running out of IRAM */
PROVIDE(_iram_text_start = ABSOLUTE(.));
        *(.UserEnter.literal);
        *(.UserEnter.text);

        . = ALIGN(16);
        *(.entry.text)
        *(.init.literal)
        *(.init)

        mapping[iram0_text]
    } > iram0_0_seg

    .dram0.data :
    {
PROVIDE(_data_start = ABSOLUTE(.));
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(.gnu.linkonce.s2.*)
        *(.jcr)

        mapping[dram0_data]

        . = ALIGN(4);
PROVIDE(_data_end = ABSOLUTE(.));
    } > dram0_0_seg

    .dram0.bss (NOLOAD) :
    {
        _noinit_start = ABSOLUTE(.);
        *(.noinit .noinit.*)
        _noinit_end = ABSOLUTE(.);

        . = ALIGN(8);
PROVIDE(_bss_start = ABSOLUTE(.));

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

        . = ALIGN(8);
PROVIDE(_bss_end = ABSOLUTE(.));
    } > dram0_0_seg

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

    .flash_rodata_dummy (NOLOAD):
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

    /**
     * This section is required to skip flash rodata sections, because `extern_ram_seg`
     * and `drom0_0_seg` are on the same bus
    */
    .ext_ram.dummy (NOLOAD):
    {
        . = ORIGIN(extern_ram_seg) + (_rodata_reserved_end - _flash_rodata_dummy_start);
        . = ALIGN(0x10000);
    } > extern_ram_seg

    /* This section holds .ext_ram.bss data, and will be put in PSRAM */
    .ext_ram.bss (NOLOAD) :
    {
        _ext_ram_bss_start = ABSOLUTE(.);
        mapping[extern_ram]

        . = ALIGN(4);
        _ext_ram_bss_end = ABSOLUTE(.);
    } > extern_ram_seg

    /* Marks the end of IRAM code segment */
    .iram0.text_end (NOLOAD) :
    {
        /* iram_end_test section exists for use by memprot unit tests only */
        *(.iram_end_test)

        /* ESP32-S3 memprot requires 16B padding for possible CPU prefetch */
        . += 16;
        /* ESP32-S3 256B alignment for PMS split lines*/
        . = ALIGN(256);
        _iram_text_end = ABSOLUTE(.);
    } > iram0_0_seg

    .iram0.data :
    {
        _iram_data_start = ABSOLUTE(.);
        mapping[iram0_data]
        . = ALIGN(4);
        _iram_data_end = ABSOLUTE(.);
    } > iram0_0_seg

    .iram0.bss (NOLOAD) :
    {
        _iram_bss_start = ABSOLUTE(.);
        mapping[iram0_bss]

        . = ALIGN(4);
        _iram_bss_end = ABSOLUTE(.);
        _iram_end = ABSOLUTE(.);
    } > iram0_0_seg

    /* Marks the end of data, bss and possibly rodata  */
    .dram0.heap_start (NOLOAD) :
    {
        . = ALIGN(8);

PROVIDE(_heap_start = ABSOLUTE(.));
PROVIDE(end = ABSOLUTE(.));         /* newlib TAG"end": heap starts here */
    } > dram0_0_seg
/* Heap ends at top of dram0_0_seg */
PROVIDE(_heap_end = ORIGIN(dram0_0_seg) + LENGTH(dram0_0_seg));

    .rtc.text :
    {
        _rtc_text_start = ABSOLUTE(.);
        *(.rtc.entry.text)
        mapping[rtc_text]
        *rtc_wake_stub*.*(.literal .text .literal.* .text.*)
        *(.rtc_text_end_test)

        /* 16B padding for possible CPU prefetch */
        . += 15;
        . = ALIGN(4);
        _rtc_text_end = ABSOLUTE(.);
    } > rtc_iram_seg

    .rtc.data :
    {
        _rtc_data_start = ABSOLUTE(.);
        mapping[rtc_data]
        *rtc_wake_stub*.*(.data .rodata .data.* .rodata.* .bss .bss.*)
        . = ALIGN(4);
        _rtc_data_end = ABSOLUTE(.);
    } > rtc_data_location

    /* RTC bss, from any source file named rtc_wake_stub*.c */
    .rtc.bss (NOLOAD) :
    {
        _rtc_noinit_start = ABSOLUTE(.);
        *(.rtc_noinit .rtc_noinit.*)
        . = ALIGN(4);
        _rtc_noinit_end = ABSOLUTE(.);

        _rtc_bss_start = ABSOLUTE(.);
        *rtc_wake_stub*.*(.bss .bss.*)
        *rtc_wake_stub*.*(COMMON)
        mapping[rtc_bss]
        . = ALIGN(4);
        _rtc_bss_end = ABSOLUTE(.);
    } > rtc_data_location

    .rtc.force_fast :
    {
        _rtc_force_fast_start = ABSOLUTE(.);
            mapping[rtc_force_fast]
            *(.rtc.force_fast .rtc.force_fast.*)

        . = ALIGN(4);
        _rtc_force_fast_end = ABSOLUTE(.);
    } > rtc_fast_seg

    .rtc.force_slow :
    {
        . = ALIGN(4);
        _rtc_force_slow_start = ABSOLUTE(.);
            *(.rtc.force_slow .rtc.force_slow.*)
        . = ALIGN(4);
        _rtc_force_slow_end = ABSOLUTE(.);
    } > rtc_slow_seg

    /** This section will be used by the debugger and disassembler to get more information
     * about raw data present in the code.
     * Indeed, it may be required to add some padding at some points in the code
     * in order to align a branch/jump destination on a particular bound.
     * Padding these instructions will generate null bytes that shall be
     * interpreted as data, and not code by the debugger or disassembler.
     * This section will only be present in the ELF file, not in the final binary
     * For more details, check GCC-212
    */
    .xt.prop 0 :
    {
        KEEP (*(.xt.prop .gnu.linkonce.prop.*))
    }

    .xt.lit 0 :
    {
        KEEP (*(.xt.lit .gnu.linkonce.p.*))
    }
}

ASSERT(((_iram_end - ORIGIN(iram0_0_seg)) <= LENGTH(iram0_0_seg)), "IRAM0 segment data does not fit.")
ASSERT(((_heap_start - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "DRAM segment data does not fit.")
