#include "esp_attr.h"
#include "esp_rom_sys.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

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
void IRAM_ATTR ESP_system_startup(void)
{
    esp_rom_printf("ESP_system_startup\n\n\n\n");
    esp_rom_delay_us(1000000);

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

    call_start_cpu0();
}

__attribute__((weak))
void app_main(void)
{
    esp_rom_printf("infinite loop...\n");
    while (1);

    // // printf("infinite loop...\n");

    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);

    // esp_restart();
}