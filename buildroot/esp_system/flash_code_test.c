#include "esp_rom_sys.h"

char const *foobar_text = "this is code from flash..\n";

void foobar(void)
{
    esp_rom_printf(foobar_text);
}
