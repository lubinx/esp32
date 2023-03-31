#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "soc.h"

#include "esp_image.h"
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#include "esp_log.h"

#include "cache_ll.h"
#include "mmu_ll.h"
#include "esp32s3/rom/cache.h"

#include "sdkconfig.h"

static char const *TAG = "bootloader";

// FLASH_read() => memcpy() overread
#pragma GCC diagnostic ignored "-Wstringop-overread"

/****************************************************************************
 *  imports
*****************************************************************************/
extern uintptr_t __bss_start__;
extern uintptr_t __bss_end__;

/**
 *  these functions defined in esp32s3.rom.ld, but defines nowhere
 *      guess its modifies register EXTMEM, see soc/extmem_struct.h
 */
extern void rom_config_instruction_cache_mode(uint32_t cfg_cache_size, uint8_t cfg_cache_ways, uint8_t cfg_cache_line_size);
extern void rom_config_data_cache_mode(uint32_t cfg_cache_size, uint8_t cfg_cache_ways, uint8_t cfg_cache_line_size);

/****************************************************************************
 *  @def
*****************************************************************************/
#define FLASH_READ_MMU_VADDR            (SOC_DROM_HIGH - MMU_PAGE_SIZE)

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
    esp_segment_hdr_t hdr;

    uintptr_t aligned_vaddr;
    size_t aligned_size;
};

/****************************************************************************
 *  @internal
*****************************************************************************/
static void cache_hal_init(void);
static void cache_hal_enable(enum cache_type_t type);
static void cache_hal_disable(enum cache_type_t type);

static kernel_entry_t KERNEL_load(uintptr_t flash_location);
static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize);
static void MAP_flash_segment(struct flash_segment_t *seg);

/****************************************************************************
 *  @implements
*****************************************************************************/
void __attribute__((noreturn)) Reset_Handler(void)
{
    memset(&__bss_start__, 0, (uintptr_t)(&__bss_end__ - &__bss_start__) * sizeof(__bss_start__));

    // efuse esp_efuse_set_rom_log_scheme(): this will permanently disable logs before enter bootloader
    /*
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
    */

    #if CONFIG_ESP_SYSTEM_LOG_LEVEL
        esp_rom_install_uart_printf();
        esp_rom_uart_set_as_console(0);
    #endif

    #if XCHAL_ERRATUM_572
        uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
        WSR(MEMCTL, memctl);
    #endif

    // WDT, BOR
    RTCCNTL.fib_sel.rtc_fib_sel = 0;
    RTCCNTL.ana_conf.glitch_rst_en = 0;
    RTCCNTL.swd_conf.swd_bypass_rst = 1;
    RTCCNTL.brown_out.ana_rst_en = 1;

    mmu_ll_unmap_all(0);
    mmu_ll_unmap_all(1);

    cache_hal_init();

    #if defined(CONFIG_ESP32S3_INSTRUCTION_CACHE_WRAP) || defined(CONFIG_ESP32S3_DATA_CACHE_WRAP)
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
    ESP_LOGD(TAG, "entry => %p\n", (void *)entry);

    // disable RWDT flashboot protection.
    RTCCNTL.wdt_wprotect = WDT_UNLOCK_VALUE;
    RTCCNTL.wdt_config0.flashboot_mod_en = 0;
    RTCCNTL.wdt_wprotect = WDT_LOCK_VALUE;

    // disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    TIMERG0.wdtwprotect.wdt_wkey = WDT_UNLOCK_VALUE;
    TIMERG0.wdtconfig0.wdt_flashboot_mod_en = 0;
    TIMERG0.wdtwprotect.wdt_wkey = WDT_LOCK_VALUE;

    // flash log
    while (UART0.status.txfifo_cnt);
    entry();
}

/****************************************************************************
 *  @internal
*****************************************************************************/
struct {
    uint32_t data_autoload_flag;
    uint32_t inst_autoload_flag;
} cache_ctx;

static void cache_hal_init(void)
{
    cache_ctx.data_autoload_flag = Cache_Disable_DCache();
    Cache_Enable_DCache(cache_ctx.data_autoload_flag);

    cache_ctx.inst_autoload_flag = Cache_Disable_ICache();
    Cache_Enable_ICache(cache_ctx.inst_autoload_flag);

    cache_ll_l1_enable_bus(0, CACHE_BUS_DBUS0);
    cache_ll_l1_enable_bus(0, CACHE_BUS_IBUS0);

    cache_ll_l1_enable_bus(1, CACHE_BUS_DBUS0);
    cache_ll_l1_enable_bus(1, CACHE_BUS_IBUS0);
}

static void cache_hal_enable(enum cache_type_t type)
{
    if (type == CACHE_TYPE_DATA) {
        Cache_Enable_DCache(cache_ctx.data_autoload_flag);
    } else if (type == CACHE_TYPE_INSTRUCTION) {
        Cache_Enable_ICache(cache_ctx.inst_autoload_flag);
    } else {
        Cache_Enable_ICache(cache_ctx.inst_autoload_flag);
        Cache_Enable_DCache(cache_ctx.data_autoload_flag);
    }
}

static void cache_hal_disable(enum cache_type_t type)
{
    if (type == CACHE_TYPE_DATA) {
        Cache_Disable_DCache();
    } else if (type == CACHE_TYPE_INSTRUCTION) {
        Cache_Disable_ICache();
    } else {
        Cache_Disable_ICache();
        Cache_Disable_DCache();
    }
}

