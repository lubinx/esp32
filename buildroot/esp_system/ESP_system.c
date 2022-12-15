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
void IRAM_ATTR SystemInit(void)
{
    esp_rom_printf("SystemInit()\n");
    esp_rom_printf("TAG: %p\n", TAG);

    esp_rom_printf("n1 ptr: %p\n", &n1);
    esp_rom_printf("n2 ptr: %p\n", &n2);
    esp_rom_printf("n3 ptr: %p\n", &n3);
    esp_rom_printf("n4 ptr: %p\n", &n4);
    esp_rom_printf("n5 ptr: %p\n", &n5);
    esp_rom_printf("n6 ptr: %p\n", &n6);
    esp_rom_printf("n7 ptr: %p\n", &n7);
    esp_rom_printf("n8 ptr: %p\n", &n8);
    esp_rom_printf("n9 ptr: %p\n", &n9);
    esp_rom_printf("\n\n\n");
    esp_rom_delay_us(1000000);
    while (1) {}

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
