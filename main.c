#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>

#include "esp_chip_info.h"

void __attribute__((weak)) app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

    printf("infinite loop...\n");
    while (1)
    {
        esp_rom_printf("*");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
