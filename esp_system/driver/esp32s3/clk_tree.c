#include <sys/errno.h>
#include "esp_log.h"

#include "soc/soc_caps.h"
#include "soc/dport_access.h"
#include "soc/system_reg.h"
#include "soc/syscon_reg.h"
#include "soc/rtc_cntl_struct.h"

#include "clk_tree.h"
#include "sdkconfig.h"
#include "soc/rtc.h"

char const *TAG = "clktree";

/****************************************************************************
 * @def
 ****************************************************************************/
#define XTAL_FREQ                       (40000000ULL)
#define PLL_DIV_TO_80M_FREQ             (80000000ULL)
#define PLL_DIV_TO_160M_FREQ            (160000000ULL)
#define PLL_DIV_TO_240M_FREQ            (240000000ULL)

/****************************************************************************
 *  @implements: initialization
 ****************************************************************************/
void clk_tree_initialize(void)
{
    // TODO: set CPU clocks by sdkconfig.h
    //  WTF, why this is about RTC?
    rtc_cpu_freq_config_t old_config, new_config;
    rtc_clk_cpu_freq_get_config(&old_config);
    uint32_t const old_freq_mhz = old_config.freq_mhz;
    uint32_t const new_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

    if (rtc_clk_cpu_freq_mhz_to_config(new_freq_mhz, &new_config))
        rtc_clk_cpu_freq_set_config(&new_config);

    // Re calculate the ccount to make time calculation correct.
    // __set_CCOUNT((uint64_t)__get_CCOUNT() * new_freq_mhz / old_freq_mhz);

    /* Disable some peripheral clocks. */
    uint32_t gate = SYSTEM_WDG_CLK_EN |
        SYSTEM_I2S0_CLK_EN |
        // SYSTEM_UART_CLK_EN |
        SYSTEM_UART1_CLK_EN |
        SYSTEM_UART2_CLK_EN |
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
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, gate);
    SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, gate);

    gate = 0;
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, gate);
    SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, gate);

    /* Disable hardware crypto clocks. */
    gate = SYSTEM_CRYPTO_AES_CLK_EN |
        SYSTEM_CRYPTO_SHA_CLK_EN |
        SYSTEM_CRYPTO_RSA_CLK_EN |
        0;
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, gate);
    SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, gate);

    /* Force clear backup dma reset signal. This is a fix to the backup dma
     * implementation in the ROM, the reset signal was not cleared when the
     * backup dma was started, which caused the backup dma operation to fail. */
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_PERI_BACKUP_RST);

    /* Disable WiFi/BT/SDIO clocks. */
    gate = SYSTEM_WIFI_CLK_WIFI_EN |
        SYSTEM_WIFI_CLK_BT_EN_M |
        SYSTEM_WIFI_CLK_I2C_CLK_EN |
        SYSTEM_WIFI_CLK_UNUSED_BIT12 |
        SYSTEM_WIFI_CLK_SDIO_HOST_EN |
        0;

    CLEAR_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, gate);
    SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);

    /* Set WiFi light sleep clock source to RTC slow clock */
    REG_SET_FIELD(SYSTEM_BT_LPCK_DIV_INT_REG, SYSTEM_BT_LPCK_DIV_NUM, 0);
    CLEAR_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_8M);
    SET_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_RTC_SLOW);

    /* Enable RNG clock. */
    periph_module_enable(PERIPH_RNG_MODULE);
}

/****************************************************************************
 *  @implements: freertos/systimer.h for freertos.
 ****************************************************************************/
uint64_t systimer_ticks_to_us(uint64_t ticks)
{
    return ticks / 16;
}

uint64_t systimer_us_to_ticks(uint64_t us)
{
    return us * 16;
}

/****************************************************************************
 *  @implements: rc / external clocks, and pll configure
 ****************************************************************************/
static inline uint64_t clk_tree_rc_fast_freq(void)
{
    return SOC_CLK_RC_FAST_FREQ_APPROX;
}

static inline uint64_t clk_tree_rc_slow_freq(void)
{
    return SOC_CLK_RC_SLOW_FREQ_APPROX;
}

static inline uint64_t clk_tree_xtal_freq(void)
{
    return XTAL_FREQ;
}

static inline uint64_t clk_tree_xtal32k_freq(void)
{
    return SOC_CLK_XTAL32K_FREQ_APPROX;
}

uint64_t clk_tree_sclk_freq(soc_sclk_t sclk)
{
    switch(sclk)
    {
    case SOC_SCLK_INT_RC_FAST:
        return clk_tree_rc_fast_freq();
    case SOC_SCLK_INT_RC_SLOW:
        return clk_tree_rc_slow_freq();
    case SOC_SCLK_XTAL:
        return clk_tree_xtal_freq();
    case SOC_SCLK_XTAL32K:
        return clk_tree_xtal32k_freq();
    }
}

