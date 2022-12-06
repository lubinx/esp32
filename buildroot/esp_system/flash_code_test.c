#include "esp_rom_sys.h"

char const *foobar_text = "this is code from flash..\n";

void foobar(void)
{
    esp_rom_printf(foobar_text);
}

/*
    rodata starts from paddr=0x00010020, vaddr=0x3c020020, size=0x90f0
    mapping data: paddr=0x00010000 and vaddr=0x3c020000, 0x10000 bytes are mapped

    text starts from paddr=0x00020020, vaddr=0x42000020, size=0x193f8
    mapping text: starting from paddr=0x00020000 and vaddr=0x42000000, 0x20000 bytes are mapped
    start: 0x4037524c
*/
