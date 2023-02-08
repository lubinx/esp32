#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"

#include "soc.h"
#include "hal/cache_hal.h"
#include "hal/cache_ll.h"
#include "hal/mmu_ll.h"
#include "hal/mmu_hal.h"
#include "hal/wdt_hal.h"

#include "bootloader_soc.h"
#include "bootloader_mem.h"

#include "esp_app_format.h"
#include "esp_log.h"

static char const *TAG = "bootloader";

// FLASH_read() => memcpy() overread
#pragma GCC diagnostic ignored "-Wstringop-overread"

/****************************************************************************
 *  imports
*****************************************************************************/
extern intptr_t __bss_start__;
extern intptr_t __bss_end__;

/**
 *  these functions defined in esp32s3.rom.ld, but defines nowhere
 *      guess its modifies register EXTMEM, see soc/extmem_struct.h
 */
extern void rom_config_instruction_cache_mode(uint32_t cfg_cache_size, uint8_t cfg_cache_ways, uint8_t cfg_cache_line_size);
extern void rom_config_data_cache_mode(uint32_t cfg_cache_size, uint8_t cfg_cache_ways, uint8_t cfg_cache_line_size);

/****************************************************************************
 *  consts
*****************************************************************************/
#define FLASH_READ_MMU_VADDR            (SOC_DROM_HIGH - CONFIG_MMU_PAGE_SIZE)

#define MEMORY_REGION_TAG(ADDR)         (\
    (SOC_IRAM_HIGH >= ADDR && SOC_IRAM_LOW <= ADDR) ? "IRAM" :  \
    (SOC_DRAM_HIGH >= ADDR && SOC_DRAM_LOW <= ADDR) ? "DRAM" :  \
    (SOC_RTC_IRAM_HIGH >= ADDR && SOC_RTC_IRAM_LOW <= ADDR) ? "fRTC" :  \
    (SOC_RTC_DATA_HIGH >= ADDR && SOC_RTC_DATA_LOW <= ADDR) ? "sRTC" : "    "    \
)

typedef void __attribute__((noreturn)) (*kernel_entry_t)(void);

struct flash_segment_t
{
    uintptr_t location;
    esp_image_segment_header_t hdr;

    uintptr_t aligned_vaddr;
    size_t aligned_size;
};

/****************************************************************************
 *  local
*****************************************************************************/
static kernel_entry_t KERNEL_load(uintptr_t flash_location);

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize);
static void MAP_flash_segment(struct flash_segment_t *seg);

/****************************************************************************
 *  exports
*****************************************************************************/
void __attribute__((noreturn)) Reset_Handler(void)
{
    // efuse esp_efuse_set_rom_log_scheme(): this will permanently disable logs before enter bootloader
    #if CONFIG_BOOT_ROM_LOG_ALWAYS_OFF
        #define ROM_LOG_MODE                ESP_EFUSE_ROM_LOG_ALWAYS_OFF
    #elif CONFIG_BOOT_ROM_LOG_ON_GPIO_LOW
        #define ROM_LOG_MODE                ESP_EFUSE_ROM_LOG_ON_GPIO_LOW
    #elif CONFIG_BOOT_ROM_LOG_ON_GPIO_HIGH
        #define ROM_LOG_MODE                ESP_EFUSE_ROM_LOG_ON_GPIO_HIGH
    #endif
    #ifdef ROM_LOG_MODE
        esp_efuse_set_rom_log_scheme(ROM_LOG_MODE);
    #endif

    #if CONFIG_BOOTLOADER_LOG_LEVEL
        esp_rom_install_uart_printf();
        esp_rom_uart_set_as_console(CONFIG_ESP_CONSOLE_UART_NUM);
    #endif

    #if XCHAL_ERRATUM_572
        uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
        WSR(MEMCTL, memctl);
    #endif

    // Enable WDT, BOR
    bootloader_ana_super_wdt_reset_config(true);
    bootloader_ana_bod_reset_config(true);
    bootloader_ana_clock_glitch_reset_config(false);

    bootloader_init_mem();
    // bootloader_clock_configure();

    memset(&__bss_start__, 0, (&__bss_end__ - &__bss_start__) * sizeof(__bss_start__));

    mmu_hal_init();
    cache_hal_init();

    #if CONFIG_ESP32S2_INSTRUCTION_CACHE_WRAP || CONFIG_ESP32S2_DATA_CACHE_WRAP || \
        CONFIG_ESP32S3_INSTRUCTION_CACHE_WRAP || CONFIG_ESP32S3_DATA_CACHE_WRAP
        uint32_t icache_wrap_enable =
            #if CONFIG_ESP32S2_INSTRUCTION_CACHE_WRAP || CONFIG_ESP32S3_INSTRUCTION_CACHE_WRAP
                1;
            #else
                0;
            #endif
        uint32_t dcache_wrap_enable =
            #if CONFIG_ESP32S2_DATA_CACHE_WRAP || CONFIG_ESP32S3_DATA_CACHE_WRAP
                1;
            #else
                0;
            #endif
        extern void esp_enable_cache_wrap(uint32_t icache_wrap_enable, uint32_t dcache_wrap_enable);
        esp_enable_cache_wrap(icache_wrap_enable, dcache_wrap_enable);
    #endif

    kernel_entry_t entry = KERNEL_load(0x10000);
    ESP_LOGI(TAG, "entry => %p\n", entry);

    // disable RWDT flashboot protection.
    wdt_hal_context_t rwdt_ctx =
        #if CONFIG_IDF_TARGET_ESP32C6 // TODO: IDF-5653
            {.inst = WDT_RWDT, .rwdt_dev = &LP_WDT};
        #else
            {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};
        #endif
    wdt_hal_write_protect_disable(&rwdt_ctx);
    wdt_hal_set_flashboot_en(&rwdt_ctx, false);
    wdt_hal_write_protect_enable(&rwdt_ctx);

    // disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
    wdt_hal_write_protect_disable(&mwdt_ctx);
    wdt_hal_set_flashboot_en(&mwdt_ctx, false);
    wdt_hal_write_protect_enable(&mwdt_ctx);

    entry();
}