int clk_tree_pll_conf(soc_pll_freq_sel_t sel)
{
    if (1 < (unsigned)sel)
        return EINVAL;

    REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL, sel);
    return 0;
}

uint64_t clk_tree_pll_freq(void)
{
    // SYSTEM.cpu_per_conf.pll_freq_sel
    switch (REG_GET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL))
    {
    default:
        return 0;
    case 0:
        return SOC_PLL_320M_FREQ;
    case 1:
        return SOC_PLL_480M_FREQ;
    }
}

/****************************************************************************
 * cpu / systick / ahb / apb
 ****************************************************************************/
int clk_tree_cpu_conf(soc_cpu_sclk_sel_t sel, uint32_t div)
{
    if ((SOC_CPU_CLK_SRC_XTAL == sel || SOC_CPU_CLK_SRC_RC_FAST == sel) && 0x3FF < div)
        return EINVAL;

    soc_pll_freq_sel_t pll_freq_sel;
    if (SOC_CPU_CLK_SRC_PLL == sel)
    {
        // Table 7­3. CPU Clock Frequency
        pll_freq_sel = (soc_pll_freq_sel_t)(REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PLL_FREQ_SEL));
        if (SOC_PLL_320M == pll_freq_sel)
        {
            if (2 != div && 4 != div)
            {
                ESP_LOGW(TAG, "PLL 320M => CPU div possiable value is 2/4");
                return EINVAL;
            }
        }
        else    // 480M
        {
            if (2 == div)
                ESP_LOGW(TAG, "CPU working at 240M may not stable");

            if (6 != div && 3 != div && 2 != div)
            {
                ESP_LOGW(TAG, "PLL 480M => CPU div possiable value is 2/3/6");
                return EINVAL;
            }
        }
    }

    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT, 0);
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, SOC_CPU_CLK_SRC_RC_FAST);

    switch(sel)
    {
    case SOC_CPU_CLK_SRC_XTAL:
    case SOC_CPU_CLK_SRC_RC_FAST:
        REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT, div - 1);
        REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, sel);
        break;

    case SOC_CPU_CLK_SRC_PLL:
        // Table 7­3. CPU Clock Frequency
        if(SOC_PLL_320M == pll_freq_sel)
        {
            switch(div)
            {
            case 2:
                REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL, 1);
                break;
            case 4:
                REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL, 0);
                break;
            }
        }
        else
        {
            switch(div)
            {
            case 2:
                REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL, 2);
                break;
            case 3:
                REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL, 1);
                break;
            case 6:
                REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL, 0);
                break;
            }
        }
        break;
    }
    return 0;
}

int clk_tree_systick_conf(soc_systick_sclk_sel_t sel, uint32_t div)
{
    if (SYSTIMER_CLK_SRC_XTAL != sel)
        return EINVAL;
    else
        return 0;
}

static inline uint32_t clk_tree_cpu_divider(void)
{
    // SYSTEM.sysclk_conf.
    return REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT) + 1;
}

uint64_t clk_tree_cpu_freq(void)
{
    switch ((soc_cpu_sclk_sel_t)REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL))
    {
    default:
        return 0;

    //--- 40M -
    case SOC_CPU_CLK_SRC_XTAL:
        ESP_LOGW(TAG, "cpu %llu, div %lu", clk_tree_xtal_freq(), clk_tree_cpu_divider());
        return clk_tree_xtal_freq() / clk_tree_cpu_divider();
    case SOC_CPU_CLK_SRC_PLL:
        switch (REG_GET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL))
        {
    // --- 80M
        case 0:
            return PLL_DIV_TO_80M_FREQ;
    // --- 160M
        case 1:
            return PLL_DIV_TO_160M_FREQ;
        case 2:
    // --- 240M
            return PLL_DIV_TO_240M_FREQ;
        default:
            return 0;
        }

    // --- should be power up default
    case SOC_CPU_CLK_SRC_RC_FAST:
        return clk_tree_rc_fast_freq() / clk_tree_cpu_divider();
    }
}

uint64_t clk_tree_systick_freq(void)
{
    // TODO: somehow systick has no route for it, only SYSTIMER_CLK_SRC_XTAL?
    //  it fixed by XTAL / 2.5 in esp32s3
    return 16000000;
}

