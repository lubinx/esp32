#include "sdkconfig.h"


#include "soc/efuse_struct.h"
#include "hal/efuse_ll.h"

#include "bootloader_flash.h"
#include "bootloader_flash_priv.h"

void bootloader_mspi_reset(void)
{
    #if CONFIG_IDF_TARGET_ESP32
        SPI1.slave.sync_reset = 0;
        SPI0.slave.sync_reset = 0;
        SPI1.slave.sync_reset = 1;
        SPI0.slave.sync_reset = 1;
        SPI1.slave.sync_reset = 0;
        SPI0.slave.sync_reset = 0;
    #else
        SPIMEM1.ctrl2.sync_reset = 0;
        SPIMEM0.ctrl2.sync_reset = 0;
        SPIMEM1.ctrl2.sync_reset = 1;
        SPIMEM0.ctrl2.sync_reset = 1;
        SPIMEM1.ctrl2.sync_reset = 0;
        SPIMEM0.ctrl2.sync_reset = 0;
    #endif
}

esp_err_t bootloader_flash_reset_chip(void)
{
    bootloader_mspi_reset();

    // Seems that sync_reset cannot make host totally idle.'
    // Sending an extra(useless) command to make the host idle in order to send reset command.
    bootloader_execute_flash_command(0x05, 0, 0, 0);

    #if CONFIG_IDF_TARGET_ESP32
        if (SPI1.ext2.st != 0)
            return ESP_FAIL
    #else
        if (!spimem_flash_ll_host_idle(&SPIMEM1))
            return ESP_FAIL;
    #endif

    bootloader_execute_flash_command(0x66, 0, 0, 0);
    bootloader_execute_flash_command(0x99, 0, 0, 0);
    return ESP_OK;
}

bool bootloader_flash_is_octal_mode_enabled(void)
{
    #if SOC_SPI_MEM_SUPPORT_OPI_MODE
        return efuse_ll_get_flash_type();
    #else
        return false;
    #endif
}

static uint32_t bootloader_flash_execute_command_common(uint8_t command, uint32_t addr_len, uint32_t address, uint8_t dummy_len,
    uint8_t mosi_len, uint32_t mosi_data, uint8_t miso_len)
{
    assert(mosi_len <= 32);
    assert(miso_len <= 32);

    uint32_t old_ctrl_reg = SPIMEM1.ctrl.val;
    uint32_t old_user_reg = SPIMEM1.user.val;
    uint32_t old_user1_reg = SPIMEM1.user1.val;

    #if CONFIG_IDF_TARGET_ESP32
        SPIMEM1.ctrl.val = SPI_WP_REG_M; // keep WP high while idle, otherwise leave DIO mode
    #else
        SPIMEM1.ctrl.val = SPI_MEM_WP_REG_M; // keep WP high while idle, otherwise leave DIO mode
    #endif

    //command phase
    SPIMEM1.user.usr_command = 1;
    SPIMEM1.user2.usr_command_bitlen = 7;
    SPIMEM1.user2.usr_command_value = command;
    //addr phase
    SPIMEM1.user.usr_addr = addr_len > 0;
    SPIMEM1.user1.usr_addr_bitlen = addr_len - 1;
    #if CONFIG_IDF_TARGET_ESP32
        SPIMEM1.addr = (addr_len > 0)? (address << (32-addr_len)) : 0;
    #else
        SPIMEM1.addr = address;
    #endif

    //dummy phase
    uint32_t total_dummy = dummy_len;
    if (miso_len > 0) {
        total_dummy += g_rom_spiflash_dummy_len_plus[1];
    }
    SPIMEM1.user.usr_dummy = total_dummy > 0;
    SPIMEM1.user1.usr_dummy_cyclelen = total_dummy - 1;
    //output data
    SPIMEM1.user.usr_mosi = mosi_len > 0;
    #if CONFIG_IDF_TARGET_ESP32
        SPIMEM1.mosi_dlen.usr_mosi_dbitlen = mosi_len ? (mosi_len - 1) : 0;
    #else
        SPIMEM1.mosi_dlen.usr_mosi_bit_len = mosi_len ? (mosi_len - 1) : 0;
    #endif

    SPIMEM1.data_buf[0] = mosi_data;
    //input data
    SPIMEM1.user.usr_miso = miso_len > 0;
    #if CONFIG_IDF_TARGET_ESP32
        SPIMEM1.miso_dlen.usr_miso_dbitlen = miso_len ? (miso_len - 1) : 0;
    #else
        SPIMEM1.miso_dlen.usr_miso_bit_len = miso_len ? (miso_len - 1) : 0;
    #endif

    SPIMEM1.cmd.usr = 1;
    while (SPIMEM1.cmd.usr != 0) {}
    SPIMEM1.ctrl.val = old_ctrl_reg;
    SPIMEM1.user.val = old_user_reg;
    SPIMEM1.user1.val = old_user1_reg;

    uint32_t ret = SPIMEM1.data_buf[0];
    if (miso_len < 32)
    {
        //set unused bits to 0
        ret &= ~(UINT32_MAX << miso_len);
    }
    return ret;
}

uint32_t bootloader_execute_flash_command(uint8_t command, uint32_t mosi_data, uint8_t mosi_len, uint8_t miso_len)
{
    const uint8_t addr_len = 0;
    const uint8_t address = 0;
    const uint8_t dummy_len = 0;

    return bootloader_flash_execute_command_common(command, addr_len, address, dummy_len, mosi_len, mosi_data, miso_len);
}
