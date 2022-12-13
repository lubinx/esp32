#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_rom_sys.h"

#include "hal/cache_ll.h"
#include "hal/mmu_ll.h"

#include "hal/cache_hal.h"
#include "hal/rtc_hal.h"
#include "hal/mmu_hal.h"
#include "hal/wdt_hal.h"

#include "bootloader_soc.h"
#include "bootloader_clock.h"

#include "esp_image_format.h"
#include "bootloader_flash.h"
#include "bootloader_flash_priv.h"
#include "esp_log.h"


/****************************************************************************
 *  imports
*****************************************************************************/
extern intptr_t _bss_start;
extern intptr_t _bss_end;

/****************************************************************************
 *  local
*****************************************************************************/
static __attribute__((noreturn)) void KERNEL_load(uintptr_t flash_location);
static void WDT_config(void);

#define MMU_FLASH_ENTRY_ID(VADDR)       (((VADDR) & MMU_VADDR_MASK) >> 16)
#define MMU_FLASH_PADDR(LOCATION)       ((LOCATION) >> 16)

#define MMU_FLASH_READ_VADDR            (SOC_DROM_HIGH - CONFIG_MMU_PAGE_SIZE)
#define MMU_FLASH_READ_ENTRY_ID         MMU_FLASH_ENTRY_ID(MMU_FLASH_READ_VADDR)

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize);
static void FLASH_mmap(uintptr_t flash_location, uintptr_t vaddr, size_t size);


/* Return true if load_addr is an address the bootloader should load into */
static bool should_load(uint32_t load_addr);
/* Return true if load_addr is an address the bootloader should map via flash cache */
static bool should_map(uint32_t load_addr);

static esp_err_t process_image_header(esp_image_metadata_t *data, uint32_t part_offset);
static esp_err_t process_segments(esp_image_metadata_t *data);
static esp_err_t process_segment_data(intptr_t load_addr, uint32_t data_addr, uint32_t data_len);


/****************************************************************************
 *  exports
*****************************************************************************/
void Reset_Handler(void)
{
#if XCHAL_ERRATUM_572
    uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
    WSR(MEMCTL, memctl);
#endif // XCHAL_ERRATUM_572

    // Enable WDT, BOR
    bootloader_ana_super_wdt_reset_config(true);
    bootloader_ana_bod_reset_config(true);
    bootloader_ana_clock_glitch_reset_config(false);

    // bootloader_super_wdt_auto_feed
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
    REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, 0);

    #if CONFIG_IDF_TARGET_ESP32C6
        // disable apm filter // TODO: IDF-5909
        REG_WRITE(LP_APM_FUNC_CTRL_REG, 0);
        REG_WRITE(LP_APM0_FUNC_CTRL_REG, 0);
        REG_WRITE(HP_APM_FUNC_CTRL_REG, 0);
    #endif

    #ifdef CONFIG_BOOTLOADER_REGION_PROTECTION_ENABLE
        // protect memory region
        esp_cpu_configure_region_protection();
    #endif

    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    #ifdef CONFIG_EFUSE_VIRTUAL
        ESP_LOGW(TAG, "eFuse virtual mode is enabled. If Secure boot or Flash encryption is enabled then it does not provide any security. FOR TESTING ONLY!");
        #ifndef CONFIG_EFUSE_VIRTUAL_KEEP_IN_FLASH
            esp_efuse_init_virtual_mode_in_ram();
        #endif
    #endif

    cache_hal_init();
    mmu_hal_init();

    WDT_config();
    bootloader_clock_configure();


    esp_image_metadata_t meta = {0};

    process_image_header(&meta, 0x10000);
    process_segments(&meta);

    cache_hal_disable(CACHE_TYPE_ALL);
    mmu_ll_unmap_all(0);

    uint32_t drom_addr = 0;
    uint32_t drom_load_addr = 0;
    uint32_t drom_size = 0;
    uint32_t irom_addr = 0;
    uint32_t irom_load_addr = 0;
    uint32_t irom_size = 0;

    // Find DROM & IROM addresses, to configure MMU mappings
    for (int i = 0; i < meta.image.segment_count; i++)
    {
        const esp_image_segment_header_t *header = &meta.segments[i];
        if (SOC_DROM_HIGH > header->load_addr && SOC_DROM_LOW <= header->load_addr)
        {
            drom_addr = meta.segment_data[i];
            drom_load_addr = header->load_addr;

            uint32_t drom_load_addr_aligned = drom_load_addr & MMU_FLASH_MASK;
            uint32_t drom_addr_aligned = drom_addr & MMU_FLASH_MASK;
            drom_size = (drom_load_addr - drom_load_addr_aligned) + header->data_len;

            uint32_t actual_mapped_len = 0;
            mmu_hal_map_region(0, MMU_TARGET_FLASH0, drom_load_addr_aligned, drom_addr_aligned, drom_size, &actual_mapped_len);

            cache_ll_l1_enable_bus(0, CACHE_BUS_DBUS0);
            #if ! CONFIG_FREERTOS_UNICORE
                cache_ll_l1_enable_bus(1, CACHE_BUS_DBUS0);
            #endif
        }
        if (SOC_IROM_HIGH > header->load_addr && SOC_IROM_LOW <= header->load_addr)
        {
            irom_addr = meta.segment_data[i];
            irom_load_addr = header->load_addr;

            uint32_t irom_load_addr_aligned = irom_load_addr & MMU_FLASH_MASK;
            uint32_t irom_addr_aligned = irom_addr & MMU_FLASH_MASK;
            irom_size = irom_load_addr - irom_load_addr_aligned + header->data_len;

            uint32_t actual_mapped_len = 0;
            mmu_hal_map_region(0, MMU_TARGET_FLASH0, irom_load_addr_aligned, irom_addr_aligned, irom_size, &actual_mapped_len);

            cache_ll_l1_enable_bus(0, CACHE_BUS_IBUS0);
            #if ! CONFIG_FREERTOS_UNICORE
                cache_ll_l1_enable_bus(1, CACHE_BUS_IBUS0);
            #endif
        }
    }
    cache_hal_enable(CACHE_TYPE_ALL);

    typedef void (*entry_t)(void) __attribute__((noreturn));
    entry_t entry = ((entry_t)meta.image.entry_addr);

    // TODO: we have used quite a bit of stack at this point.
    // use "movsp" instruction to reset stack back to where ROM stack starts.
    (*entry)();

    KERNEL_load(0x10000);
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

