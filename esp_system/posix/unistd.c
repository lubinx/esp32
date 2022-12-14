#include <features.h>
#include <sys/types.h>

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
    vTaskDelay(seconds * 1000 / portTICK_PERIOD_MS);
    return 0;
}

int msleep(uint32_t msec)
{
    vTaskDelay(msec / portTICK_PERIOD_MS);
    return 0;
}

// TODO: move to pthread.c
int pthread_yield(void)
{
    msleep(0);
}

int pthread_setcancelstate(int state, int *oldstate)
{
    ARG_UNUSED(state, oldstate);
    return ENOSYS;
}

int pthread_setcanceltype(int type, int *old_type)
{
    ARG_UNUSED(type, old_type);
    return ENOSYS;
}