uint64_t clk_tree_ahb_freq(void)
{
    // ref table 7.5
    // AHB_CLK path is highly dependent on CPU_CLK path
    switch ((soc_cpu_sclk_sel_t)REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL))
    {
    // --- equal to cpu
    case SOC_CPU_CLK_SRC_XTAL:
        return clk_tree_xtal_freq() / clk_tree_cpu_divider();
    case SOC_CPU_CLK_SRC_RC_FAST:
        return clk_tree_rc_fast_freq() / clk_tree_cpu_divider();
    // ---

    case SOC_CPU_CLK_SRC_PLL:
        // AHB_CLK is a fixed value when CPU_CLK is clocked from PLL
        return PLL_DIV_TO_80M_FREQ;
    }
}

// esp32s3 ahb = apb
uint64_t clk_tree_apb_freq(void)
    __attribute__((alias(("clk_tree_ahb_freq"))));

/****************************************************************************
 *  rtc fast/slow clks
 ****************************************************************************/
int clk_tree_rtc_fast_conf(soc_rtc_fast_sclk_sel_t sel, uint32_t div)
{
    return ENOSYS;
}

int clk_tree_rtc_slow_conf(soc_rtc_slow_sclk_sel_t sel, uint32_t div)
{
    return ENOSYS;
}

uint64_t clk_tree_rtc_fast_freq(void)
{
    return 0; // clk_tree_lp_fast_get_freq_hz(precision);
}

uint64_t clk_tree_rtc_slow_freq(void)
{
    switch ((soc_rtc_slow_sclk_sel_t)RTCCNTL.clk_conf.ana_clk_rtc_sel) // REG_GET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ANA_CLK_RTC_SEL)
    {
    // TODO: divider
    case SOC_RTC_SLOW_CLK_SRC_RC_SLOW:
        return clk_tree_rc_slow_freq();
    case SOC_RTC_SLOW_CLK_SRC_XTAL32K:
        return clk_tree_xtal32k_freq();
    case SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256:
        return SOC_CLK_RC_FAST_D256_FREQ_APPROX;
    default:
        return 0;
    }
}

/****************************************************************************
 *  clk gate
 ****************************************************************************/
static inline uint32_t periph_ll_get_clk_en_mask(periph_module_t periph)
{
    switch (periph)
    {
    case PERIPH_SARADC_MODULE:
        return SYSTEM_APB_SARADC_CLK_EN;
    case PERIPH_RMT_MODULE:
        return SYSTEM_RMT_CLK_EN;
    case PERIPH_LEDC_MODULE:
        return SYSTEM_LEDC_CLK_EN;
    case PERIPH_UART0_MODULE:
        return SYSTEM_UART_CLK_EN;
    case PERIPH_UART1_MODULE:
        return SYSTEM_UART1_CLK_EN;
    case PERIPH_UART2_MODULE:
        return SYSTEM_UART2_CLK_EN;
    case PERIPH_USB_MODULE:
        return SYSTEM_USB_CLK_EN;
    case PERIPH_I2C0_MODULE:
        return SYSTEM_I2C_EXT0_CLK_EN;
    case PERIPH_I2C1_MODULE:
        return SYSTEM_I2C_EXT1_CLK_EN;
    case PERIPH_I2S0_MODULE:
        return SYSTEM_I2S0_CLK_EN;
    case PERIPH_I2S1_MODULE:
        return SYSTEM_I2S1_CLK_EN;
    case PERIPH_LCD_CAM_MODULE:
        return SYSTEM_LCD_CAM_CLK_EN;
    case PERIPH_TIMG0_MODULE:
        return SYSTEM_TIMERGROUP_CLK_EN;
    case PERIPH_TIMG1_MODULE:
        return SYSTEM_TIMERGROUP1_CLK_EN;
    case PERIPH_PWM0_MODULE:
        return SYSTEM_PWM0_CLK_EN;
    case PERIPH_PWM1_MODULE:
        return SYSTEM_PWM1_CLK_EN;
    case PERIPH_UHCI0_MODULE:
        return SYSTEM_UHCI0_CLK_EN;
    case PERIPH_UHCI1_MODULE:
        return SYSTEM_UHCI1_CLK_EN;
    case PERIPH_PCNT_MODULE:
        return SYSTEM_PCNT_CLK_EN;
    case PERIPH_SPI_MODULE:
        return SYSTEM_SPI01_CLK_EN;
    case PERIPH_SPI2_MODULE:
        return SYSTEM_SPI2_CLK_EN;
    case PERIPH_SPI3_MODULE:
        return SYSTEM_SPI3_CLK_EN;
    case PERIPH_SDMMC_MODULE:
        return SYSTEM_SDIO_HOST_CLK_EN;
    case PERIPH_TWAI_MODULE:
        return SYSTEM_TWAI_CLK_EN;
    case PERIPH_RNG_MODULE:
        return SYSTEM_WIFI_CLK_RNG_EN;
    case PERIPH_WIFI_MODULE:
        return SYSTEM_WIFI_CLK_WIFI_EN_M;
    case PERIPH_BT_MODULE:
        return SYSTEM_WIFI_CLK_BT_EN_M;
    case PERIPH_WIFI_BT_COMMON_MODULE:
        return SYSTEM_WIFI_CLK_WIFI_BT_COMMON_M;
    case PERIPH_BT_BASEBAND_MODULE:
        return SYSTEM_BT_BASEBAND_EN;
    case PERIPH_BT_LC_MODULE:
        return SYSTEM_BT_LC_EN;
    case PERIPH_SYSTIMER_MODULE:
        return SYSTEM_SYSTIMER_CLK_EN;
    case PERIPH_DEDIC_GPIO_MODULE:
        return SYSTEM_CLK_EN_DEDICATED_GPIO;
    case PERIPH_GDMA_MODULE:
        return SYSTEM_DMA_CLK_EN;
    case PERIPH_AES_MODULE:
        return SYSTEM_CRYPTO_AES_CLK_EN;
    case PERIPH_SHA_MODULE:
        return SYSTEM_CRYPTO_SHA_CLK_EN;
    case PERIPH_RSA_MODULE:
        return SYSTEM_CRYPTO_RSA_CLK_EN;
    case PERIPH_HMAC_MODULE:
        return SYSTEM_CRYPTO_HMAC_CLK_EN;
    case PERIPH_DS_MODULE:
        return SYSTEM_CRYPTO_DS_CLK_EN;
    default:
        return 0;
    }
}