static kernel_entry_t KERNEL_load(uintptr_t flash_location)
{
    esp_image_hdr_t hdr;
    struct flash_segment_t ro_seg = {0};
    struct flash_segment_t text_seg = {0};

    ESP_LOGD(TAG, "reading image at %p", (void *)flash_location);
    FLASH_read(flash_location, &hdr, sizeof(hdr));
    flash_location += sizeof(hdr);

    if (ESP_IMAGE_HEADER_MAGIC != hdr.magic)
    {
        ESP_LOGE(TAG, "error loading kernel: image hdr'TAG should be 0x%x, but 0x%x\n", ESP_IMAGE_HEADER_MAGIC, hdr.magic);
        goto kernel_load_error;
    }

    for (unsigned i = 0; i < hdr.segment_count; i ++)
    {
        esp_segment_hdr_t seg;

        FLASH_read(flash_location, &seg, sizeof(seg));
        flash_location += sizeof(seg);

        if (SOC_DROM_HIGH >= seg.load_addr && SOC_DROM_LOW <= seg.load_addr)
        {
            ESP_LOGD(TAG, "segment %u at %p map => %p DROM size=%ld", i, (void *)flash_location, (void *)seg.load_addr, seg.data_len);

            ro_seg.location = flash_location;
            ro_seg.hdr = seg;
        }
        else if (SOC_IROM_HIGH >= seg.load_addr && SOC_IROM_LOW <= seg.load_addr)
        {
            ESP_LOGD(TAG, "segment %u at %p map => %p IROM size=%ld", i, (void *)flash_location, (void *)seg.load_addr, seg.data_len);

            text_seg.location = flash_location;
            text_seg.hdr = seg;
        }
        else
        {
            if (seg.load_addr)
            {
                ESP_LOGD(TAG, "segment %u at %p load=> %p %s size=%ld", i,
                    (void *)flash_location, (void *)seg.load_addr,
                    MEMORY_REGION_TAG(seg.load_addr), seg.data_len
                );

                uint8_t *dest = (uint8_t *)seg.load_addr;
                size_t readed = 0;

                while (readed < seg.data_len)
                    readed += (size_t)FLASH_read(flash_location + readed, dest + readed, seg.data_len - readed);
            }
            else
                ESP_LOGD(TAG, "segment %d at %p pad size=%ld", i, (void *)flash_location, seg.data_len);
        }

        flash_location += seg.data_len;
    }
    cache_hal_disable(CACHE_TYPE_ALL);
    mmu_ll_unmap_all(0);

    if (ro_seg.location)
    {
        MAP_flash_segment(&ro_seg);

        cache_ll_l1_enable_bus(0, CACHE_BUS_DBUS0);
        cache_ll_l1_enable_bus(1, CACHE_BUS_DBUS0);
    }

    if (text_seg.location)
    {
        MAP_flash_segment(&text_seg);

        cache_ll_l1_enable_bus(0, CACHE_BUS_IBUS0);
        cache_ll_l1_enable_bus(1, CACHE_BUS_IBUS0);
    }

    rom_config_instruction_cache_mode(CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE,
        CONFIG_ESP32S3_ICACHE_ASSOCIATED_WAYS,
        CONFIG_ESP32S3_INSTRUCTION_CACHE_LINE_SIZE
    );
    rom_config_data_cache_mode(CONFIG_ESP32S3_DATA_CACHE_SIZE,
        CONFIG_ESP32S3_DCACHE_ASSOCIATED_WAYS,
        CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE
    );

    cache_hal_enable(CACHE_TYPE_ALL);
    return (kernel_entry_t)hdr.entry_addr;

kernel_load_error:
    while(1) {}
}

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize)
{
    static uintptr_t current_paddr = (uintptr_t)-1;
    uint32_t paddr = mmu_ll_format_paddr(flash_location, 0);

    if (current_paddr != paddr)
    {
        current_paddr = paddr;
        cache_hal_disable(CACHE_TYPE_DATA);

        uint32_t entry_id = mmu_ll_get_entry_id(FLASH_READ_MMU_VADDR);
        mmu_ll_write_entry(entry_id, paddr, MMU_TARGET_FLASH0);
        cache_hal_enable(CACHE_TYPE_DATA);
    }

    off_t offset = flash_location & (MMU_PAGE_SIZE - 1);
    void *src = (void *)(FLASH_READ_MMU_VADDR + offset);

    int bytes_remain = MMU_PAGE_SIZE - offset;
    bufsize = bufsize < (size_t)bytes_remain ? bufsize : (size_t)bytes_remain;

    memcpy(buf, src, bufsize);
    return (ssize_t)bufsize;
}

static void MAP_flash_segment(struct flash_segment_t *seg)
{
    unsigned offset = seg->location & (MMU_PAGE_SIZE - 1);

    seg->aligned_vaddr = seg->hdr.load_addr & ~((uintptr_t)MMU_PAGE_SIZE - 1);
    seg->aligned_size = (seg->hdr.data_len + offset + MMU_PAGE_SIZE - 1) & ~((uintptr_t)MMU_PAGE_SIZE - 1);

    int pages = (int)seg->aligned_size / MMU_PAGE_SIZE;

    uint32_t entry_id = mmu_ll_get_entry_id(seg->aligned_vaddr);
    uint32_t paddr = mmu_ll_format_paddr(seg->location, 0);

    while (pages)
    {
        mmu_ll_write_entry(entry_id, paddr, MMU_TARGET_FLASH0);

        entry_id ++;
        paddr ++;
        pages --;
    }
}
