#include <stdlib.h>
#include "uc/glist.h"

#include "esp_attr.h"
#include "esp_err.h"

#include "esp_system.h"

void IRAM_ATTR _exit(int status)
{
    esp_restart();
}

int atexit(void (*function)(void))
{
}

esp_err_t esp_register_shutdown_handler(void (*function)(void))
    __attribute__((alias("atexit")));

void IRAM_ATTR esp_restart(void)
{
    while (1);
}
