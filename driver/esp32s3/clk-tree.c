#include <sys/errno.h>
#include "esp_log.h"

#include "soc.h"
#include "clk-tree.h"
#include "regi2c_ctrl.h"

#include "soc/dport_access.h"
#include "sdkconfig.h"

static __attribute__((used)) char const *TAG = "clktree";

/****************************************************************************
 * @def
 ****************************************************************************/
// default xtal is 40M, using sdkconfig?
#define XTAL_FREQ                       XTAL_40M

#define PLL_320M_FREQ                   (320000000ULL)
#define PLL_480M_FREQ                   (480000000ULL)

#define PLL_DIV_TO_80M_FREQ             (80000000ULL)
#define PLL_DIV_TO_160M_FREQ            (160000000ULL)
#define PLL_DIV_TO_240M_FREQ            (240000000ULL)

/****************************************************************************
 *  @internal
 ****************************************************************************/
static void OSC_configure(void);

static uint32_t *periph_clk_en_reg(PERIPH_module_t periph);
static uint32_t *periph_rst_en_reg(PERIPH_module_t periph);
static uint32_t periph_clk_en_mask(PERIPH_module_t periph);
static uint32_t periph_rst_en_mask(PERIPH_module_t periph);

/**
 *  RC FAST is optional powered
 */
static unsigned RC_FAST_refcount;

/****************************************************************************
 * @implements hw/clk-tree.h
 ****************************************************************************/