static inline uint32_t periph_ll_get_rst_en_mask(periph_module_t periph)
{
    switch (periph)
    {
    case PERIPH_SARADC_MODULE:
        return SYSTEM_APB_SARADC_RST;
    case PERIPH_RMT_MODULE:
        return SYSTEM_RMT_RST;
    case PERIPH_LEDC_MODULE:
        return SYSTEM_LEDC_RST;
    case PERIPH_WIFI_MODULE:
        return SYSTEM_WIFIMAC_RST;
    case PERIPH_BT_MODULE:
        return  (SYSTEM_BTBB_RST | SYSTEM_BTBB_REG_RST | SYSTEM_RW_BTMAC_RST | SYSTEM_RW_BTLP_RST | SYSTEM_RW_BTMAC_REG_RST | SYSTEM_RW_BTLP_REG_RST);
    case PERIPH_UART0_MODULE:
        return SYSTEM_UART_RST;
    case PERIPH_UART1_MODULE:
        return SYSTEM_UART1_RST;
    case PERIPH_UART2_MODULE:
        return SYSTEM_UART2_RST;
    case PERIPH_USB_MODULE:
        return SYSTEM_USB_RST;
    case PERIPH_I2C0_MODULE:
        return SYSTEM_I2C_EXT0_RST;
    case PERIPH_I2C1_MODULE:
        return SYSTEM_I2C_EXT1_RST;
    case PERIPH_I2S0_MODULE:
        return SYSTEM_I2S0_RST;
    case PERIPH_I2S1_MODULE:
        return SYSTEM_I2S1_RST;
    case PERIPH_LCD_CAM_MODULE:
        return SYSTEM_LCD_CAM_RST;
    case PERIPH_TIMG0_MODULE:
        return SYSTEM_TIMERGROUP_RST;
    case PERIPH_TIMG1_MODULE:
        return SYSTEM_TIMERGROUP1_RST;
    case PERIPH_PWM0_MODULE:
        return SYSTEM_PWM0_RST;
    case PERIPH_PWM1_MODULE:
        return SYSTEM_PWM1_RST;
    case PERIPH_UHCI0_MODULE:
        return SYSTEM_UHCI0_RST;
    case PERIPH_UHCI1_MODULE:
        return SYSTEM_UHCI1_RST;
    case PERIPH_PCNT_MODULE:
        return SYSTEM_PCNT_RST;
    case PERIPH_SPI_MODULE:
        return SYSTEM_SPI01_RST;
    case PERIPH_SPI2_MODULE:
        return SYSTEM_SPI2_RST;
    case PERIPH_SPI3_MODULE:
        return SYSTEM_SPI3_RST;
    case PERIPH_SDMMC_MODULE:
        return SYSTEM_SDIO_HOST_RST;
    case PERIPH_TWAI_MODULE:
        return SYSTEM_TWAI_RST;
    case PERIPH_SYSTIMER_MODULE:
        return SYSTEM_SYSTIMER_RST;
    case PERIPH_DEDIC_GPIO_MODULE:
        return SYSTEM_RST_EN_DEDICATED_GPIO;
    case PERIPH_GDMA_MODULE:
        return SYSTEM_DMA_RST;
    case PERIPH_HMAC_MODULE:
        return SYSTEM_CRYPTO_HMAC_RST;
    case PERIPH_DS_MODULE:
        return SYSTEM_CRYPTO_DS_RST;
    case PERIPH_AES_MODULE:
        return SYSTEM_CRYPTO_AES_RST;
    case PERIPH_SHA_MODULE:
        return SYSTEM_CRYPTO_SHA_RST;
    case PERIPH_RSA_MODULE:
        return SYSTEM_CRYPTO_RSA_RST;
    default:
        return 0;
    }
}

