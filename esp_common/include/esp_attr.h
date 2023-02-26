
/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ESP_ATTR_H__
#define __ESP_ATTR_H__

    // deprecate function
    #define IDF_DEPRECATED(REASON)      __attribute__((deprecated(REASON)))

    // Forces a function to be inlined
    #define FORCE_INLINE_ATTR           static inline __attribute__((always_inline))
    // Forces to not inline function
    #define NOINLINE_ATTR               __attribute__((noinline))

    // Forces data to be 4 bytes aligned
    // #define WORD_ALIGNED_ATTR           __attribute__((aligned(4)))
    // Forces data to be placed to DMA-capable places
    // #define DMA_ATTR                    WORD_ALIGNED_ATTR DRAM_ATTR

   // Forces code into IRAM instead of flash
    #define IRAM_ATTR                   _SECTION_ATTR_IMPL(".iram1", __COUNTER__)

    // Forces data into DRAM instead of flash
    #define DRAM_ATTR                   _SECTION_ATTR_IMPL(".dram1", __COUNTER__)

    // Forces a string into DRAM instead of flash
    // Use as esp_rom_printf(DRAM_STR("Hello world!\n"));
    #define DRAM_STR(str)               (__extension__({static const DRAM_ATTR char __c[] = (str); (char const *)&__c;}))

    /*
    // Forces data into noinit section to avoid initialization after restart.
    #define __NOINIT_ATTR               _SECTION_ATTR_IMPL(".noinit", __COUNTER__)

    // Forces code into DRAM instead of flash and map it to coredump
    // Use dram2 instead of dram1 to make sure this section will not be included
    // by dram1 section in the linker script
    #define COREDUMP_DRAM_ATTR          _SECTION_ATTR_IMPL(".dram2.coredump", __COUNTER__)

    // Forces data into IRAM instead of DRAM
    #define IRAM_DATA_ATTR __attribute__((section(".iram.data")))

    // Forces data into IRAM instead of DRAM and map it to coredump
    #define COREDUMP_IRAM_DATA_ATTR _SECTION_ATTR_IMPL(".iram2.coredump", __COUNTER__)

    // Forces bss into IRAM instead of DRAM
    #define IRAM_BSS_ATTR __attribute__((section(".iram.bss")))

    // Forces data into RTC memory. See "docs/deep-sleep-stub.rst"
    // Any variable marked with this attribute will keep its value
    // during a deep sleep / wake cycle.
    #define RTC_DATA_ATTR _SECTION_ATTR_IMPL(".rtc.data", __COUNTER__)

    // Forces data into RTC memory of .noinit section.
    // Any variable marked with this attribute will keep its value
    // after restart or during a deep sleep / wake cycle.
    #define RTC_NOINIT_ATTR  _SECTION_ATTR_IMPL(".rtc_noinit", __COUNTER__)

    // Forces read-only data into RTC memory. See "docs/deep-sleep-stub.rst"
    #define RTC_RODATA_ATTR _SECTION_ATTR_IMPL(".rtc.rodata", __COUNTER__)

    // Forces data into RTC memory and map it to coredump
    #define COREDUMP_RTC_DATA_ATTR _SECTION_ATTR_IMPL(".rtc.coredump", __COUNTER__)

    // Allows to place data into RTC_SLOW memory.
    #define RTC_SLOW_ATTR _SECTION_ATTR_IMPL(".rtc.force_slow", __COUNTER__)

    // Forces code into RTC fast memory. See "docs/deep-sleep-stub.rst"
    #define RTC_IRAM_ATTR _SECTION_ATTR_IMPL(".rtc.text", __COUNTER__)

    // Allows to place data into RTC_FAST memory.
    #define RTC_FAST_ATTR _SECTION_ATTR_IMPL(".rtc.force_fast", __COUNTER__)

    // Allows to place data into RTC_FAST memory and map it to coredump
    #define COREDUMP_RTC_FAST_ATTR _SECTION_ATTR_IMPL(".rtc.fast.coredump", __COUNTER__)

    // Forces bss variable into external memory. "
    #define EXT_RAM_BSS_ATTR _SECTION_ATTR_IMPL(".ext_ram.bss", __COUNTER__)

    // Forces data into external memory noinit section to avoid initialization after restart.
    #define EXT_RAM_NOINIT_ATTR _SECTION_ATTR_IMPL(".ext_ram_noinit", __COUNTER__)
    */

    // Implementation for a unique custom section
    //
    // This prevents gcc producing "x causes a section type conflict with y"
    // errors if two variables in the same source file have different linkage (maybe const & non-const) but are placed in the same custom section
    //
    // Using unique sections also means --gc-sections can remove unused
    // data with a custom section type set
    #ifndef CONFIG_IDF_TARGET_LINUX
        #define _SECTION_ATTR_IMPL(SECTION, COUNTER) __attribute__((section(SECTION "." _COUNTER_STRINGIFY(COUNTER))))
        #define _COUNTER_STRINGIFY(COUNTER) #COUNTER
    #else
        // Custom section attributes are generally not used in the port files for Linux target, but may be found
        // in the common header files. Don't declare custom sections in that case.
        #define _SECTION_ATTR_IMPL(SECTION, COUNTER)
    #endif

    // REVIEW: remove these: This allows using enum as flags in C++
    // Format: FLAG_ATTR(flag_enum_t)
    #ifdef __cplusplus
        #define FLAG_ATTR_IMPL(TYPE, INT_TYPE) \
            FORCE_INLINE_ATTR constexpr TYPE operator~ (TYPE a) { return (TYPE)~(INT_TYPE)a; } \
            FORCE_INLINE_ATTR constexpr TYPE operator| (TYPE a, TYPE b) { return (TYPE)((INT_TYPE)a | (INT_TYPE)b); } \
            FORCE_INLINE_ATTR constexpr TYPE operator& (TYPE a, TYPE b) { return (TYPE)((INT_TYPE)a & (INT_TYPE)b); } \
            FORCE_INLINE_ATTR constexpr TYPE operator^ (TYPE a, TYPE b) { return (TYPE)((INT_TYPE)a ^ (INT_TYPE)b); } \
            FORCE_INLINE_ATTR constexpr TYPE operator>> (TYPE a, int b) { return (TYPE)((INT_TYPE)a >> b); } \
            FORCE_INLINE_ATTR constexpr TYPE operator<< (TYPE a, int b) { return (TYPE)((INT_TYPE)a << b); } \
            FORCE_INLINE_ATTR TYPE& operator|=(TYPE& a, TYPE b) { a = a | b; return a; } \
            FORCE_INLINE_ATTR TYPE& operator&=(TYPE& a, TYPE b) { a = a & b; return a; } \
            FORCE_INLINE_ATTR TYPE& operator^=(TYPE& a, TYPE b) { a = a ^ b; return a; } \
            FORCE_INLINE_ATTR TYPE& operator>>=(TYPE& a, int b) { a = a >> b; return a; } \
            FORCE_INLINE_ATTR TYPE& operator<<=(TYPE& a, int b) { a = a << b; return a; }

        #define FLAG_ATTR_U32(TYPE) FLAG_ATTR_IMPL(TYPE, uint32_t)
        #define FLAG_ATTR FLAG_ATTR_U32
    #else
        #define FLAG_ATTR(TYPE)
    #endif
#endif