void CLK_initialize(void)
{
    OSC_configure();

    uint32_t old_freq_mhz = (uint32_t)(CLK_cpu_freq() / _MHZ);
    if (XTAL_FREQ < CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * _MHZ)
    {
        switch (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
        {
        default:
            ESP_EARLY_LOGW(TAG, "unknown CPU frequency %uMhz, fallback to 80Mhz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
            goto fallthrough;

        fallthrough:
        case PLL_DIV_TO_80M_FREQ / _MHZ:
            CLK_pll_conf(PLL_FREQ_SEL_320M);
            CLK_cpu_conf(CPU_SCLK_SEL_PLL, 4);
            break;
        case PLL_DIV_TO_160M_FREQ / _MHZ:
            CLK_pll_conf(PLL_FREQ_SEL_320M);
            CLK_cpu_conf(CPU_SCLK_SEL_PLL, 2);
            break;
        case PLL_DIV_TO_240M_FREQ / _MHZ:
            CLK_pll_conf(PLL_FREQ_SEL_480M);
            CLK_cpu_conf(CPU_SCLK_SEL_PLL, 2);
            break;
        };
    }
    else
        CLK_cpu_conf(CPU_SCLK_SEL_XTAL, 1);

    if (old_freq_mhz != CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
    {
        // Re calculate the ccount to make time calculation correct.
        __set_CCOUNT((uint32_t)(__get_CCOUNT() * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ / old_freq_mhz));
    }

    uint32_t clk_en0 =
        SYSTEM_EFUSE_CLK_EN |
        // SYSTEM_LEDC_CLK_EN |
        // SYSTEM_TWAI_CLK_EN |
    // ADC
        // SYSTEM_APB_SARADC_CLK_EN |
        // SYSTEM_ADC2_ARB_CLK_EN |
    // I2S
        // SYSTEM_I2S0_CLK_EN |
        // SYSTEM_I2S1_CLK_EN |
    // PWM
        // SYSTEM_PWM0_CLK_EN |
        // SYSTEM_PWM1_CLK_EN |
        // SYSTEM_PWM2_CLK_EN |
        // SYSTEM_PWM3_CLK_EN |
    // I2C
        // SYSTEM_I2C_EXT0_CLK_EN |
        // SYSTEM_I2C_EXT1_CLK_EN |
    // SPI
        SYSTEM_SPI01_CLK_EN |       // SPI0 is required by internal-flash
        // SYSTEM_SPI2_CLK_EN |
        // SYSTEM_SPI3_CLK_EN |
        // SYSTEM_SPI4_CLK_EN |
        // SYSTEM_SPI2_DMA_CLK_EN |
        // SYSTEM_SPI3_DMA_CLK_EN |
    // UART
        SYSTEM_UART_MEM_CLK_EN |
        SYSTEM_UART_CLK_EN |
        // SYSTEM_UART1_CLK_EN |
    // USB
        // SYSTEM_USB_CLK_EN |
        // SYSTEM_UHCI0_CLK_EN |
        // SYSTEM_UHCI1_CLK_EN |
    // TIMERS
        // SYSTEM_SYSTIMER_CLK_EN |
        // SYSTEM_TIMERS_CLK_EN |
        // SYSTEM_TIMERGROUP1_CLK_EN |
        // SYSTEM_TIMERGROUP_CLK_EN |
        // SYSTEM_RMT_CLK_EN |
        // SYSTEM_WDG_CLK_EN |
        0;
    uint32_t clk_en1 =
        // SYSTEM_USB_DEVICE_CLK_EN |
        // SYSTEM_UART2_CLK_EN |
        // SYSTEM_LCD_CAM_CLK_EN |
        // SYSTEM_SDIO_HOST_CLK_EN |
        // SYSTEM_DMA_CLK_EN |
        // SYSTEM_CRYPTO_HMAC_CLK_EN |
        // SYSTEM_CRYPTO_DS_CLK_EN |
        // SYSTEM_CRYPTO_RSA_CLK_EN |
        // SYSTEM_CRYPTO_SHA_CLK_EN |
        // SYSTEM_CRYPTO_AES_CLK_EN |
        SYSTEM_PERI_BACKUP_CLK_EN |
        0;

    SYSTEM.perip_clk_en0.val = clk_en0, SYSTEM.perip_rst_en0.val = ~clk_en0;
    SYSTEM.perip_clk_en1.val = clk_en1, SYSTEM.perip_rst_en1.val = ~clk_en1;
    // GPIO
    SYSTEM.cpu_peri_clk_en.clk_en_dedicated_gpio = 1;

    // WiFi/BT/SDIO/misc
    uint32_t wifi_bt_clkl_dis = SYSTEM_WIFI_CLK_WIFI_EN |
        SYSTEM_WIFI_CLK_BT_EN |
        SYSTEM_WIFI_CLK_I2C_CLK_EN |
        SYSTEM_WIFI_CLK_UNUSED_BIT12 |
        SYSTEM_WIFI_CLK_SDIO_HOST_EN |
        0;
    SYSCON.wifi_clk_en &= ~wifi_bt_clkl_dis;
    SYSCON.wifi_clk_en |= SYSTEM_WIFI_CLK_EN;

    /* reset WiFi low-power clock to RTC slow clock */
    SYSTEM.bt_lpck_div_int.bt_lpck_div_num = 0;
    SYSTEM.bt_lpck_div_frac.lpclk_sel_8m = 0;
    SYSTEM.bt_lpck_div_frac.lpclk_sel_rtc_slow = 1;

    // default RC SLOW to driven RTC
    RTCCNTL.clk_conf.ana_clk_rtc_sel = RTC_SCLK_SEL_RC_SLOW;
    // default XTAL/2 to driven RTC FAST
    RTCCNTL.clk_conf.fast_clk_rtc_sel = RTC_FAST_SCLK_SEL_XTAL_D2;

    // TODO: freertos using SYSTIMER, remove this later
    CLK_periph_enable(PERIPH_SYSTIMER_MODULE);
}

static void OSC_configure(void)
{
    // TODO: use sdkconfig to enable/disable XTAL 32k, default powerup for dev board
    // IO
    RTCIO.xtal_32p_pad.x32p_mux_sel = 1;
    RTCIO.xtal_32n_pad.x32n_mux_sel = 1;

    // configure: this is default from esp-idf code
    //  TODO: using sdkconfig to configure xtal 32k parameters
    RTCCNTL.ext_xtl_conf.dac_xtal_32k = 3;
    RTCCNTL.ext_xtl_conf.dres_xtal_32k = 3;
    RTCCNTL.ext_xtl_conf.dgm_xtal_32k = 3;
    RTCCNTL.ext_xtl_conf.dbuf_xtal_32k = 1;
    // xpd status, meaning?
    RTCCNTL.ext_xtl_conf.xpd_xtal_32k = RTC_CNTL_XPD_XTAL_32K_V;

    // soo..XTAL mode selection not found in esp-idf code?
    enum XTAL32K_mode_t
    {
        XTAL32K_CRYSTAL_MODE        = 0,
        XTAL32K_EXTERNAL_MODE,
        XTAL32K_BOOTSTRAP_MODE
    };

    RTCCNTL.clk_conf.dig_xtal32k_en = 1;
}

int CLK_pll_conf(enum PLL_freq_sel_t sel)
{
    RTCCNTL.options0.val = RTCCNTL.options0.val & ~(
        RTC_CNTL_BB_I2C_FORCE_PD |
        RTC_CNTL_BBPLL_FORCE_PD |
        RTC_CNTL_BBPLL_I2C_FORCE_PD |
        0);
    // force power up
    RTCCNTL.options0.val = RTCCNTL.options0.val | (
        RTC_CNTL_BBPLL_FORCE_PU |
        RTC_CNTL_BBPLL_I2C_FORCE_PU |
        RTC_CNTL_BB_I2C_FORCE_PU
    );

    REGI2C_bbpll_enable();
    REGI2C_bbpll_calibration_start();

    uint8_t div_ref;
    uint8_t div7_0;
    uint8_t dr1;
    uint8_t dr3;
    uint8_t dchgp;
    uint8_t dcur;
    uint8_t dbias;

    if (PLL_FREQ_SEL_480M == sel)
    {
        switch (XTAL_FREQ)
        {
        default:
            ESP_EARLY_LOGW(TAG, "unkonwn XTAL frequency: %u, assume is 40M", XTAL_FREQ);
            goto fallthrough_1;

        fallthrough_1:
        case XTAL_40M:
            div_ref = 0;
            div7_0 = 8;
            dr1 = 0;
            dr3 = 0;
            dchgp = 5;
            dcur = 3;
            dbias = 2;
            break;
        case XTAL_32M:
            div_ref = 1;
            div7_0 = 26;
            dr1 = 1;
            dr3 = 1;
            dchgp = 4;
            dcur = 0;
            dbias = 2;
            break;
        }
        REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_MODE_HF, 0x6B);
    }
    else    // PLL_FREQ_SEL_320M
    {
        switch (XTAL_FREQ)
        {
        default:
            ESP_EARLY_LOGW(TAG, "unkonwn XTAL frequency: %u, assume is 40M", XTAL_FREQ);
            goto fallthrough_2;

        fallthrough_2:
        case XTAL_40M:
            div_ref = 0;
            div7_0 = 4;
            dr1 = 0;
            dr3 = 0;
            dchgp = 5;
            dcur = 3;
            dbias = 2;
            break;
        case XTAL_32M:
            div_ref = 1;
            div7_0 = 6;
            dr1 = 0;
            dr3 = 0;
            dchgp = 5;
            dcur = 3;
            dbias = 2;
            break;
        }
        REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_MODE_HF, 0x69);
    }
    uint8_t i2c_bbpll_lref  = (dchgp << I2C_BBPLL_OC_DCHGP_LSB) | (div_ref);
    uint8_t i2c_bbpll_div_7_0 = div7_0;
    uint8_t i2c_bbpll_dcur = (1 << I2C_BBPLL_OC_DLREF_SEL_LSB ) | (3 << I2C_BBPLL_OC_DHREF_SEL_LSB) | dcur;

    REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_OC_REF_DIV, i2c_bbpll_lref);
    REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_OC_DIV_7_0, i2c_bbpll_div_7_0);
    REGI2C_WRITE_MASK(I2C_BBPLL, I2C_BBPLL_OC_DR1, dr1);
    REGI2C_WRITE_MASK(I2C_BBPLL, I2C_BBPLL_OC_DR3, dr3);
    REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_OC_DCUR, i2c_bbpll_dcur);
    REGI2C_WRITE_MASK(I2C_BBPLL, I2C_BBPLL_OC_VCO_DBIAS, dbias);

    REGI2C_bbpll_calibration_end();
    SYSTEM.cpu_per_conf.pll_freq_sel = sel;

    RTCCNTL.options0.val = RTCCNTL.options0.val & ~(
        RTC_CNTL_BBPLL_FORCE_PU |
        RTC_CNTL_BBPLL_I2C_FORCE_PU |
        RTC_CNTL_BB_I2C_FORCE_PU
    );
    return 0;
}