/****************************************************************************
 *  libc / posix polyfill
*****************************************************************************/
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;
}

int pthread_setcancelstate(int state, int *oldstate)
{
    return 0;
}

/****************************************************************************
 *  local
*****************************************************************************/
static kernel_entry_t KERNEL_load(uintptr_t flash_location)
{
    esp_image_header_t hdr;
    struct flash_segment_t ro_seg = {0};
    struct flash_segment_t text_seg = {0};

    ESP_LOGI(TAG, "reading image at %p", flash_location);
    FLASH_read(flash_location, &hdr, sizeof(hdr));
    flash_location += sizeof(hdr);

    if (ESP_IMAGE_HEADER_MAGIC != hdr.magic)
    {
        ESP_LOGE(TAG, "error loading kernel: image hdr'TAG should be 0x%x, but 0x%x\n", ESP_IMAGE_HEADER_MAGIC, hdr.magic);
        goto kernel_load_error;
    }

    for (int i = 0; i < hdr.segment_count; i ++)
    {
        esp_image_segment_header_t seg;

        FLASH_read(flash_location, &seg, sizeof(seg));
        flash_location += sizeof(seg);

        if (SOC_DROM_HIGH >= seg.load_addr && SOC_DROM_LOW <= seg.load_addr)
        {
            ESP_LOGI(TAG, "segment %d at %p map => %p DROM size=%d", i, flash_location, seg.load_addr, seg.data_len);

            ro_seg.location = flash_location;
            ro_seg.hdr = seg;
        }
        else if (SOC_IROM_HIGH >= seg.load_addr && SOC_IROM_LOW <= seg.load_addr)
        {
            ESP_LOGI(TAG, "segment %d at %p map => %p IROM size=%d", i, flash_location, seg.load_addr, seg.data_len);

            text_seg.location = flash_location;
            text_seg.hdr = seg;
        }
        else
        {
            ESP_LOGI(TAG, "segment %d at %p load=> %p %s size=%d", i, flash_location, seg.load_addr, MEMORY_REGION_TAG(seg.load_addr), seg.data_len);

            if (seg.load_addr)
            {
                uint8_t *dest = (uint8_t *)seg.load_addr;
                size_t readed = 0;

                while (readed < seg.data_len)
                    readed += FLASH_read(flash_location + readed, dest + readed, seg.data_len - readed);
            }
        }

        flash_location += seg.data_len;
    }
    cache_hal_disable(CACHE_TYPE_ALL);
    mmu_ll_unmap_all(0);

    if (ro_seg.location)
    {
        MAP_flash_segment(&ro_seg);

        cache_ll_l1_enable_bus(0, CACHE_BUS_DBUS0);
        #if ! CONFIG_FREERTOS_UNICORE
            cache_ll_l1_enable_bus(1, CACHE_BUS_DBUS0);
        #endif
    }

    if (text_seg.location)
    {
        MAP_flash_segment(&text_seg);

        cache_ll_l1_enable_bus(0, CACHE_BUS_IBUS0);
        #if ! CONFIG_FREERTOS_UNICORE
            cache_ll_l1_enable_bus(1, CACHE_BUS_IBUS0);
        #endif
    }

    rom_config_instruction_cache_mode(CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE,
        CONFIG_ESP32S3_ICACHE_ASSOCIATED_WAYS,
        CONFIG_ESP32S3_INSTRUCTION_CACHE_LINE_SIZE
    );
    rom_config_data_cache_mode(CONFIG_ESP32S3_DATA_CACHE_SIZE,
        CONFIG_ESP32S3_DCACHE_ASSOCIATED_WAYS,
        CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE
    );

    // REVIEW: unknown purpuse
    /*
    extern uint32_t Cache_Set_IDROM_MMU_Size(uint32_t irom_size, uint32_t drom_size);
    {
        uint32_t cache_mmu_irom_size = text_seg.aligned_size / CONFIG_MMU_PAGE_SIZE * sizeof(uint32_t);
        Cache_Set_IDROM_MMU_Size(cache_mmu_irom_size, CACHE_DROM_MMU_MAX_END - cache_mmu_irom_size);
    }

    extern void Cache_Set_IDROM_MMU_Info(uint32_t instr_page_num, uint32_t rodata_page_num,
        uint32_t rodata_start, uint32_t rodata_end, int i_off, int ro_off);
    {
        int s_instr_flash2spiram_off = 0;
        int s_rodata_flash2spiram_off = 0;
        #if CONFIG_SPIRAM_FETCH_INSTRUCTIONS
            extern int instruction_flash2spiram_offset(void);
            s_instr_flash2spiram_off = instruction_flash2spiram_offset();
        #endif
        #if CONFIG_SPIRAM_RODATA
            extern int rodata_flash2spiram_offset(void);
            s_rodata_flash2spiram_off = rodata_flash2spiram_offset();
        #endif

        Cache_Set_IDROM_MMU_Info(
            text_seg.aligned_size / CONFIG_MMU_PAGE_SIZE,
            ro_seg.aligned_size / CONFIG_MMU_PAGE_SIZE,
            ro_seg.hdr.load_addr, ro_seg.hdr.load_addr + ro_seg.hdr.data_len,
            s_instr_flash2spiram_off,
            s_rodata_flash2spiram_off
        );
    }
    */

    // REVIEW: unknown purpuse
    /*
    #if CONFIG_ESP32S3_DATA_CACHE_16KB
        Cache_Invalidate_DCache_All();
        Cache_Occupy_Addr(SOC_DROM_LOW, CONFIG_ESP32S3_DATA_CACHE_SIZE);
    #endif
    */

    cache_hal_enable(CACHE_TYPE_ALL);
    return (kernel_entry_t)hdr.entry_addr;

kernel_load_error:
    while(1) {}
}

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize)
{
    static uintptr_t current_paddr = (uintptr_t)-1;
    uint32_t paddr = mmu_ll_format_paddr(0, flash_location);

    if (current_paddr != paddr)
    {
        current_paddr = paddr;
        cache_hal_disable(CACHE_TYPE_DATA);

        uint32_t entry_id = mmu_ll_get_entry_id(0, FLASH_READ_MMU_VADDR);
        mmu_ll_write_entry(0, entry_id, paddr, MMU_TARGET_FLASH0);
        cache_hal_enable(CACHE_TYPE_DATA);
    }

    off_t offset = flash_location & (CONFIG_MMU_PAGE_SIZE - 1);
    void *src = (void *)(FLASH_READ_MMU_VADDR + offset);

    int bytes_remain = CONFIG_MMU_PAGE_SIZE - offset;
    bufsize = bufsize < bytes_remain ? bufsize : bytes_remain;

    memcpy(buf, src, bufsize);
    return bufsize;
}

static void MAP_flash_segment(struct flash_segment_t *seg)
{
    off_t offset = seg->location & (CONFIG_MMU_PAGE_SIZE - 1);

    seg->aligned_vaddr = seg->hdr.load_addr & ~(CONFIG_MMU_PAGE_SIZE - 1);
    seg->aligned_size = (seg->hdr.data_len + offset + CONFIG_MMU_PAGE_SIZE - 1) & ~(CONFIG_MMU_PAGE_SIZE - 1);

    int pages = seg->aligned_size / CONFIG_MMU_PAGE_SIZE;

    uint32_t entry_id = mmu_ll_get_entry_id(0, seg->aligned_vaddr);
    uint32_t paddr = mmu_ll_format_paddr(0, seg->location);

    while (pages)
    {
        mmu_ll_write_entry(0, entry_id, paddr, MMU_TARGET_FLASH0);

        entry_id ++;
        paddr ++;
        pages --;
    }
}
