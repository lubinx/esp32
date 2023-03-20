#include <stdio.h>
#include <stdbool.h>

#include <sys/errno.h>
#include <soc/io_mux_reg.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>

#include "gpio.h"
#include "hw/hal/gpio_hal.h"

/***************************************************************************/
/** @def
****************************************************************************/
#define PIN_NB_IS_INVALID(NB)           (NB > 48 || (NB > 21 && pin_nb < 26))

union io_mux_reg_t
{
    struct
    {
        uint32_t slp_oe     : 1;
        uint32_t slp_sel    : 1;
        uint32_t slp_pd     : 1;
        uint32_t slp_pu     : 1;
        uint32_t slp_ie     : 1;
        uint32_t slp_drv    : 2;
        uint32_t func_pd    : 1;
        uint32_t func_pu    : 1;
        uint32_t func_ie    : 1;
        uint32_t func_drv   : 2;
        uint32_t func_sel   : 3;
        uint32_t filter_en  : 1;
        uint32_t rsv        : 16;
    };

    uint32_t val;
};
typedef union io_mux_reg_t          io_mux_reg_t;

enum    // should mapping => enum GPIO_trig_t in hw/gpio.h
{
    PIN_INTR_DISABLE,
    PIN_INTR_TRIG_RISING_EDGE,
    PIN_INTR_TRIG_FALLING_EDGE,
    PIN_INTR_TRIG_BOTH_EDGE,
    PIN_INTR_TRIG_LOW_LEVEL,
    PIN_INTR_TRIG_HIGH_LEVEL,
};

/***************************************************************************/
/** @internal
****************************************************************************/
static io_mux_reg_t *IO_MUX_REG(uint8_t pin_nb);

/***************************************************************************/
/** constructor
****************************************************************************/
void GPIO_initialize(void)
{
}

void GPIO_print_iomux(void)
{
    #define PIN_DRV_MA(DRV)         (0 == DRV ? 5 : (1 == DRV ? 10 : (2 == DRV ? 20 : 30)))

    printf("+-----+-----------------------------+-----------------------------+-----+-----+\n");
    printf("|     | GenericPIO functions        | RTC(sleep) GPIO             |     |     |\n");
    printf("+ PIN +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+ FLT | SEL |\n");
    printf("|     | SEL | IE  | PD  | PU  | DRV | OE  | IE  | PD  | PU  | DRV |     |     |\n");
    printf("+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+\n");


    for (int i = 0; i < 49; i ++)
    {
        if (i < 22 || i > 25)
        {
            io_mux_reg_t *reg = IO_MUX_REG(i);

            printf("| %02d  ", i);
            printf("| %u   | %u   | %u   | %u   |%2d mA", reg->func_sel, reg->func_ie, reg->func_pd, reg->func_pu, PIN_DRV_MA(reg->func_drv));

            if (reg->slp_sel)
                printf("| %u   | %u   | %u   | %u   |%2d mA", reg->slp_oe, reg->slp_ie, reg->slp_pd, reg->slp_pu, PIN_DRV_MA(reg->slp_drv));
            else
                printf("| DISABLED                    ");

            printf("| %u   |\n", reg->filter_en);
        }
        else if (22 == i)
            printf("+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+...22~25\n");

    }

    printf("+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+\n");
    #undef PIN_DRV_MA
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
int GPIO_disable_pin_nb(enum GPIO_pad_pull_t pull, uint8_t pin_nb)
{
    io_mux_reg_t *io_mux_reg = IO_MUX_REG(pin_nb);
    if (! io_mux_reg)
        return EINVAL;

    GPIO.pin[pin_nb].int_ena = PIN_INTR_DISABLE;

    if (31 > pin_nb)
        GPIO.enable_w1tc = (1UL << pin_nb);
    else
        GPIO.enable1_w1tc.data = (1UL << (pin_nb - 32));

    io_mux_reg->func_ie = 0;
    io_mux_reg->func_pd = 0;
    io_mux_reg->func_pu = 1;        // pull-up to powersave?
}

int GPIO_setdir_input_pin_nb(uint8_t pin_nb)
{
    io_mux_reg_t *io_mux_reg = IO_MUX_REG(pin_nb);
    if (! io_mux_reg)
        return EINVAL;
}

int GPIO_setdir_output_pin_nb(enum GPIO_output_mode_t mode, uint8_t pin_nb)
{
    io_mux_reg_t *io_mux_reg = IO_MUX_REG(pin_nb);
    if (! io_mux_reg)
        return EINVAL;
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
static io_mux_reg_t *IO_MUX_REG(uint8_t pin_nb)
{
    if (PIN_NB_IS_INVALID(pin_nb))
        return NULL;
    else
        return (io_mux_reg_t *)(REG_IO_MUX_BASE + (pin_nb + 1) * sizeof(uint32_t));
}

static void GPIO_input_en(bool en)
{

}

static void GPIO_ouput_en(bool en, bool opendrain)
{

}
