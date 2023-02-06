ENTRY(Startup_Handler);

SECTIONS
{
    .intr.vectors :
    {
PROVIDE(_iram_start = ABSOLUTE(.));
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

        *(.UserEnter.literal);
        *(.UserEnter.text);

        . = ALIGN (16);
        *(.entry.text)
        *(.init.literal)
        *(.init)

        . = ALIGN(4);
    } > SRAM_text_seg

    .iram_attr.text :
    {
PROVIDE(_iram_text_start = ABSOLUTE(.));
        mapping[iram0_text]

        /* catch stray IRAM_ATTR */
        *(.iram*)
        /* iram_end_test section exists for use by memprot unit tests only */
        /* *(.iram_end_test) */

        /* ESP32-S3 memprot requires 16B padding for possible CPU prefetch  & alignment for PMS*/
        . += 16;
        . = ALIGN(256);
PROVIDE(_iram_text_end = ABSOLUTE(.));
PROVIDE(_iram_end = ABSOLUTE(.));
    } > SRAM_text_seg

    .__ram_overlapped_dummy (NOLOAD):
    {
        . = ORIGIN(SRAM_data_seg) + (_iram_end > SRAM_TEXT_OVERLAPPED_START ? _iram_end - SRAM_TEXT_OVERLAPPED_START : 0);
    } > SRAM_data_seg

    .data.rw :
    {
PROVIDE(_data_start = ABSOLUTE(.));
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.sdata*)
        *(.gnu.linkonce.s.*)
        *(.gnu.linkonce.s2.*)
        *(.jcr)

        mapping[dram0_data]

        . = ALIGN(4);
PROVIDE(_data_end = ABSOLUTE(.));
    } > SRAM_data_seg

    .data.noinit (NOLOAD) :
    {
        *(.noinit .noinit.*)
        . = ALIGN(4);
    }

    .data.zi (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(__bss_start__ = ABSOLUTE(.));

        mapping[dram0_bss]

        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
        *(.sbss2*)
        *(.gnu.linkonce.sb2.*)
        *(.dynsbss)
        *(.dynbss)
        *(.share.mem)
        *(.gnu.linkonce.b.*)

        . = ALIGN(8);
        PROVIDE(__bss_end__ = ABSOLUTE(.));
    } > SRAM_data_seg

    .data.ro : ALIGN(0x10)
    {
        /*
        *(.rodata_desc .rodata_desc.*)
        *(.rodata_custom_desc .rodata_custom_desc.*)
        . = ALIGN(4);
        */

PROVIDE(__zero_table_start__ = ABSOLUTE(.));
        LONG (__bss_start__)
        LONG (__bss_end__ - __bss_start__)
        LONG (__rtc_bss_start__)
        LONG (__rtc_bss_end__ - __rtc_bss_start__)
        /*
        LONG (_iram_bss_start)
        LONG (_iram_bss_end - _iram_bss_start)
        */
PROVIDE(__zero_table_end__ = ABSOLUTE(.));

PROVIDE(_flash_rodata_start = ABSOLUTE(.));

        mapping[flash_rodata]

        *(.irom1.text)                  /* catch stray ICACHE_RODATA_ATTR */
        *(.gnu.linkonce.r.*)
        *(.rodata1)

        . = ALIGN(4);   /* WTF? . = (. + 3) & ~ 3; */
        __eh_frame = ABSOLUTE(.);
        KEEP(*(.eh_frame))

        . = ALIGN(4);
        __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
        *(.xt_except_table)
        *(.gcc_except_table .gcc_except_table.*)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)

        /*constructor and destructor tables */
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

        /* REVIEW: This section holds .ext_ram.bss data, and will be put in PSRAM */
        /*
        _ext_ram_bss_start = ABSOLUTE(.);
        mapping[extern_ram]

        . = ALIGN(4);
        _ext_ram_bss_end = ABSOLUTE(.);
        */
PROVIDE(_flash_rodata_end = ABSOLUTE(.));
    } > FLASH_data_seg
