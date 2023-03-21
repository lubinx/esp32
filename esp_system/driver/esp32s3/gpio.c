#include <stdio.h>
#include <stdbool.h>
#include <sys/errno.h>

#include <rtos/glist.h>
#include <soc/soc_caps.h>
#include <soc/io_mux_reg.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>

#include "gpio.h"
#include "hw/hal/gpio_hal.h"

/***************************************************************************/
/** @def
****************************************************************************/
// esp32s3 valid pin is 0~21, 22~48
#define PIN_NB_IS_INVALID(NB)           (NB > 48 || (NB > 21 && NB < 26))

// IO_MUX_REG[x] was undeclared
union io_mux_reg
{
    struct
    {
        uint32_t slp_oe             : 1;
        uint32_t slp_sel            : 1;
        uint32_t slp_pd             : 1;
        uint32_t slp_pu             : 1;
        uint32_t slp_ie             : 1;
        uint32_t slp_drv            : 2;
        uint32_t func_pd            : 1;
        uint32_t func_pu            : 1;
        uint32_t func_ie            : 1;
        uint32_t func_drv           : 2;
        uint32_t mcu_sel            : 3;
        uint32_t filter_en          : 1;
        uint32_t rsv                : 16;
    };
    uint32_t val;
};
typedef union io_mux_reg            io_mux_reg_t;

// GPIO.pin[xx] was unnamed
union gpio_pin_reg
{
    struct
    {
        uint32_t sync2_bypass       : 2;
        uint32_t pad_driver         : 1;    // 0: push-pull, 1: open-drain
        uint32_t sync1_bypass       : 2;
        uint32_t reserved5          : 2;
        uint32_t int_type           : 3;    // enum gpio_pin_inttype_t below
        uint32_t wakeup_enable      : 1;
        uint32_t config             : 2;
        uint32_t int_ena            : 5;    // 0: disabled, 1: enabled, 2: enable non-maskable interrupt(how?), other: unknown
        uint32_t reserved18         : 14;
    };
    uint32_t val;
};
typedef union gpio_pin_reg          gpio_pin_reg_t;

// GPIO.func_in_sel_cfg was unnamed
union gpio_func_in_sel_cfg
{
    struct
    {
        uint32_t func_sel           : 6;    // PIN number: 0~48
        uint32_t sig_in_inv         : 1;
        uint32_t sig_in_sel         : 1;    // 0: bypass matrix, 1: using matrix
        uint32_t reserved8          : 24;
    };
    uint32_t val;
};
typedef union gpio_func_in_sel_cfg  gpio_func_in_sel_cfg_t;
#define GPIO_FUNC_IN_USE_MATRIX         (1)
#define GPIO_FUNC_IN_BYPASS_MATRIX      (0)

// GPIO.func_out_sel_cfg was unnamed
union gpio_func_out_sel_cfg
{
    struct
    {
        uint32_t func_sel           : 9;    // soc/gpio_sig_map.h
        uint32_t inv_sel            : 1;
        uint32_t oen_sel            : 1;    // 0: use func_sel matrix; 1: bypass matrix to using GPIO.(OUT)enable/1
        uint32_t oen_inv_sel        : 1;
        uint32_t reserved12         : 20;
    };
    uint32_t val;
};
typedef union gpio_func_out_sel_cfg gpio_func_out_sel_cfg_t;
#define GPIO_FUNC_OUT_USE_MATRIX        (0)
#define GPIO_FUNC_OUT_BYPASS_MATRIX     (1)

// gpio_pin_reg_t::inttype values, should mapping => hw/gpio.h: enum GPIO_trig_t
enum gpio_pin_inttype
{
    PIN_INTR_DISABLE                = 0,
    PIN_INTR_TRIG_RISING_EDGE,
    PIN_INTR_TRIG_FALLING_EDGE,
    PIN_INTR_TRIG_BOTH_EDGE,
    PIN_INTR_TRIG_LOW_LEVEL,
    PIN_INTR_TRIG_HIGH_LEVEL,
};
static enum gpio_pin_inttype const GPIO_intr_trig_xlat[] =
{
    [TRIG_BY_FALLING_EDGE]  = PIN_INTR_TRIG_FALLING_EDGE,
    [TRIG_BY_LOW_LEVEL]     = PIN_INTR_TRIG_LOW_LEVEL,
    [TRIG_BY_RISING_EDGE]   = PIN_INTR_TRIG_RISING_EDGE,
    [TRIG_BY_HIGH_LEVEL]    = PIN_INTR_TRIG_HIGH_LEVEL,
    [TRIG_BY_BOTH_EDGE]     = PIN_INTR_TRIG_BOTH_EDGE,
};