uint32_t CLK_pll_freq(void)
{
    switch (SYSTEM.cpu_per_conf.pll_freq_sel)
    {
    case 0:
        return PLL_320M_FREQ;
    case 1:
        return PLL_480M_FREQ;
    default:
        return 0;
    }
}

int CLK_cpu_conf(enum CPU_sclk_sel_t sel, uint32_t div)
{
    if (0 == div)       // div 0 is truly mistake by code, no fallback
        return EINVAL;

    if (CPU_SCLK_SEL_XTAL == sel && MINIAL_CPU_WORK_FREQ > XTAL_FREQ / div)
    {
        ESP_EARLY_LOGW(TAG, "invalid DIV(%u) for XTAL, fallback to %u Mhz", (unsigned)div, (unsigned)(MINIAL_CPU_WORK_FREQ / _MHZ));
        div = XTAL_FREQ / MINIAL_CPU_WORK_FREQ;
    }
    if (CPU_SCLK_SEL_RC_FAST == sel && MINIAL_CPU_WORK_FREQ > RC_FAST_FREQ / div)
    {
        ESP_EARLY_LOGW(TAG, "invalid DIV(%u) for RC FAST, fallback to %u Mhz", (unsigned)div, (unsigned)(MINIAL_CPU_WORK_FREQ / _MHZ));
        div = RC_FAST_FREQ / MINIAL_CPU_WORK_FREQ;
    }

    if (CPU_SCLK_SEL_RC_FAST == sel)
        CLK_SCLK_RC_FAST_ref();

    enum CPU_sclk_sel_t old_sel = SYSTEM.sysclk_conf.soc_clk_sel;

    /// @ref Table 7-2 / Table 7-3
    switch (sel)
    {
    case CPU_SCLK_SEL_PLL:
        // temporay switch back to XTAL (power up default)
        if (CPU_SCLK_SEL_PLL == SYSTEM.sysclk_conf.pre_div_cnt)
        {
            SYSTEM.sysclk_conf.pre_div_cnt = 1;
            SYSTEM.sysclk_conf.soc_clk_sel = CPU_SCLK_SEL_XTAL;
        }

        if (PLL_FREQ_SEL_320M == SYSTEM.cpu_per_conf.pll_freq_sel)
        {
            switch(div)
            {
            default:
                ESP_EARLY_LOGW(TAG, "invalid DIV(%u) for PLL 320M => CPU div possiable value is 2/4, fallback to 80 Mhz", (unsigned)div);
                goto fallthrough_4;

            fallthrough_4:
            case 4:
                SYSTEM.cpu_per_conf.cpuperiod_sel = 0;
                break;
            case 2:
                SYSTEM.cpu_per_conf.cpuperiod_sel = 1;
                break;
            }
        }
        else        // 480M
        {
            switch(div)
            {
            default:
                ESP_EARLY_LOGW(TAG, "invalid DIV(%u) for PLL 480M => CPU div possiable value is 2/3/6, fallback to 80 Mhz", (unsigned)div);
                goto fallthrough_6;

            fallthrough_6:
            case 6:
                SYSTEM.cpu_per_conf.cpuperiod_sel = 0;
                break;
            case 3:
                SYSTEM.cpu_per_conf.cpuperiod_sel = 1;
                break;
            case 2:
                SYSTEM.cpu_per_conf.cpuperiod_sel = 2;
                break;
            }
        }
        SYSTEM.sysclk_conf.soc_clk_sel = CPU_SCLK_SEL_PLL;
        break;

    default:
    case CPU_SCLK_SEL_RC_FAST:
    case CPU_SCLK_SEL_XTAL:
        SYSTEM.sysclk_conf.pre_div_cnt = BIT_WIDTH_OF(10, div - 1);
        SYSTEM.sysclk_conf.soc_clk_sel = sel;
        break;
    }

    if (CPU_SCLK_SEL_RC_FAST == old_sel)
        CLK_SCLK_RC_FAST_release();

    return 0;
}

