#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int usleep(useconds_t us)
{
    const int us_per_tick = portTICK_PERIOD_MS * 1000;

    if (us > us_per_tick)
        vTaskDelay((us + us_per_tick - 1) / us_per_tick);
    else
        esp_rom_delay_us((uint32_t) us);

    return 0;
}

unsigned int sleep(unsigned int seconds)
{
    usleep(1000000UL * seconds);
    return 0;
}

int msleep(uint32_t msec)
{
    return usleep(1000UL * msec);
}
