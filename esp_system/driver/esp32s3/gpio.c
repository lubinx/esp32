#include <stdio.h>
#include <stdbool.h>
#include <sys/errno.h>

#include <rtos/glist.h>
#include <soc/soc_caps.h>
#include <soc/io_mux_reg.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_reg.h>

#include "gpio.h"
#include "hw/hal/gpio_hal.h"

/***************************************************************************/
/** @def
****************************************************************************/
#define GPIO_OUTPUT_ENABLE(NB)      \
    (32 > (NB) ? (GPIO.enable_w1ts = (1UL << (NB))) : (GPIO.enable1_w1ts.data = (1UL << ((NB) - 32))))
#define GPIO_OUTPUT_DISABLE(NB)     \
    (32 > (NB) ? (GPIO.enable_w1tc = (1UL << (NB))) : (GPIO.enable1_w1tc.data = (1UL << ((NB) - 32))))
#define GPIO_OUTPUT_IS_ENABLED(NB)  \
    (0 != (32 > (NB) ? (GPIO.enable & (1ULL << (NB))) : (GPIO.enable1.data & (1ULL << ((NB) - 32)))))

#define GPIO_INPUT_PEEK(NB)         \
    (32 > (NB) ? (GPIO.in & (1UL << (NB))) : (GPIO.in1.data & (1UL << ((NB) - 32))))
#define GPIO_OUTPUT_PEEK(NB)        \
    (32 > (NB) ? (GPIO.out & (1UL << (NB))) : (GPIO.out1.data & (1UL << ((NB) - 32))))
#define GPIO_OUTPUT_SET(NB)         \
    (32 > (NB) ? (GPIO.out_w1ts = (1UL << (NB))) : (GPIO.out1_w1ts.data = (1UL << ((NB) - 32))))
#define GPIO_OUTPUT_CLEAR(NB)       \
    (32 > (NB) ? (GPIO.out_w1tc = (1UL << (NB))) : (GPIO.out1_w1tc.data = (1UL << ((NB) - 32))))

#define IOMUX_SEL0_IS_GPIO(NB)      (0 != ((1ULL << NB) & (0x1FFFFFFFFFFFFULL & ~(0ULL |    \
        BIT22 | BIT23 | BIT24 | BIT25 | BIT26 | \
        BIT27  | BIT28 | BIT29 | BIT30  | BIT31 | BIT32 | \
        BIT39 | BIT40 | BIT41 | BIT42 |BIT43 | BIT44 | BIT47 | BIT48))) \
    )
#define IOMUX_IS_GPIO(NB, SEL)      ((IOMUX_SEL_GPIO == (SEL)) || (IOMUX_SEL0 == (SEL) && IOMUX_SEL0_IS_GPIO(NB)))

// IOMUX_REG[x] was undeclared
union iomux_reg
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
typedef union iomux_reg             iomux_reg_t;
// configure
static uint32_t iomux_reg_val(enum GPIO_pad_pull_t pp, uint8_t sel, uint8_t drv, bool ie, bool filter_en);

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
typedef union gpio_func_in_sel_cfg  iomux_matrix_in_t;  // alias to matrix in
#define IOMUX_MATRIX_IN_EN          (1)
#define IOMUX_MATRIX_IN_BYPASS      (0)


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
typedef union gpio_func_out_sel_cfg iomux_matrix_out_t; // alias to matrix out
#define IOMUX_MATRIX_OUT_EN         (0)
#define IOMUX_MATRIX_OUT_BYPASS     (1)

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
     *  read IOMUX_print() to get more detail
    */
    iomux_reg_t *volatile mux;
    iomux_matrix_out_t *volatile mat_out;
    iomux_matrix_in_t *volatile mat_in;       // nullable
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
        iomux_reg_t *mux;

        if ((SOC_GPIO_VALID_GPIO_MASK & (1LLU << i)))
        {
            GPIO_matrix[i].pin = (void *)&GPIO.pin[i];
            GPIO_matrix[i].mux = mux = (void *)(REG_IO_MUX_BASE + (i + 1) * sizeof(uint32_t));
        }

        if ((SOC_GPIO_VALID_OUTPUT_GPIO_MASK & (1LLU << i)))
        {
            GPIO_matrix[i].mat_out = (void *)&GPIO.func_out_sel_cfg[i];
            // bypass matrix
            GPIO_matrix[i].mat_out->oen_sel = IOMUX_MATRIX_OUT_BYPASS;

            // configure all PIN to GPIO, special configure for SPI
            switch (i)
            {
            case 26:    // SPI CS1
            case 27:    // SPI Half-Duplex(HD)
            case 28:    // SPI WP
            default:
                mux->val = iomux_reg_val(HIGH_Z, IOMUX_SEL_GPIO, mux->func_drv, false, false);
                break;

            case 29:    // SPI CS0
            case 30:    // SPI CLK
                mux->func_ie = 0;
                mux->func_pd = mux->func_pu = 0;    // HighZ
                break;

            case 31:    // SPI Q
            case 32:    // SPI D
                break;
            }
        }
    }
}

