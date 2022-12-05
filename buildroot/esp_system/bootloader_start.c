#include "esp_attr.h"
#include "esp_rom_sys.h"

#include "bootloader_init.h"
#include "bootloader_utility.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_flash.h"

#include "soc/soc.h"

#include "hal/mmu_hal.h"
#include "hal/cache_hal.h"
#include "hal/cache_ll.h"


static const char *TAG = "boot";
static int n1 = 10;
static int n2 = 20;
static int n3 = 30;
static int n4 = 40;
static int n5 = 50;
static int n6 = 60;
static int n7 = 70;
static int n8 = 80;
static int n9 = 90;

/****************************************************************************
 *  imports
*****************************************************************************/
extern intptr_t _bss_start;
extern intptr_t _bss_end;
extern intptr_t _flash_rodata_start;
extern intptr_t _flash_rodata_end;
extern intptr_t _flash_text_start;
extern intptr_t _flash_text_end;

extern __attribute__((noreturn))
    void IRAM_ATTR call_start_cpu0(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void IRAM_ATTR bootloader_startup(void)
{
    esp_rom_printf("\n\n\n1. hello call_start_cpu0 \
        \ttag: %s\n\
        \tn1: %d\n\
        \tn2: %d\n\
        \tn3: %d\n\
        \tn4: %d\n\
        \tn5: %d\n\
        \tn6: %d\n\
        \tn7: %d\n\
        \tn8: %d\n\
        \tn9: %d\n",
        TAG, n1, n2, n3, n4, n5, n6, n7, n8, n9
    );

    // 1. Hardware initialization
    if (ESP_OK != bootloader_init())
        bootloader_reset();

    esp_rom_printf("\n\n\n2. hello call_start_cpu0: after bootloader_init()\n\n\n");

    // extern void foobar(void);
    // foobar();

    cache_hal_disable(CACHE_TYPE_ALL);

    /*
        drom_addr: 10020, drom_load_addr: 3c020020, drom_size: 37104
        irom_addr: 20020, irom_load_addr: 42000020, irom_size: 103416
    */

    {
        extern char const *foobar_text;
        esp_rom_printf("rom test addr: 0x%08x\n", foobar_text);

        uint32_t rodata_org = (uint32_t)&_flash_rodata_start - SOC_DROM_LOW;
        uint32_t rodata_size = (uint32_t)&_flash_rodata_end - (uint32_t)&_flash_rodata_start;

        esp_rom_printf("_flash_rodata_start: 0x%08x\n", &_flash_rodata_start);
        esp_rom_printf("_flash_rodata_end: 0x%08x\n", &_flash_rodata_end);
        esp_rom_printf("rodata_org: 0x%08x\n", rodata_org);
        esp_rom_printf("rodata_size: 0x%08x\n\n", rodata_size);

        esp_rom_printf("_flash_text_start: 0x%08x\n", &_flash_text_start);
        esp_rom_printf("_flash_text_end: 0x%08x\n", &_flash_text_end);

        // cache_bus_mask_t bus_mask;
        uint32_t actual_mapped_len = 0;

        mmu_hal_map_region(0, MMU_TARGET_FLASH0,
            0x3c020000,
            0,
            rodata_size,
            &actual_mapped_len
        );
        esp_rom_printf("mapped: %u\n", actual_mapped_len);

        cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, 0x3c020000, rodata_size);
        cache_ll_l1_enable_bus(0, bus_mask);

        cache_hal_enable(CACHE_TYPE_DATA);

        esp_rom_printf("testing...\n");
        esp_rom_printf(foobar_text);
        esp_rom_printf("end test\n\n\n");
    }

    while (1) {}

    // call_start_cpu0();
}

__attribute__((weak))
void app_main(void)
{
    // esp_rom_printf("infinite loop...\n");

    // // printf("infinite loop...\n");

    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);

    // esp_restart();
}