PROVIDE(_flash_rodata_align = ALIGNOF(.data.ro));

    .data.ro_noload (NOLOAD) :
    {
        /* REVIEW: what is this? */
        mapping[rodata_noload]
        . = ALIGN (4);
    } > FLASH_data_seg

    .__flash_overlapped_dummy (NOLOAD):
    {
        . = . + SIZEOF(.data.ro) + SIZEOF(.data.ro_noload);
        . = ALIGN(MMU_PAGE_SIZE);
    } > FLASH_text_seg

    /* REVIEW: these section is impossiable to exists?
    .data.iram0 :
    {
        _iram_data_start = ABSOLUTE(.);
        mapping[iram0_data]
        . = ALIGN(4);
        _iram_data_end = ABSOLUTE(.);
    } > SRAM_text_seg

    .bss.iram (NOLOAD) :
    {
        _iram_bss_start = ABSOLUTE(.);
        mapping[iram0_bss]
        _iram_bss_end = ABSOLUTE(.);
        . = ALIGN(4);
    } > SRAM_text_seg
    */

    .flash.text :
    {
        *(.fini.literal)
        *(.fini)
        *(.gnu.version)

        *(.literal* .text* .stub .gnu.warning .gnu.linkonce.literal* .gnu.linkonce.t.*)

        mapping[flash_text]

        /* catch stray ICACHE_RODATA_ATTR */
        *(.irom0.text)

        /* ESP32-S3 memprot requires 16B padding for possible CPU prefetch  & alignment for PMS*/
        . += 16;
    } > FLASH_text_seg

    /* heap should present here see memory.ld appendix */
    .__heap_dummy(NOLOAD) :
    {
        . = ALIGN(8);
PROVIDE(_heap_start = ABSOLUTE(.));
PROVIDE(end = ABSOLUTE(.));

        . = ABSOLUTE(SRAM_ESP_SYSTEM_START);

PROVIDE(__heap_limit__ = ABSOLUTE(.));
    } > SRAM_data_seg

    .__stacks_dummy(NOLOAD) :
    {
        /* stacks should present here see memory.ld appendix */
        . = ALIGN(8);
        . += 16384;
    } > SRAM_data_seg

    .__esp_system_reserved (NOLOAD) :
    {
        . = ORIGIN(SRAM_data_seg) + LENGTH(SRAM_data_seg) - DCACHE_SIZE;
    } > SRAM_data_seg

    .__dcache_unused_dummy (NOLOAD) :
    {
        . += DCACHE_SIZE;
    } > SRAM_data_seg

    .rtc.text :
    {
        *(.rtc.entry.text)

        mapping[rtc_text]

        *rtc_wake_stub*.*(.literal* .text*)
        *(.rtc_text_end_test)

        /* 16B padding for possible CPU prefetch and 4B alignment for PMS split lines */
        . += 16;
        . = ALIGN(4);
PROVIDE(_rtc_text_end = ABSOLUTE(.));

        mapping[rtc_force_fast]
        *(.rtc.force_fast*)

        . = ALIGN(4) ;
PROVIDE(_rtc_force_fast_end = ABSOLUTE(.));
    } > RTC_fast_seg

    .rtc.data :
    {
        mapping[rtc_data]

        *rtc_wake_stub*.*(.data* .rodata*)
        . = ALIGN(4) ;

        *(.rtc.force_slow*)
        . = ALIGN(4) ;
    } > RTC_slow_seg

    .rtc.bss (NOLOAD) :
    {
PROVIDE(__rtc_bss_start__ = ABSOLUTE(.));

        *rtc_wake_stub*.*(.bss*)
        *rtc_wake_stub*.*(COMMON)

        mapping[rtc_bss]

        . = ALIGN(4) ;
PROVIDE(__rtc_bss_end__ = ABSOLUTE(.));
    } > RTC_slow_seg

    .rtc_noinit (NOLOAD):
    {
        *(.rtc_noinit*)
        . = ALIGN(4) ;
    } > RTC_slow_seg

    .xt.prop 0 :
    {
        KEEP (*(.xt.prop* .gnu.linkonce.prop.*))
    }

    .xt.lit 0 :
    {
        KEEP (*(.xt.lit* .gnu.linkonce.p.*))
    }
}