/****************************************************************************
 * @implements: io mux & matrix
 ****************************************************************************/
int IOMUX_configure(enum iomux_def mux)
{
    int pin_nb = mux >> 12;

    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    // disconnect previous matrix connections
    IOMUX_route_disconnect(pin_nb);

    iomux_reg_t reg = {.val = mat->mux->val};
    reg.mcu_sel = mux & 0xF;
    // reg.func_ie = (0 != IOMUX_IS_INPUT & mux);
    mat->mux->val = reg.val;

    uint8_t mode = 0xF & (mux >> 4);
    if (MODE_INPUT == mode || MODE_INPUT_WITH_FILTER == mode)
        return GPIO_setdir_input_pin_nb(pin_nb, 0xF & (mux >> 8), MODE_INPUT_WITH_FILTER == mode);
    else
        return GPIO_setdir_output_pin_nb(pin_nb, mode);
}

int IOMUX_deconfigure(enum iomux_def mux, bool pull_up)
{
    int pin_nb = mux >> 12;

    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    if (mat->mux->mcu_sel != (mux & 0xF))
        return EACCES;

    return GPIO_disable_pin_nb(pin_nb, pull_up);
}

int IOMUX_route_input(uint8_t pin_nb, uint16_t sig_idx, bool inv, enum GPIO_pad_pull_t pp, bool filter_en)
{
    if (sig_idx > 0x100)
        return EINVAL;

    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    // disconnect previous matrix connections
    IOMUX_route_disconnect(pin_nb);

    iomux_matrix_in_t mat_in = {0};
    mat_in.func_sel = pin_nb;
    mat_in.sig_in_inv = inv;
    mat_in.sig_in_sel = IOMUX_MATRIX_IN_EN;

    iomux_matrix_in_t *cfg = (void *)&GPIO.func_in_sel_cfg[sig_idx];
    cfg->val = mat_in.val;

    mat->mat_in = cfg;
    mat->mux->val = iomux_reg_val(pp, IOMUX_SEL_GPIO, mat->mux->func_drv, true, filter_en);
    return 0;
}

int IOMUX_route_output(uint8_t pin_nb, uint16_t sig_idx, enum GPIO_output_mode_t mode, bool inv, bool oen_inv)
{
    if (sig_idx > 0x100)
        return EINVAL;

    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    mat->mux->mcu_sel = IOMUX_SEL_GPIO;     // select IOMUX to gpio
    GPIO_setdir_output_pin_nb(pin_nb, mode);

    iomux_matrix_out_t mat_out = {0};
    mat_out.func_sel = sig_idx;
    mat_out.inv_sel = inv;
    mat_out.oen_inv_sel = oen_inv;
    mat_out.oen_sel = IOMUX_MATRIX_OUT_EN;
    mat->mat_out->val = mat_out.val;

    // open-drian: route input
    if (mat->pin->pad_driver)
    {
        iomux_matrix_in_t mat_in = {0};
        mat_in.func_sel = pin_nb;
        mat_in.sig_in_inv = inv;
        mat_in.sig_in_sel = IOMUX_MATRIX_IN_EN;

        iomux_matrix_in_t *cfg = (void *)&GPIO.func_in_sel_cfg[sig_idx];
        cfg->val = mat_in.val;

        mat->mat_in = cfg;
    }
}

