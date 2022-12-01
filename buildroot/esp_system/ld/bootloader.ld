ENTRY(bootloader_startup);

/*
 * 40370000 <- IRAM/Icache -> 40378000 <- D/IRAM (I) -> 403E0000
 *                            3FC88000 <- D/IRAM (D) -> 3FCF0000 <- DRAM/DCache -> 3FD00000
*/

/**
 *  Memory Usage of ROM 1st bootloader
 *
 *      0x3fcd7e00 ------------------> _dram0_0_start
 *                 |               |
 *                 |               |
 *                 |               |   1. Large buffers that are only used in certain boot modes, see shared_buffers.h
 *                 |               |
 *                 |               |
 *      0x3fce9710 ------------------> __stack_sentry
 *                 |               |
 *                 |               |   2. Startup pro cpu stack (freed when IDF app is running)
 *                 |               |
 *      0x3fceb710 ------------------> __stack (pro cpu)
 *                 |               |
 *                 |               |      Startup app cpu stack
 *                 |               |
 *      0x3fced710 ------------------> __stack_app (app cpu)
 *                 |               |
 *                 |               |
 *                 |               |   3. Shared memory only used in startup code or nonos/early boot*
 *                 |               |      (can be freed when IDF runs)
 *                 |               |
 *                 |               |
 *      0x3fceee34 ------------------> _dram0_rtos_reserved_start
 *                 |               |
 *                 |               |
 *                 |               |   4. Shared memory used in startup code and when IDF runs
 *                 |               |
 *                 |               |
 *      0x3fcef770 ------------------> _dram0_rtos_reserved_end
 *                 |               |
 *      0x3fcef81c ------------------> _data_start_interface
 *                 |               |
 *                 |               |   5. End of DRAM is the 'interface' data with constant addresses (ECO compatible)
 *                 |               |
 *      0x3fcf0000 ------------------> _data_end_interface
*/
MEMORY
{
    startup_text_seg (RWX) :        org = 0x40378000, len = 416K
    startup_data_seg (RW) :         org = 0x3FC88000, len = 416K

    drom0_0_seg (R) :               org = 0x3C000000, len = 32M
}

SECTIONS
{
    .startup.text :
    {
        *(.stub .gnu.warning .gnu.linkonce.literal* .gnu.linkonce.t.*)
        *(.iram1 .iram1.*) /* catch stray IRAM_ATTR */

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

        *(.fini.literal)
        *(.fini)
        *(.gnu.version)

        . = ALIGN (16);
        *(.entry.text)
        *(.init.literal)
        *(.init)

        *(.literal* .text* .stub .gnu.warning .gnu.linkonce.literal* .gnu.linkonce.t.*)
        *(.iram .iram.*) /* catch stray IRAM_ATTR */
        *(.fini.literal)
        *(.fini)
        *(.gnu.version)

        /* CPU will try to prefetch 16bytes */
        . += 16;
        . = ALIGN(16);
    } > startup_text_seg

    .__pad.text_sizeof (NOLOAD) :
    {
        . += SIZEOF(.startup.text);
    } > startup_data_seg

    .startup.rodata :
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
    } > startup_data_seg

    .data :
    {
        _data_start = ABSOLUTE(.);
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(.gnu.linkonce.s2.*)
        *(.jcr)
        _data_end = ABSOLUTE(.);
    } > startup_data_seg

    /* Shared RAM */
    .bss (NOLOAD) :
    {
        . = ALIGN (8);
        _bss_start = ABSOLUTE(.);
        *(.dynsbss)
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
        *(.sbss2)
        *(.sbss2.*)
        *(.gnu.linkonce.sb2.*)
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN (8);
        _bss_end = ABSOLUTE(.);
    } > startup_data_seg

    .flash_rodata_dummy (NOLOAD):
    {
        _flash_rodata_dummy_start = ABSOLUTE(.);
        . = ALIGN(0x10000) + 0x20;
        _rodata_reserved_start = .;  /* This is a symbol marking the flash.rodata start, this can be used for mmu driver to maintain virtual address */
    } > drom0_0_seg

    .flash.rodata :
    {
        *(.rodata*)
        *(.sdata*)
        *(.gnu.linkonce.r.*)
    } > drom0_0_seg

    .flash.text :
    {
        *(.text*)

        __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
        *(.xt_except_table)
        *(.gcc_except_table)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)
        *(.eh_frame)
        . = (. + 3) & ~ 3;
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
    } > drom0_0_seg

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