uint32_t CLK_cpu_freq(void)
{
    /// @ref Table 7-2 / Table 7-3
    switch (SYSTEM.sysclk_conf.soc_clk_sel)
    {
    case CPU_SCLK_SEL_RC_FAST:
        return RC_FAST_FREQ / (SYSTEM.sysclk_conf.pre_div_cnt + 1);
    case CPU_SCLK_SEL_XTAL:
        return XTAL_FREQ / (SYSTEM.sysclk_conf.pre_div_cnt + 1);
    case CPU_SCLK_SEL_PLL:
        switch (SYSTEM.cpu_per_conf.cpuperiod_sel)
        {
        case 0:     // 80M
            return PLL_DIV_TO_80M_FREQ;
        case 1:     // 160M
            return PLL_DIV_TO_160M_FREQ;
        case 2:     // 240M
            return PLL_DIV_TO_240M_FREQ;
        }
    }
    /// TODO: log error?
    return 0;
}

int CLK_systimer_conf(enum SYSTIMER_sclk_sel_t sel)
{
    if (SYSTIMER_SCLK_SEL_XTAL != sel)
        return EINVAL;
    else
        return 0;
}

uint32_t CLK_systimer_freq(void)
{
    //  somehow it fixed by XTAL_FREQ / 2.5 in esp32s3
    return 16000000;
}