int IOMUX_route_disconnect(uint8_t pin_nb)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    // bypass maxtrix output
    mat->mat_out->oen_sel = IOMUX_MATRIX_OUT_BYPASS;

    // bypass maxtrix input if is connected
    if (mat->mat_in)
    {
        mat->mat_in->sig_in_sel = IOMUX_MATRIX_IN_BYPASS;
        mat->mat_in = NULL;
    }
    return 0;
}

void IOMUX_print(void)
{
    printf("GPIO matrix: \n");

    printf("\t+-----+-----------+------+-----------------+----------------------------+-----+\n");
    printf("\t|     |           |      | Matrix INPUT    | Matrix OUTPUT              |     |\n");
    printf("\t| PIN | PAD-Pull  | MUX  +-----------+-----+-----------+-----+----+-----+ SLP |\n");
    printf("\t|     |           |      | EN => MAT | FLT | EN => MAT | LVL | OD | DRV |     |\n");
    printf("\t+-----+-----------+------+-----------+-----+-----------+-----+----+-----+-----+\n");


    for (int i = 0; i < 49; i ++)
    {
        if (22 == i)
        {
            printf("\t+-----+-----------+------+-----------+-----+-----------+----+-----+-----+-----+ ...22~25 is reserved\n");
            continue;
        }
        struct GPIO_matrix *mat = &GPIO_matrix[i];
        if (! mat->pin)
            continue;

        bool mux_is_gpio = IOMUX_IS_GPIO(i, mat->mux->mcu_sel);

        printf("\t| %02d  | %-9s |", i,
            ! mat->mux->func_pu && ! mat->mux->func_pd ? "High-Z" : (mat->mux->func_pu ? "Pull-Up" : (mat->mux->func_pd ? "Pull-Down" : "Error"))
        );
        if (IOMUX_IS_GPIO(i, mat->mux->mcu_sel))
            printf(" GPIO ");
        else
            printf(" %d    ", mat->mux->mcu_sel);

        // input
        bool input_en = mat->mux->func_ie;
        {
            if (input_en)
            {
                if (mat->mat_in)
                    printf("| ✔️ <=%c %03u ",  mat->mat_in->sig_in_inv ? '~' : '=', mat->mat_in - (iomux_matrix_in_t *)GPIO.func_in_sel_cfg);
                else
                    printf("| <= MUX    ");
            }
            else
                printf("| ❌         ");
        }
        printf("| %s   ", mat->mux->filter_en ?  "✔️" : "❌");

        // output
        {
            bool out_by_matrix = IOMUX_MATRIX_OUT_EN == mat->mat_out->oen_sel;
            bool output_en = out_by_matrix || (mux_is_gpio ? GPIO_OUTPUT_IS_ENABLED(i) : mat->pin->pad_driver || ! input_en);

            if (output_en)
            {
                if (out_by_matrix)
                    printf("| ✔️ %c=> %03u ", mat->mat_out->inv_sel ? '~' : '=', mat->mat_out->func_sel);
                else
                    printf("| => MUX    ");
            }
            else
                printf("| ❌         ");
        }
        printf("|  %d  | %s  |%2d mA",
            0 != GPIO_OUTPUT_PEEK(i),
            mat->pin->pad_driver ? "✔️" : "❌",
            0 == mat->mux->func_drv ? 5 : (1 == mat->mux->func_drv ? 10 : (2 == mat->mux->func_drv ? 20 : 30))
        );
        // sleep en
        printf("| %s   |\n", mat->mux->slp_sel ? "✔️" : "❌");
    }

    printf("\t+-----+-----------+------+-----------+-----+-----------+----+-----+-----+-----+\n");
    printf(" * ref Table 6-3. IO MUX Pin Functions\n");
    printf(" * MAT(INPUT/OUTPUT) => XXX: definitions in soc/gpio_sig_map.h\n");
    printf(" * MAT may bypass => MUX: definitions in esp32s3/include/gpio.h\n");
}

/***************************************************************************/
/** @implements
****************************************************************************/
int GPIO_disable(void *const gpio, uint32_t pins, bool pull_up)
{
    return ENOSYS;
}