/***************************************************************************/
/** @internal
****************************************************************************/
struct GPIO_matrix
{
    /**
     * non-null PIN 0~21 & PIN 22~48
    */
    gpio_pin_reg_t *volatile pin;

    /**
     * mux & mat_out & mat_in to decide the IO routing...this is sickly:
     *  read GPIO_print_iomux() to get more detail
    */
    io_mux_reg_t *volatile mux;
    gpio_func_out_sel_cfg_t *volatile mat_out;
    gpio_func_in_sel_cfg_t *volatile mat_in;       // nullable
};
static struct GPIO_matrix GPIO_matrix[SOC_GPIO_PIN_COUNT] = {0};

/****************************************************************************
 *  initialization: called by SOC_initialize()
 ****************************************************************************/
void GPIO_initialize(void)
{
    // GPIO initial direct-INPUT is on
    for (int i = 0; i < lengthof(GPIO.func_in_sel_cfg); i ++)
        GPIO.func_in_sel_cfg[i].val = 0;

    // GPIO initial direct-OUTPUT is off
    GPIO.enable = 0;
    GPIO.enable1.data = 0;

    for (int i = 0; i < SOC_GPIO_PIN_COUNT; i ++)
    {
        io_mux_reg_t *mux;

        if ((SOC_GPIO_VALID_GPIO_MASK & (1LLU << i)))
        {
            GPIO_matrix[i].pin = (void *)&GPIO.pin[i];
            GPIO_matrix[i].mux = mux = (void *)(REG_IO_MUX_BASE + (i + 1) * sizeof(uint32_t));
        }

        if ((SOC_GPIO_VALID_OUTPUT_GPIO_MASK & (1LLU << i)))
        {
            GPIO_matrix[i].mat_out = (void *)&GPIO.func_out_sel_cfg[i];
            // default to disable output
            GPIO_matrix[i].mat_out->oen_sel = GPIO_FUNC_OUT_BYPASS_MATRIX;

            // configure all PIN to GPIO, special configure for SPI
            switch (i)
            {
            case 26:    // SPI CS1
            case 28:    // SPI WP
            case 29:    // SPI CS0
            case 30:    // SPI CLK
                mux->func_ie = 0;
                mux->func_pd = mux->func_pu = 0;    // HighZ
                break;
            case 31:    // SPI Q
            case 32:    // SPI D
                break;

            case 27:    // SPI Half-Duplex(HD)
            default:
                mux->mcu_sel = IO_MUX_GPIO;
                break;
            }
        }
    }

    for (int i = 0; i < 22; i ++)
    {
        GPIO_matrix[i].mux->mcu_sel = IO_MUX_GPIO;
    }
    for (int i = 33; i < 49; i ++)
    {
        GPIO_matrix[i].mux->mcu_sel = IO_MUX_GPIO;
    }
}

