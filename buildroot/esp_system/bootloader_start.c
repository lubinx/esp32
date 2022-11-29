#include "esp_attr.h"

#include "bootloader_init.h"
#include "bootloader_utility.h"

#include "esp_rom_sys.h"
// #include "bootloader_common.h"


/****************************************************************************
 *  imports
*****************************************************************************/
extern __attribute__((noreturn))
    void IRAM_ATTR call_start_cpu0(void);

/****************************************************************************
 *  exports
*****************************************************************************/
void IRAM_ATTR bootloader_startup(void)
{
    esp_rom_printf("infinite loop...\n");

    if (ESP_OK == bootloader_init())
        call_start_cpu0();  // init freerots => app_main()
    else
        bootloader_reset();
}

__attribute__((weak))
void app_main(void)
{
    esp_rom_printf("infinite loop...\n");
    // printf("infinite loop...\n");

    while (1)
    {
    }
}
