/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/time.h>
#include <sys/param.h>

#include "soc/syscon_reg.h"
#include "soc/system_reg.h"
#include "soc/soc.h"
#include "soc/rtc.h"
#include "soc/rtc_periph.h"
#include "driver/clk_tree.h"

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_cpu.h"
#include "esp_clk_internal.h"
#include "esp_rom_sys.h"
// #include "esp_private/periph_ctrl.h"
#include "esp_private/esp_clk.h"
#include "bootloader_clock.h"

static char const *TAG = "clk";

/* Number of cycles to wait from the 32k XTAL oscillator to consider it running.
 * Larger values increase startup delay. Smaller values may cause false positive
 * detection (i.e. oscillator runs for a few cycles and then stops).
 */
#define SLOW_CLK_CAL_CYCLES     CONFIG_RTC_CLK_CAL_CYCLES

#define RTC_XTAL_CAL_RETRY 1

 /* Indicates that this 32k oscillator gets input from external oscillator, rather
  * than a crystal.
  */
#define EXT_OSC_FLAG    BIT(3)

  /* This is almost the same as soc_rtc_slow_clk_src_t, except that we define
   * an extra enum member for the external 32k oscillator.
   * For convenience, lower 2 bits should correspond to soc_rtc_slow_clk_src_t values.
   */

void esp_clk_init(void)
{
}


/* This function is not exposed as an API at this point.
 * All peripheral clocks are default enabled after chip is powered on.
 * This function disables some peripheral clocks when cpu starts.
 * These peripheral clocks are enabled when the peripherals are initialized
 * and disabled when they are de-initialized.
 */
void esp_perip_clk_init(void)
{
    uint32_t common_perip_clk = SYSTEM_WDG_CLK_EN |
        SYSTEM_I2S0_CLK_EN |
    #if CONFIG_ESP_CONSOLE_UART_NUM != 0
        SYSTEM_UART_CLK_EN |
    #endif
    #if CONFIG_ESP_CONSOLE_UART_NUM != 1
        SYSTEM_UART1_CLK_EN |
    #endif
    #if CONFIG_ESP_CONSOLE_UART_NUM != 2
        SYSTEM_UART2_CLK_EN |
    #endif
        SYSTEM_USB_CLK_EN |
        SYSTEM_SPI2_CLK_EN |
        SYSTEM_I2C_EXT0_CLK_EN |
        SYSTEM_UHCI0_CLK_EN |
        SYSTEM_RMT_CLK_EN |
        SYSTEM_PCNT_CLK_EN |
        SYSTEM_LEDC_CLK_EN |
        SYSTEM_TIMERGROUP1_CLK_EN |
        SYSTEM_SPI3_CLK_EN |
        SYSTEM_SPI4_CLK_EN |
        SYSTEM_PWM0_CLK_EN |
        SYSTEM_TWAI_CLK_EN |
        SYSTEM_PWM1_CLK_EN |
        SYSTEM_I2S1_CLK_EN |
        SYSTEM_SPI2_DMA_CLK_EN |
        SYSTEM_SPI3_DMA_CLK_EN |
        SYSTEM_PWM2_CLK_EN |
        SYSTEM_PWM3_CLK_EN |
        0;
    uint32_t hwcrypto_perip_clk = SYSTEM_CRYPTO_AES_CLK_EN |
        SYSTEM_CRYPTO_SHA_CLK_EN |
        SYSTEM_CRYPTO_RSA_CLK_EN |
        0;
    uint32_t wifi_bt_sdio_clk = SYSTEM_WIFI_CLK_WIFI_EN |
        SYSTEM_WIFI_CLK_BT_EN_M |
        SYSTEM_WIFI_CLK_I2C_CLK_EN |
        SYSTEM_WIFI_CLK_UNUSED_BIT12 |
        SYSTEM_WIFI_CLK_SDIO_HOST_EN |
        0;
    uint32_t common_perip_clk1 = 0;

    //Reset the communication peripherals like I2C, SPI, UART, I2S and bring them to known state.
    common_perip_clk |= SYSTEM_I2S0_CLK_EN |
    #if CONFIG_ESP_CONSOLE_UART_NUM != 0
        SYSTEM_UART_CLK_EN |
    #endif
    #if CONFIG_ESP_CONSOLE_UART_NUM != 1
        SYSTEM_UART1_CLK_EN |
    #endif
    #if CONFIG_ESP_CONSOLE_UART_NUM != 2
        SYSTEM_UART2_CLK_EN |
    #endif
        SYSTEM_USB_CLK_EN |
        SYSTEM_SPI2_CLK_EN |
        SYSTEM_I2C_EXT0_CLK_EN |
        SYSTEM_UHCI0_CLK_EN |
        SYSTEM_RMT_CLK_EN |
        SYSTEM_UHCI1_CLK_EN |
        SYSTEM_SPI3_CLK_EN |
        SYSTEM_SPI4_CLK_EN |
        SYSTEM_I2C_EXT1_CLK_EN |
        SYSTEM_I2S1_CLK_EN |
        SYSTEM_SPI2_DMA_CLK_EN |
        SYSTEM_SPI3_DMA_CLK_EN;

    /* Disable some peripheral clocks. */
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, common_perip_clk);
    SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, common_perip_clk);

    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, common_perip_clk1);
    SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, common_perip_clk1);

    /* Disable hardware crypto clocks. */
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
    SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, hwcrypto_perip_clk);

    /* Force clear backup dma reset signal. This is a fix to the backup dma
     * implementation in the ROM, the reset signal was not cleared when the
     * backup dma was started, which caused the backup dma operation to fail. */
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_PERI_BACKUP_RST);

    /* Disable WiFi/BT/SDIO clocks. */
    CLEAR_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);
    SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);

    /* Set WiFi light sleep clock source to RTC slow clock */
    REG_SET_FIELD(SYSTEM_BT_LPCK_DIV_INT_REG, SYSTEM_BT_LPCK_DIV_NUM, 0);
    CLEAR_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_8M);
    SET_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_RTC_SLOW);

    /* Enable RNG clock. */
    periph_module_enable(PERIPH_RNG_MODULE);
}