void GPIO_print_iomux(void)
{
    printf("GPIO matrix: \n");
    printf(" * MAT(INPUT/OUTPUT) => XXX: definitions was inside soc/gpio_sig_map.h\n");
    printf(" * MAT may bypass => MUX\n");

    #define _EN(EN)                 (EN ? "✔️" : "❌")
    #define _PAD_PULL(PU, PD)       ((! PU && ! PD) ? "High-Z" : (PU ? "Pull-Up" : (PD ? "Pull-Down" : "Error")))
    #define _DRV_MA(DRV)            (0 == DRV ? 5 : (1 == DRV ? 10 : (2 == DRV ? 20 : 30)))
    #define _OD(OEN, OD)            (OEN? (OD ? "WiredAnd" : "PushPull") : "")

    printf("\t+-----+----------+-----+-----------------+----------------------------+-----+\n");
    printf("\t|     |          |     | INPUT           | OUTPUT                     |     |\n");
    printf("\t| PIN | PAD-Pull | MUX +-----------+-----+-----------+----------+-----+ SLP |\n");
    printf("\t|     |          |     | EN => MAT | FLT | EN => MAT |   Mode   | DRV |     |\n");
    printf("\t+-----+----------+-----+-----------+-----+-----------+----------+-----+-----+\n");


    for (int i = 0; i < 49; i ++)
    {
        if (22 == i)
        {
            printf("\t+-----+-----+----------+-----------+-----+-----------+----------+-----+-----+ ...22~25 is reserved\n");
            continue;
        }
        struct GPIO_matrix *mat = &GPIO_matrix[i];
        if (! mat->pin)
            continue;

        printf("\t| %02d  | %-9s| %-3u ", i, _PAD_PULL(mat->mux->func_pu, mat->mux->func_pd), mat->mux->mcu_sel);

        // input
        if (mat->mux->func_ie)
        {
            if (mat->mat_in)
                printf("| %s  => %3u |  %s  ",  _EN(mat->mux->func_ie), mat->mat_in->sig_in_sel,  _EN(mat->mux->filter_en));
            else
                printf("| %s  => MUX |  %s  ",  _EN(mat->mux->func_ie), _EN(mat->mux->filter_en));
        }
        else
            printf("| %s         |  %s  ",  _EN(mat->mux->func_ie), _EN(mat->mux->filter_en));

        // output
        bool out_by_matrix = GPIO_FUNC_OUT_USE_MATRIX == mat->mat_out->oen_sel;
        bool out_en = out_by_matrix ||
            IO_MUX_GPIO != mat->mux->mcu_sel ||     // output is always enabled unless its GPIO
            (32 > i ? (0 != (GPIO.enable & (1 << i))) : (0 != (GPIO.enable1.data & (1 << (i - 32)))));

        if (out_en)
        {
            if (out_by_matrix)
                printf("| %s  => %3u ", _EN(out_en), mat->mat_out->func_sel);
            else
                printf("| %s  => MUX ", _EN(out_en));
        }
        else
            printf("| %s         ", _EN(out_en));

        printf("| %-8s |%2d mA", _OD(out_en, mat->pin->pad_driver), _DRV_MA(mat->mux->func_drv));

        // sleep en
        printf("|  %s  |\n", _EN(mat->mux->slp_sel));
    }

    printf("\t+-----+-----+----------+-----------+-----+-----------+----------+-----+-----+\n\n");

    #undef _EN
    #undef _OD
    #undef _PAD_PULL
    #undef _DRV_MA
}

/***************************************************************************/
/** @implements: configure
****************************************************************************/
int GPIO_disable(void *const gpio, uint32_t pins)
{
    return ENOSYS;
}

int GPIO_setdir_input(void *const gpio, uint32_t pins)
{
    return ENOSYS;
}

int GPIO_setdir_output(enum GPIO_output_mode_t mode, void *const gpio, uint32_t pins)
{
    if (OPEN_SOURCE == mode)
        return ENOTSUP;

    return ENOSYS;
}

int GPIO_debounce(void *const gpio, uint32_t pins, uint32_t millisecond)
{
    return ENOSYS;
}

int GPIO_hold_repeating(void *const gpio, uint32_t pins, uint32_t millisecond)
{
    return ENOSYS;
}

/***************************************************************************/
/** @implements: configure by PIN number
****************************************************************************/
int GPIO_connect_in_signal(uint8_t pin_nb, uint16_t sig_idx, bool inv)
{
    return 0;
}

int GPIO_connect_out_signal(uint8_t pin_nb, uint16_t sig_idx, bool inv, bool oen_inv)
{
    return 0;
}

int GPIO_disconnect_signal(uint8_t pin_nb, enum io_mux_pin mux)
{
    if (mux > IO_MUX_FUNC4 && (pin_nb != mux >> 4))
        return EINVAL;

    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    mat->mux->mcu_sel = mux & 0xF;
    // bypass maxtrix output
    mat->mat_out->oen_sel = GPIO_FUNC_OUT_BYPASS_MATRIX;

    // bypass maxtrix input if is connected
    if (mat->mat_in)
    {
        mat->mat_in->sig_in_sel = GPIO_FUNC_IN_BYPASS_MATRIX;
        mat->mat_in = NULL;
    }
    return 0;
}