void esp_log_write(esp_log_level_t level, char const *tag, char const *format, ...)
{
}

/****************************************************************************
 *  local
*****************************************************************************/
static void KERNEL_load(uintptr_t flash_location)
{
    struct FLASH_segment
    {
        uintptr_t location;
        esp_image_segment_header_t hdr;
    };

    static esp_image_header_t hdr;
    static struct FLASH_segment ro_seg;
    static struct FLASH_segment text_seg;

    FLASH_read(flash_location, &hdr, sizeof(hdr));
    flash_location += sizeof(hdr);

    if (ESP_IMAGE_HEADER_MAGIC != hdr.magic)
    {
        esp_rom_printf("error loading kernel: invalid image tag(magic): %x\n", hdr.magic);
        goto kernel_load_error;
    }

    esp_rom_printf("\n");
    esp_rom_printf("entry_addr: 0x%08x\n", hdr.entry_addr);
    esp_rom_printf("segment_count:  0x%08x\n", hdr.segment_count);
    esp_rom_printf("\n");

    for (int i = 0; i < hdr.segment_count; i ++)
    {
        esp_image_segment_header_t seg;

        FLASH_read(flash_location, &seg, sizeof(seg));
        flash_location += sizeof(seg);

        esp_rom_printf("segment: %d, flash location: 0x%08x\n", i, flash_location);
        esp_rom_printf("load_addr: 0x%08x\n", seg.load_addr);
        esp_rom_printf("data_len: %u\n", seg.data_len);

        if (SOC_DRAM_HIGH >= seg.load_addr && SOC_DRAM_LOW <= seg.load_addr)
        {
            uint8_t *dest = (uint8_t *)seg.load_addr;
            size_t readed = 0;

            while (readed < seg.data_len)
                readed += FLASH_read(flash_location, dest + readed, seg.data_len - readed);
        }
        else if (SOC_IRAM_HIGH >= seg.load_addr && SOC_IRAM_LOW <= seg.load_addr)
        {
            uint8_t *dest = (uint8_t *)seg.load_addr;
            size_t readed = 0;

            while (readed < seg.data_len)
                readed += FLASH_read(flash_location, dest + readed, seg.data_len - readed);

            esp_rom_printf("this is a text.iram_attr section\n");
        }
        else if (SOC_DROM_HIGH >= seg.load_addr && SOC_DROM_LOW <= seg.load_addr)
        {
            ro_seg.location = flash_location;
            ro_seg.hdr = seg;
        }
        else if (SOC_IROM_HIGH >= seg.load_addr && SOC_IROM_LOW <= seg.load_addr)
        {
            text_seg.location = flash_location;
            text_seg.hdr = seg;
        }

        esp_rom_printf("\n");
        flash_location += seg.data_len;
    }
    cache_hal_disable(CACHE_TYPE_ALL);
    mmu_ll_unmap_all(0);

    if (ro_seg.location)
    {
        esp_rom_printf("map rodata: %p => %p: %d\n", ro_seg.location, ro_seg.hdr.load_addr, ro_seg.hdr.data_len);
        // CACHE_BUS_DBUS0
        // map rodata: 0x00010020 => 0x3c020020: 37104

        /*
        int pages = (ro_seg.hdr.data_len + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

        uint32_t entry_id = MMU_FLASH_ENTRY_ID(ro_seg.hdr.load_addr);
        uint32_t paddr = MMU_FLASH_PADDR(ro_seg.location);

        for (int i = 0; i < pages; i ++)
        {
            mmu_ll_write_entry(0, entry_id, paddr, MMU_TARGET_FLASH0);

            entry_id ++;
            paddr ++;
        }
        */
        FLASH_mmap(ro_seg.location, ro_seg.hdr.load_addr, ro_seg.hdr.data_len);
        cache_ll_l1_enable_bus(0, CACHE_BUS_DBUS0);
    }

    if (text_seg.location)
    {
        // CACHE_BUS_IBUS0
        esp_rom_printf("map text: %p => %p: %d\n", text_seg.location, text_seg.hdr.load_addr, text_seg.hdr.data_len);

        FLASH_mmap(text_seg.location, text_seg.hdr.load_addr, text_seg.hdr.data_len);
        cache_ll_l1_enable_bus(0, CACHE_BUS_IBUS0);
    }
    cache_hal_enable(CACHE_TYPE_ALL);

    /*
        drom_addr: 10020, drom_load_addr: 3c020020, drom_size: 37104
        irom_addr: 20020, irom_load_addr: 42000020, irom_size: 103416
    */

    //     cache_bus_mask_t bus_mask;

    // {
    //     cache_bus_mask_t bus_mask;
    //     uint32_t length;

    //     mmu_hal_map_region(0, MMU_TARGET_FLASH0,
    //         &_flash_rodata_end - &_flash_rodata_start,
    //         _flash_rodata_start,
    //         &_flash_rodata_end - &_flash_rodata_start,
    //         &length);

    //     esp_rom_printf("mapped: %u\n", length);
    // }

    if (false)
    {
kernel_load_error:
        esp_rom_delay_us(1000000);
        while(1) {}
    }

    typedef void __attribute__((noreturn)) (*entry_t)(void);
    (*(entry_t)hdr.entry_addr)();
}

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize)
{
    static uintptr_t current_paddr = (uintptr_t)-1;
    uint32_t paddr = MMU_FLASH_PADDR(flash_location);

    if (current_paddr != paddr)
    {
        current_paddr = paddr;
        cache_hal_disable(CACHE_TYPE_ALL);

        mmu_ll_write_entry(0, MMU_FLASH_READ_ENTRY_ID, paddr, MMU_TARGET_FLASH0);
        cache_hal_enable(CACHE_TYPE_ALL);
    }

    off_t offset = flash_location & (CONFIG_MMU_PAGE_SIZE - 1);
    void *src = (void *)(MMU_FLASH_READ_VADDR + offset);

    int bytes_remain = CONFIG_MMU_PAGE_SIZE - offset;
    bufsize = bufsize < bytes_remain ? bufsize : bytes_remain;

    esp_rom_printf("dst: %p, src: %p, size: %u\n", buf, src, bufsize);
    memcpy(buf, src, bufsize);
    return bufsize;
}