uint32_t CLK_ahb_freq(void)
{
    switch (SYSTEM.sysclk_conf.soc_clk_sel)     // ref table 7-5
    {
    // AHB_CLK path is highly dependent on CPU_CLK path
    case CPU_SCLK_SEL_XTAL:
        return XTAL_FREQ / (SYSTEM.sysclk_conf.pre_div_cnt + 1);
    case CPU_SCLK_SEL_RC_FAST:
        return RC_FAST_FREQ / (SYSTEM.sysclk_conf.pre_div_cnt + 1);
    // AHB_CLK is a fixed value when CPU_CLK is clocked from PLL
    case CPU_SCLK_SEL_PLL:
        return PLL_DIV_TO_80M_FREQ;
    default:
        return 0;
    }
}

// esp32s3 ahb = apb
uint32_t CLK_apb_freq(void)
    __attribute__((alias(("CLK_ahb_freq"))));

int CLK_rtc_conf(enum RTC_sclk_sel_t sel)
{
    /**
     *  NOTE: keep in mind RTC counter is 48bits, count year = pow(2, 48) / ticks / 86400 / 365
     *  to make things easier
     *      .div RC SLOW to 34000, closer to XTAL(32768)
     *      .due to RC FAST can't div more without remain, its keep 78125 ticks, which only count for 114 year, this kind enough
     */
    switch (sel)
    {
    case RTC_SCLK_SEL_RC_SLOW:              // 34000 ticks (136000 / 4), 262 year
        RTCCNTL.slow_clk_conf.rtc_ana_clk_div_vld = 1;
        RTCCNTL.slow_clk_conf.rtc_ana_clk_div = (4 -1);
        break;
    case RTC_SLOW_SCLK_SEL_RC_FAST_D256:    // 78125 ticks, 114 year
        CLK_SCLK_RC_FAST_ref();
        RTCCNTL.slow_clk_conf.rtc_ana_clk_div_vld = 0;
        break;
    case RTC_SCLK_SEL_XTAL32K:              // 32768 ticks, 272 year
        RTCCNTL.slow_clk_conf.rtc_ana_clk_div_vld = 0;
        break;
    }

    enum RTC_sclk_sel_t old_sel = RTCCNTL.clk_conf.ana_clk_rtc_sel;
    RTCCNTL.clk_conf.ana_clk_rtc_sel = sel;

    if (RTC_SLOW_SCLK_SEL_RC_FAST_D256 == old_sel)
        CLK_SCLK_RC_FAST_release();

    return 0;
}

