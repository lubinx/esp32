#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "esp_private/esp_clk.h"

extern "C" void __attribute__((weak)) app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");
    printf("silicon revision v%d.%d\n", chip_info.revision / 100, chip_info.revision % 100);
    printf("Minimum free heap size: %d bytes\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf ("cpu frequency: %d\n", esp_clk_cpu_freq());

    printf("infinite loop...\n");
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    try
    {
        throw "foobar";
    }
    catch(char const *e)
    {
        esp_rom_printf("catched c++ exception: %s\n", e);
    }

    char *buf = (char *)malloc(20480);
    esp_rom_printf("malloc test allocate 20k buffer: %p\n", buf);
    free(buf);


    while (1)
    {
        pthread_mutex_lock(&mutex);
        esp_rom_printf("*");
        pthread_mutex_unlock(&mutex);

        msleep(1000);
    }
}
