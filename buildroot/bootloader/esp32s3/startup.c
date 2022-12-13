#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

#include "soc/soc.h"
#include "hal/mmu_ll.h"
#include "hal/cache_hal.h"
#include "hal/wdt_hal.h"

// #include "hal/mpu_types.h"
#include "soc/soc_caps.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/ext_mem_defs.h"

#if CONFIG_IDF_TARGET_ESP32C6
    #include "soc/hp_apm_reg.h"
    #include "soc/lp_apm_reg.h"
    #include "soc/lp_apm0_reg.h"
#endif

#include "bootloader_soc.h"
#include "esp_app_format.h"


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

#define MMU_FLASH_ENTRY_ID(VADDR)       ((VADDR & MMU_VADDR_MASK) >> 16)
#define MMU_FLASH_PADDR(ADDR)           (ADDR >> 16)

#define MMU_FLASH_READ_VADDR            (SOC_DROM_HIGH - CONFIG_MMU_PAGE_SIZE)
#define MMU_FLASH_READ_ENTRY_ID         MMU_FLASH_ENTRY_ID(MMU_FLASH_READ_VADDR)

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize);

/****************************************************************************
 *  exports
*****************************************************************************/
void Reset_Handler(void)
{
#if XCHAL_ERRATUM_572
    uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;
    WSR(MEMCTL, memctl);
#endif // XCHAL_ERRATUM_572

    // Enable WDT, BOR, and GLITCH reset
    bootloader_ana_super_wdt_reset_config(true);
    bootloader_ana_bod_reset_config(true);
    bootloader_ana_clock_glitch_reset_config(true);

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

    mmu_ll_unmap_all(0);
    #if !CONFIG_FREERTOS_UNICORE
        mmu_ll_unmap_all(1);
    #endif

    WDT_config();
    KERNEL_load(0x10000);

/*
    if (ESP_OK != ret)
    {
// startup_failure:
        esp_rom_delay_us(2000000);
        esp_rom_software_reset_system();
    }
*/
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
static void KERNEL_load(uintptr_t flash_location)
{
    static esp_image_header_t hdr;
    struct FLASH_segment
    {
        uintptr_t location;
        esp_image_segment_header_t hdr;
    };
    static struct FLASH_segment ro;
    static struct FLASH_segment text;

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
            ro.location = flash_location;
            ro.hdr = seg;
        }
        else if (SOC_IROM_HIGH >= seg.load_addr && SOC_IROM_LOW <= seg.load_addr)
        {
            text.location = flash_location;
            text.hdr = seg;
        }

        esp_rom_printf("\n");
        flash_location +=  seg.data_len;
    }

    if (ro.location)
    {

    }

    if (text.location)
    {

    }

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

    if (true)
    {
kernel_load_error:
        while(1) {}
    }

    typedef void __attribute__((noreturn)) (*entry_t)(void);
    ((entry_t)hdr.entry_addr)();
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

    // Disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that there's not gap in WDT protection.
    wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
    wdt_hal_write_protect_disable(&mwdt_ctx);
    wdt_hal_set_flashboot_en(&mwdt_ctx, false);
    wdt_hal_write_protect_enable(&mwdt_ctx);
}

static ssize_t FLASH_read(uintptr_t flash_location, void *buf, size_t bufsize)
{
    static uintptr_t mmu_mapped_paddr = (uintptr_t)-1;
    // uintptr_t paddr = flash_location & ~(CONFIG_MMU_PAGE_SIZE - 1);
    uint32_t paddr = MMU_FLASH_PADDR(flash_location);

    if (mmu_mapped_paddr != paddr)
    {
        mmu_mapped_paddr = paddr;
        // cache_hal_disable(CACHE_TYPE_DATA);

        mmu_ll_write_entry(0, MMU_FLASH_READ_ENTRY_ID, paddr, MMU_TARGET_FLASH0);
        // cache_hal_enable(CACHE_TYPE_DATA);
    }

    off_t offset = flash_location & (CONFIG_MMU_PAGE_SIZE - 1);
    void *src = (void *)(MMU_FLASH_READ_VADDR + offset);

    int bytes_remain = CONFIG_MMU_PAGE_SIZE - offset;
    bufsize = bufsize < bytes_remain ? bufsize : bytes_remain;

    memcpy(buf, src, bufsize);
    return bufsize;
}