static void FLASH_mmap(uintptr_t flash_location, uintptr_t vaddr, size_t size)
{
    uint32_t entry_id = MMU_FLASH_ENTRY_ID(vaddr);
    uint32_t paddr = MMU_FLASH_PADDR(flash_location);
    int pages = ((size + (flash_location & (CONFIG_MMU_PAGE_SIZE - 1))) + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

    for (int i = 0; i < pages; i ++)
    {
        mmu_ll_write_entry(0, entry_id, paddr, MMU_TARGET_FLASH0);

        entry_id ++;
        paddr ++;
    }
}

static void WDT_config(void)
{
    /*
     * At this point, the flashboot protection of RWDT and MWDT0 will have been
     * automatically enabled. We can disable flashboot protection as it's not
     * needed anymore. If configured to do so, we also initialize the RWDT to
     * protect the remainder of the bootloader process.
     */

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

#ifdef CONFIG_BOOTLOADER_WDT_ENABLE
    wdt_hal_init(&rwdt_ctx, WDT_RWDT, 0, false);
    wdt_hal_write_protect_disable(&rwdt_ctx);

    wdt_hal_config_stage(&rwdt_ctx, WDT_STAGE0,
        (uint32_t)((uint64_t)CONFIG_BOOTLOADER_WDT_TIME_MS * rtc_clk_slow_freq_get_hz() / 1000),
        WDT_STAGE_ACTION_RESET_RTC
    );
    wdt_hal_enable(&rwdt_ctx);
    wdt_hal_write_protect_enable(&rwdt_ctx);
#endif

    // disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
    wdt_hal_write_protect_disable(&mwdt_ctx);
    wdt_hal_set_flashboot_en(&mwdt_ctx, false);
    wdt_hal_write_protect_enable(&mwdt_ctx);
}


esp_err_t process_image_header(esp_image_metadata_t *data, uint32_t part_offset)
{
    bzero(data, sizeof(esp_image_metadata_t));
    data->start_addr = part_offset;
    FLASH_read(data->start_addr, &data->image, sizeof(esp_image_header_t));

    data->image_len = sizeof(esp_image_header_t);
    return ESP_OK;
}

esp_err_t process_segments(esp_image_metadata_t *data)
{
    esp_err_t err = ESP_OK;
    uint32_t start_segments = data->start_addr + data->image_len;
    uint32_t next_addr = start_segments;

    for (int i = 0; i < data->image.segment_count; i++)
    {
        esp_image_segment_header_t *header = &data->segments[i];
        FLASH_read(next_addr, header, sizeof(esp_image_segment_header_t));

        next_addr += sizeof(esp_image_segment_header_t);
        data->segment_data[i] = next_addr;
        next_addr += header->data_len;
    }
    // Segments all loaded, verify length
    uint32_t end_addr = next_addr;

    data->image_len += end_addr - start_segments;

    for (int i = 0; i < data->image.segment_count; i++)
    {
        intptr_t load_addr = data->segments[i].load_addr;
        if (should_load(load_addr))
        {
            uint32_t data_len = data->segments[i].data_len;
            uint32_t data_addr = data->segment_data[i];

            process_segment_data(load_addr, data_addr, data_len);
        }
    }
    return err;
}

static esp_err_t process_segment_data(intptr_t load_addr, uint32_t data_addr, uint32_t data_len)
{
    const uint32_t *data = (const uint32_t *)bootloader_mmap(data_addr, data_len);
    esp_rom_printf("dest: %p, src: %p, size: %u\n", load_addr, data, data_len);

    memcpy((void *)load_addr, data, data_len);
    bootloader_munmap(data);
    return ESP_OK;
}

static bool should_map(uint32_t load_addr)
{
    return (load_addr >= SOC_IROM_LOW && load_addr < SOC_IROM_HIGH)
           || (load_addr >= SOC_DROM_LOW && load_addr < SOC_DROM_HIGH);
}

static bool should_load(uint32_t load_addr)
{
    /* Reload the RTC memory segments whenever a non-deepsleep reset
       is occurring */
    bool load_rtc_memory = esp_rom_get_reset_reason(0) != RESET_REASON_CORE_DEEP_SLEEP;

    if (should_map(load_addr)) {
        return false;
    }

    if (!load_rtc_memory) {
#if SOC_RTC_FAST_MEM_SUPPORTED
        if (load_addr >= SOC_RTC_IRAM_LOW && load_addr < SOC_RTC_IRAM_HIGH) {
            return false;
        }
        if (load_addr >= SOC_RTC_DRAM_LOW && load_addr < SOC_RTC_DRAM_HIGH) {
            return false;
        }
#endif

#if SOC_RTC_SLOW_MEM_SUPPORTED
        if (load_addr >= SOC_RTC_DATA_LOW && load_addr < SOC_RTC_DATA_HIGH) {
            return false;
        }
#endif
    }

    return true;
}