int GPIO_setdir_input(void *const gpio, uint32_t pins, enum GPIO_pad_pull_t pp, bool filter_en)
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
/** @implements: by PIN number
****************************************************************************/
int GPIO_disable_pin_nb(uint8_t pin_nb, bool pull_up)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    mat->pin->val = 0;
    GPIO_OUTPUT_DISABLE(pin_nb);

    mat->mux->val = iomux_reg_val(pull_up ? PULL_UP : HIGH_Z, IOMUX_SEL_GPIO, mat->mux->func_drv, false, false);
    IOMUX_route_disconnect(pin_nb);
}

int GPIO_setdir_input_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp, bool filter_en)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    // disconnect matrix connections
    IOMUX_route_disconnect(pin_nb);

    mat->mux->val = iomux_reg_val(pp, mat->mux->mcu_sel, mat->mux->func_drv, true, filter_en);

    // disable output when output is not open-drain
    if (! mat->pin->pad_driver)
        GPIO_OUTPUT_DISABLE(pin_nb);
}

int GPIO_setdir_output_pin_nb(uint8_t pin_nb, enum GPIO_output_mode_t mode)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    enum GPIO_pad_pull_t pp;
    bool od = false;
    bool filter_en = false;

    switch (mode)
    {
    case PUSH_PULL_DOWN:
        GPIO_OUTPUT_CLEAR(pin_nb);
        goto push_pull;

    case PUSH_PULL_UP:
        GPIO_OUTPUT_SET(pin_nb);
        goto push_pull;

    case PUSH_PULL:
push_pull:
        // disconnect pad pull resistor, see Figure 6­2. Internal Structure of a Pad
        pp = HIGH_Z;
        break;

    case OPEN_DRAIN_WITH_FILTER:
        filter_en = true;
        goto open_drain;

    case OPEN_DRAIN:
open_drain:
        od = true;
        pp = HIGH_Z;
        break;

    case OPEN_DRAIN_WITH_PULL_UP_FILTER:
        filter_en = true;
        goto open_drain_with_pull_up;

    case OPEN_DRAIN_WITH_PULL_UP:
open_drain_with_pull_up:
        od = true;
        pp = PULL_UP;
        break;

/*
    case OPEN_SOURCE:
        pp = HIGH_Z;
        break;
    case OPEN_SOURCE_WITH_PULL_DOWN:
        pp = PULL_DOWN;
        break;
*/
    default:
        return ENOTSUP;
    }

    // disconnect matrix connections
    IOMUX_route_disconnect(pin_nb);

    mat->pin->pad_driver = od;
    mat->mux->val = iomux_reg_val(pp, mat->mux->mcu_sel, mat->mux->func_drv, od, filter_en);
    GPIO_OUTPUT_ENABLE(pin_nb);
}

int GPIO_input_peek_pin_nb(uint8_t pin_nb)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    return GPIO_INPUT_PEEK(pin_nb);
}

int GPIO_output_peek_pin_nb(uint8_t pin_nb)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    return GPIO_OUTPUT_PEEK(pin_nb);
}

int GPIO_output_set_pin_nb(uint8_t pin_nb)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    GPIO_OUTPUT_SET(pin_nb);
    return 0;
}

int GPIO_output_clear_pin_nb(uint8_t pin_nb)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    GPIO_OUTPUT_CLEAR(pin_nb);
    return 0;
}

int GPIO_output_toggle_pin_nb(uint8_t pin_nb)
{
    struct GPIO_matrix *mat = &GPIO_matrix[pin_nb];
    if (! mat->pin)
        return ENOTSUP;

    if (GPIO_OUTPUT_PEEK(pin_nb))
        GPIO_OUTPUT_CLEAR(pin_nb);
    else
        GPIO_OUTPUT_SET(pin_nb);
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
static uint32_t iomux_reg_val(enum GPIO_pad_pull_t pp, uint8_t sel, uint8_t drv, bool ie, bool filter_en)
{
    iomux_reg_t reg =
    {
        .slp_ie = ie,
        .slp_pd = (pp == PULL_DOWN),
        .slp_pu = (PULL_UP == pp),
        .slp_drv = drv,
        .func_pd = (pp == PULL_DOWN),
        .func_pu = (PULL_UP == pp),
        .func_drv = drv,
        .func_ie = ie,
        .mcu_sel = sel, .func_ie = ie,
        .filter_en = filter_en,
    };
    return reg.val;
}