static void GPIO_set_mux_reg(io_mux_reg_t *mux, uint8_t sel, uint32_t drv, bool filter_en, enum GPIO_pad_pull_t pp)
{
    switch (pp)
    {
    case HIGH_Z:
        mux->func_pd = 0;
        mux->func_pu = 0;
        break;
    case PULL_DOWN:
        mux->func_pu = 0;
        mux->func_pd = 1;
        break;
    case PULL_UP:
        mux->func_pd = 0;
        mux->func_pu = 1;
        break;
    }

    mux->mcu_sel = sel;
    mux->func_drv = drv;
    mux->filter_en = filter_en;
}

int GPIO_disable_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    if (31 > pin_nb)
        GPIO.enable_w1tc = (1UL << pin_nb);
    else
        GPIO.enable1_w1tc.data = (1UL << (pin_nb - 32));

    mat->pin->val = 0;

    io_mux_reg_t mux = {0};
    GPIO_set_mux_reg(&mux, IO_MUX_GPIO, mat->mux->func_drv, mat->mux->filter_en, pp);
    mat->mux->val = mux.val;

    GPIO_disconnect_signal(pin_nb, IO_MUX_GPIO);
}

int GPIO_setdir_input_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp, bool filter_en)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    // disconnect previous matrix connects
    GPIO_disconnect_signal(pin_nb, IO_MUX_GPIO);

    io_mux_reg_t mux = {0};
    GPIO_set_mux_reg(&mux, IO_MUX_GPIO, mat->mux->func_drv, filter_en, pp);
    mux.func_ie = 1;
    mat->mux->val = mux.val;
}

int GPIO_setdir_output_pin_nb(uint8_t pin_nb, enum GPIO_output_mode_t mode)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    // disconnect previous matrix connects
    GPIO_disconnect_signal(pin_nb, IO_MUX_GPIO);

    io_mux_reg_t mux = {0};
    GPIO_set_mux_reg(&mux, IO_MUX_DEF, mat->mux->func_drv, mat->mux->filter_en, PULL_DOWN);
    mat->mux->val = mux.val;
}

int GPIO_setod_pin_nb(uint8_t pin_nb, bool pull_up, bool filter)
{
}

/***************************************************************************/
/** @implements
****************************************************************************/
uint32_t GPIO_peek(void *const gpio, uint32_t pins)
{
    if ((uintptr_t)gpio != (uintptr_t)gpio & pins)
        return (uint32_t)__set_errno_nullptr(EINVAL);
}

uint32_t GPIO_peek_output(void *const gpio, uint32_t pins)
{
    if ((uintptr_t)gpio != (uintptr_t)gpio & pins)
        return (uint32_t)__set_errno_nullptr(EINVAL);
}

void GPIO_toggle(void *const gpio, uint32_t pins)
{
    if ((uintptr_t)gpio != (uintptr_t)gpio & pins)
        return (void)__set_errno_nullptr(EINVAL);

}

void GPIO_set(void *const gpio, uint32_t pins)
{
    if ((uintptr_t)gpio != (uintptr_t)gpio & pins)
        return (void)__set_errno_nullptr(EINVAL);

}

void GPIO_clear(void *const gpio, uint32_t pins)
{
    if ((uintptr_t)gpio != (uintptr_t)gpio & pins)
        return (void)__set_errno_nullptr(EINVAL);

}

/***************************************************************************/
/** @implements: interrupt hal
****************************************************************************/
void GPIO_HAL_intr_enable(void *const gpio, uint32_t pins, enum GPIO_trig_t trig)
{
}

void GPIO_HAL_intr_disable(void *const gpio, uint32_t pins)
{

}

/***************************************************************************/
/** @internal
****************************************************************************/
static void GPIO_input_en(bool en)
{

}

static void GPIO_ouput_en(bool en, bool opendrain)
{

}
