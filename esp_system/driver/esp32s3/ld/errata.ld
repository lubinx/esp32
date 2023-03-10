/* missed in esp32s3.peripherals.ld */
PROVIDE(SYSTEM              = 0x600C0000);
PROVIDE(ASSIST_DEBUG        = 0x600CE000);
PROVIDE(EXTMEM              = 0x600C4000);
/*
PROVIDE(_gl obal_impure_ptr = 0x3fceffd0);
PROVIDE(syscall_table_ptr   = 0x3fceffd4);
*/

/* TODO: remove esp_rom PROVIDE functions */
PROVIDE(esp_rom_route_intr_matrix = intr_matrix_set);

PROVIDE(esp_rom_printf = ets_printf);
PROVIDE(esp_rom_delay_us = ets_delay_us);
PROVIDE(esp_rom_install_uart_printf = ets_install_uart_printf);

PROVIDE(esp_rom_regi2c_read = rom_i2c_readReg);
PROVIDE(esp_rom_regi2c_read_mask = rom_i2c_readReg_Mask);
PROVIDE(esp_rom_regi2c_write = rom_i2c_writeReg);
PROVIDE(esp_rom_regi2c_write_mask = rom_i2c_writeReg_Mask);


PROVIDE(esp_log_printf = ets_printf);
