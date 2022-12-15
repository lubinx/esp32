#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "xt_utils.h"

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
#include "bootloader_mem.h"

#include "esp_app_format.h"

/****************************************************************************
 *  imports
*****************************************************************************/
extern intptr_t _bss_start;
extern intptr_t _bss_end;

/****************************************************************************
 *  consts
*****************************************************************************/
#define FLASH_READ_MMU_VADDR            (SOC_DROM_HIGH - CONFIG_MMU_PAGE_SIZE)

typedef void __attribute__((noreturn)) (*kernel_entry_t)(void);

/****************************************************************************
 *  local
*****************************************************************************/
static void bootloader_super_wdt_auto_feed(void);
static void bootloader_config_wdt(void);

static kernel_entry_t KERNEL_load(uintptr_t flash_location);

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize);
static void FLASH_map_region(uintptr_t flash_location, uintptr_t vaddr, size_t size);

/****************************************************************************
 *  exports
*****************************************************************************/
void __attribute__((noreturn)) Reset_Handler(void)
{
    #if XCHAL_ERRATUM_572
        uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
        WSR(MEMCTL, memctl);
    #endif

    // Enable WDT, BOR
    bootloader_ana_super_wdt_reset_config(true);
    bootloader_ana_bod_reset_config(true);
    bootloader_ana_clock_glitch_reset_config(false);

    bootloader_init_mem();
    bootloader_clock_configure();

    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    #ifdef CONFIG_EFUSE_VIRTUAL
        ESP_LOGW(TAG, "eFuse virtual mode is enabled. If Secure boot or Flash encryption is enabled then it does not provide any security. FOR TESTING ONLY!");
        #ifndef CONFIG_EFUSE_VIRTUAL_KEEP_IN_FLASH
            esp_efuse_init_virtual_mode_in_ram();
        #endif
    #endif

    cache_hal_init();

    mmu_ll_unmap_all(0);
    #if ! CONFIG_FREERTOS_UNICORE
        mmu_ll_unmap_all(1);
    #endif

    kernel_entry_t entry = KERNEL_load(0x10000);

    bootloader_super_wdt_auto_feed();
    bootloader_config_wdt();

    esp_rom_printf("### sp: %p\n", xt_utils_get_sp());
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
static void bootloader_super_wdt_auto_feed(void)
{
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
    REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, 0);
}

static void bootloader_config_wdt(void)
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


static kernel_entry_t KERNEL_load(uintptr_t flash_location)
{
    struct FLASH_segment
    {
        uintptr_t location;
        esp_image_segment_header_t hdr;
    };

    esp_image_header_t hdr;
    struct FLASH_segment ro_seg;
    struct FLASH_segment text_seg;

    FLASH_read(flash_location, &hdr, sizeof(hdr));
    flash_location += sizeof(hdr);

    if (ESP_IMAGE_HEADER_MAGIC != hdr.magic)
    {
        esp_rom_printf("error loading kernel: invalid image tag(magic) %x\n", hdr.magic);
        goto kernel_load_error;
    }

    for (int i = 0; i < hdr.segment_count; i ++)
    {
        esp_image_segment_header_t seg;

        FLASH_read(flash_location, &seg, sizeof(seg));
        flash_location += sizeof(seg);

        if (SOC_DROM_HIGH >= seg.load_addr && SOC_DROM_LOW <= seg.load_addr)
        {
            ro_seg.location = flash_location;
            ro_seg.hdr = seg;
        }
        else if (SOC_IROM_HIGH >= seg.load_addr && SOC_IROM_LOW <= seg.load_addr)
        {
            text_seg.location = flash_location;
            text_seg.hdr = seg;
        }
        else
        {
            uint8_t *dest = (uint8_t *)seg.load_addr;
            size_t readed = 0;

            while (readed < seg.data_len)
                readed += FLASH_read(flash_location + readed, dest + readed, seg.data_len - readed);
        }

        flash_location += seg.data_len;
    }
    cache_hal_disable(CACHE_TYPE_ALL);
    mmu_ll_unmap_all(0);

    if (ro_seg.location)
    {
        FLASH_map_region(ro_seg.location, ro_seg.hdr.load_addr, ro_seg.hdr.data_len);

        cache_ll_l1_enable_bus(0, CACHE_BUS_DBUS0);
        #if ! CONFIG_FREERTOS_UNICORE
            cache_ll_l1_enable_bus(1, CACHE_BUS_DBUS0);
        #endif
    }

    if (text_seg.location)
    {
        FLASH_map_region(text_seg.location, text_seg.hdr.load_addr, text_seg.hdr.data_len);

        cache_ll_l1_enable_bus(0, CACHE_BUS_IBUS0);
        #if ! CONFIG_FREERTOS_UNICORE
            cache_ll_l1_enable_bus(1, CACHE_BUS_IBUS0);
        #endif
    }
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

static void FLASH_map_region(uintptr_t flash_location, uintptr_t vaddr, size_t size)
{
    off_t offset = flash_location & (CONFIG_MMU_PAGE_SIZE - 1);
    int pages = (size + offset + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

    uint32_t entry_id = mmu_ll_get_entry_id(0, vaddr);
    uint32_t paddr = mmu_ll_format_paddr(0, flash_location);

    while (pages)
    {
        mmu_ll_write_entry(0, entry_id, paddr, MMU_TARGET_FLASH0);

        entry_id ++;
        paddr ++;
        pages --;
    }
}