uint32_t CLK_rtc_freq(void)
{
    unsigned div;

    if (RTCCNTL.slow_clk_conf.rtc_ana_clk_div_vld)
        div = 1;
    else
        div = RTCCNTL.slow_clk_conf.rtc_ana_clk_div + 1;

    switch (RTCCNTL.clk_conf.ana_clk_rtc_sel)
    {
    case RTC_SCLK_SEL_RC_SLOW:
        return RC_SLOW_FREQ / div;
    case RTC_SCLK_SEL_XTAL32K:
        return XTAL32K_FREQ / div;
    case RTC_SLOW_SCLK_SEL_RC_FAST_D256:
        return RC_FAST_D256_FREQ / div;
    default:
        return 0;
    }
}

/****************************************************************************
 * @implements esp32s3/clk-tree.h
 ****************************************************************************/
unsigned CLK_SCLK_RC_FAST_ref(void)
{
    unsigned refcount = __sync_add_and_fetch(&RC_FAST_refcount, 1);

    if (1 == refcount)
    {
        RTCCNTL.clk_conf.enb_ck8m = 0;
        RTCCNTL.clk_conf.dig_clk8m_en = 1;
        RTCCNTL.timer1.ck8m_wait = 5;

        // default div
        RTCCNTL.clk_conf.ck8m_div_sel_vld = 0;
        RTCCNTL.clk_conf.ck8m_div = 0;
    }
    return refcount;
}

unsigned CLK_SCLK_RC_FAST_release(void)
{
    unsigned refcount = __sync_sub_and_fetch(&RC_FAST_refcount, 1);

    if (0 == refcount)
    {
        RTCCNTL.clk_conf.enb_ck8m = 1;
    }
    return refcount;
}

int CLK_rtc_fast_sclk_sel(enum RTC_FAST_sclk_sel_t sel, uint32_t div)
{
    ARG_UNUSED(div);

    if (RTC_FAST_SCLK_SEL_RC_FAST == sel)
        CLK_SCLK_RC_FAST_ref();

    enum RTC_FAST_sclk_sel_t old_sel = RTCCNTL.clk_conf.fast_clk_rtc_sel;
    RTCCNTL.clk_conf.fast_clk_rtc_sel = sel;

    if (RTC_FAST_SCLK_SEL_RC_FAST == old_sel)
        CLK_SCLK_RC_FAST_release();
    return 0;
}

uint32_t CLK_rtc_fast_sclk_freq(void)
{
    switch (RTCCNTL.clk_conf.fast_clk_rtc_sel)
    {
    case RTC_FAST_SCLK_SEL_XTAL_D2:
        return XTAL_D2_FREQ;
    case RTC_FAST_SCLK_SEL_RC_FAST:
        return RC_FAST_FREQ;
    default:
        return 0;
    }
}

/**
 *  TODO: uart source clocks updating & updated override is not a good way, what about all other clocks update?
*/
__attribute__((weak))
void CLK_uart_sclk_updating(uart_dev_t *dev)
{
    ARG_UNUSED(dev);
}

__attribute__((weak))
void CLK_uart_sclk_updated(uart_dev_t *dev)
{
    ARG_UNUSED(dev);
}