static uint32_t periph_ll_get_clk_en_reg(periph_module_t periph)
{
    switch (periph)
    {
    case PERIPH_DEDIC_GPIO_MODULE:
        return SYSTEM_CPU_PERI_CLK_EN_REG;
    case PERIPH_RNG_MODULE:
    case PERIPH_WIFI_MODULE:
    case PERIPH_BT_MODULE:
    case PERIPH_WIFI_BT_COMMON_MODULE:
    case PERIPH_BT_BASEBAND_MODULE:
    case PERIPH_BT_LC_MODULE:
        return SYSTEM_WIFI_CLK_EN_REG;
    case PERIPH_UART2_MODULE:
    case PERIPH_SDMMC_MODULE:
    case PERIPH_LCD_CAM_MODULE:
    case PERIPH_GDMA_MODULE:
    case PERIPH_HMAC_MODULE:
    case PERIPH_DS_MODULE:
    case PERIPH_AES_MODULE:
    case PERIPH_SHA_MODULE:
    case PERIPH_RSA_MODULE:
        return SYSTEM_PERIP_CLK_EN1_REG;
    default:
        return SYSTEM_PERIP_CLK_EN0_REG;
    }
}

static uint32_t periph_ll_get_rst_en_reg(periph_module_t periph)
{
    switch (periph)
    {
    case PERIPH_DEDIC_GPIO_MODULE:
        return SYSTEM_CPU_PERI_RST_EN_REG;
    case PERIPH_RNG_MODULE:
    case PERIPH_WIFI_MODULE:
    case PERIPH_BT_MODULE:
    case PERIPH_WIFI_BT_COMMON_MODULE:
    case PERIPH_BT_BASEBAND_MODULE:
    case PERIPH_BT_LC_MODULE:
        return SYSTEM_CORE_RST_EN_REG;
    case PERIPH_UART2_MODULE:
    case PERIPH_SDMMC_MODULE:
    case PERIPH_LCD_CAM_MODULE:
    case PERIPH_GDMA_MODULE:
    case PERIPH_HMAC_MODULE:
    case PERIPH_DS_MODULE:
    case PERIPH_AES_MODULE:
    case PERIPH_SHA_MODULE:
    case PERIPH_RSA_MODULE:
        return SYSTEM_PERIP_RST_EN1_REG;
    default:
        return SYSTEM_PERIP_RST_EN0_REG;
    }
}

void clk_tree_module_enable(periph_module_t periph)
{
    DPORT_SET_PERI_REG_MASK(periph_ll_get_clk_en_reg(periph), periph_ll_get_clk_en_mask(periph));
    DPORT_CLEAR_PERI_REG_MASK(periph_ll_get_rst_en_reg(periph), periph_ll_get_rst_en_mask(periph));
}

void clk_tree_module_disable(periph_module_t periph)
{
    DPORT_CLEAR_PERI_REG_MASK(periph_ll_get_clk_en_reg(periph), periph_ll_get_clk_en_mask(periph));
    DPORT_SET_PERI_REG_MASK(periph_ll_get_rst_en_reg(periph), periph_ll_get_rst_en_mask(periph));
}

void clk_tree_module_reset(periph_module_t periph)
{
    DPORT_SET_PERI_REG_MASK(periph_ll_get_rst_en_reg(periph), periph_ll_get_rst_en_mask(periph));
    DPORT_CLEAR_PERI_REG_MASK(periph_ll_get_rst_en_reg(periph), periph_ll_get_rst_en_mask(periph));
}

bool clk_tree_module_is_enable(periph_module_t periph)
{
    return 0 != DPORT_GET_PERI_REG_MASK(periph_ll_get_clk_en_reg(periph), periph_ll_get_clk_en_mask(periph));
}