int CLK_uart_sclk_sel(uart_dev_t *dev, enum UART_sclk_sel_t sel)
{
    if (sel != dev->clk_conf.sclk_sel)
    {
        CLK_uart_sclk_updating(dev);

        if (UART_SCLK_SEL_RC_FAST == sel)
            CLK_SCLK_RC_FAST_ref();

        enum UART_sclk_sel_t old_sel = dev->clk_conf.sclk_sel;
        dev->clk_conf.sclk_sel = sel;

        if (UART_SCLK_SEL_RC_FAST == old_sel)
            CLK_SCLK_RC_FAST_release();

        CLK_uart_sclk_updated(dev);
    }
    return 0;
}

uint32_t CLK_uart_sclk_freq(uart_dev_t *dev)
{
    switch(dev->clk_conf.sclk_sel)
    {
    case UART_SCLK_SEL_APB:
        return CLK_apb_freq();
    case UART_SCLK_SEL_RC_FAST:
        return RC_FAST_FREQ;
    case UART_SCLK_SEL_XTAL:
        return XTAL_FREQ;
    default:
        return 0;
    }
}

int CLK_i2c_sclk_sel(i2c_dev_t *dev, enum I2C_sclk_sel_t sel)
{
    if (sel != dev->clk_conf.sclk_sel)
    {
        if (I2C_SCLK_SEL_RC_FAST == sel)
            CLK_SCLK_RC_FAST_ref();

        enum I2C_sclk_sel_t old_sel = dev->clk_conf.sclk_sel;
        dev->clk_conf.sclk_sel = sel;

        if (I2C_SCLK_SEL_RC_FAST == old_sel)
            CLK_SCLK_RC_FAST_release();
    }
    return 0;
}

uint32_t CLK_i2c_sclk_freq(i2c_dev_t *dev)
{
    switch (dev->clk_conf.sclk_sel)
    {
    case I2C_SCLK_SEL_XTAL:
        return XTAL_FREQ;
    case I2C_SCLK_SEL_RC_FAST:
        return RC_FAST_FREQ;
    default:
        return 0;
    }
}

int CLK_i2s_rx_sclk_sel(i2s_dev_t *dev, enum I2S_sclk_sel_t sel)
{
    dev->rx_clkm_conf.rx_clk_sel = sel;
    return 0;
}

uint64_t CLK_i2s_rx_sclk_freq(i2s_dev_t *dev)
{
    switch (dev->rx_clkm_conf.rx_clk_sel)
    {
    case I2S_SCLK_SEL_XTAL:
        return XTAL_FREQ;
    case I2S_SCLK_SEL_PLL_D2:
        return CLK_pll_freq() / 2;
    case I2S_SCLK_SEL_PLL_F160M:
        return PLL_DIV_TO_160M_FREQ;
    default:
        return 0;
    }
}

int CLK_i2s_tx_sclk_sel(i2s_dev_t *dev, enum I2S_sclk_sel_t sel)
{
    dev->tx_clkm_conf.tx_clk_sel = sel;
    return 0;
}

uint64_t CLK_i2s_tx_sclk_freq(i2s_dev_t *dev)
{
    switch (dev->tx_clkm_conf.tx_clk_sel)
    {
    case I2S_SCLK_SEL_XTAL:
        return XTAL_FREQ;
    case I2S_SCLK_SEL_PLL_D2:
        return CLK_pll_freq() / 2;
    case I2S_SCLK_SEL_PLL_F160M:
        return PLL_DIV_TO_160M_FREQ;
    default:
        return 0;
    }
}

/****************************************************************************
 *  @implements: peripheral module gating control
 ****************************************************************************/
bool CLK_periph_is_enabled(PERIPH_module_t periph)
{
    if (PERIPH_MODULE_MAX == periph)
        return false;
    else
        return 0 != DPORT_GET_PERI_REG_MASK(periph_clk_en_reg(periph), periph_clk_en_mask(periph));
}

int CLK_periph_enable(PERIPH_module_t periph)
{
    if (PERIPH_MODULE_MAX == periph)
        return ENODEV;

    DPORT_SET_PERI_REG_MASK(periph_clk_en_reg(periph), periph_clk_en_mask(periph));
    DPORT_CLEAR_PERI_REG_MASK(periph_rst_en_reg(periph), periph_rst_en_mask(periph));
    return 0;
}

int CLK_periph_disable(PERIPH_module_t periph)
{
    if (PERIPH_MODULE_MAX == periph)
        return ENODEV;

    DPORT_CLEAR_PERI_REG_MASK(periph_clk_en_reg(periph), periph_clk_en_mask(periph));
    DPORT_SET_PERI_REG_MASK(periph_rst_en_reg(periph), periph_rst_en_mask(periph));
    return 0;
}

int CLK_periph_reset(PERIPH_module_t periph)
{
    if (PERIPH_MODULE_MAX == periph)
        return ENODEV;

    DPORT_SET_PERI_REG_MASK(periph_rst_en_reg(periph), periph_rst_en_mask(periph));
    DPORT_CLEAR_PERI_REG_MASK(periph_rst_en_reg(periph), periph_rst_en_mask(periph));
    return 0;
}

/****************************************************************************
 *  @internal
 ****************************************************************************/
static uint32_t *periph_clk_en_reg(PERIPH_module_t periph)
{
    switch (periph)
    {
    case PERIPH_DEDIC_GPIO_MODULE:
        return (uint32_t *)SYSTEM_CPU_PERI_CLK_EN_REG;
    case PERIPH_RNG_MODULE:
    case PERIPH_WIFI_MODULE:
    case PERIPH_BT_MODULE:
    case PERIPH_WIFI_BT_COMMON_MODULE:
    case PERIPH_BT_BASEBAND_MODULE:
    case PERIPH_BT_LC_MODULE:
        return (uint32_t *)SYSTEM_WIFI_CLK_EN_REG;
    case PERIPH_UART2_MODULE:
    case PERIPH_SDMMC_MODULE:
    case PERIPH_LCD_CAM_MODULE:
    case PERIPH_GDMA_MODULE:
    case PERIPH_HMAC_MODULE:
    case PERIPH_DS_MODULE:
    case PERIPH_AES_MODULE:
    case PERIPH_SHA_MODULE:
    case PERIPH_RSA_MODULE:
        return (uint32_t *)SYSTEM_PERIP_CLK_EN1_REG;
    default:
        return (uint32_t *)SYSTEM_PERIP_CLK_EN0_REG;
    }
}

static uint32_t *periph_rst_en_reg(PERIPH_module_t periph)
{
    switch (periph)
    {
    case PERIPH_DEDIC_GPIO_MODULE:
        return (uint32_t *)SYSTEM_CPU_PERI_RST_EN_REG;
    case PERIPH_RNG_MODULE:
    case PERIPH_WIFI_MODULE:
    case PERIPH_BT_MODULE:
    case PERIPH_WIFI_BT_COMMON_MODULE:
    case PERIPH_BT_BASEBAND_MODULE:
    case PERIPH_BT_LC_MODULE:
        return (uint32_t *)SYSTEM_CORE_RST_EN_REG;
    case PERIPH_UART2_MODULE:
    case PERIPH_SDMMC_MODULE:
    case PERIPH_LCD_CAM_MODULE:
    case PERIPH_GDMA_MODULE:
    case PERIPH_HMAC_MODULE:
    case PERIPH_DS_MODULE:
    case PERIPH_AES_MODULE:
    case PERIPH_SHA_MODULE:
    case PERIPH_RSA_MODULE:
        return (uint32_t *)SYSTEM_PERIP_RST_EN1_REG;
    default:
        return (uint32_t *)SYSTEM_PERIP_RST_EN0_REG;
    }
}
static uint32_t periph_clk_en_mask(PERIPH_module_t periph)
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
        return SYSTEM_WIFI_CLK_WIFI_EN;
    case PERIPH_BT_MODULE:
        return SYSTEM_WIFI_CLK_BT_EN;
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

static uint32_t periph_rst_en_mask(PERIPH_module_t periph)
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