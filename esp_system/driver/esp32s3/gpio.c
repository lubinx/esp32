#include <sys/errno.h>

#include "gpio.h"
#include "hw/hal/gpio_hal.h"

/***************************************************************************/
/** constructor
****************************************************************************/
void GPIO_initialize(void)
{
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
int GPIO_disable_pin_nb(enum GPIO_resistor_t pull, uint8_t pin_nb)
{

}

int GPIO_setdir_input_pin_nb(uint8_t pin_nb)
{

}

int GPIO_setdir_output_pin_nb(enum GPIO_output_mode_t mode, uint8_t pin_nb)
{

}

/***************************************************************************/
/** @implements
****************************************************************************/
uint32_t GPIO_peek(void *const gpio, uint32_t pins)
{

}

uint32_t GPIO_peek_output(void *const gpio, uint32_t pins)
{
}

void GPIO_toggle(void *const gpio, uint32_t pins)
{
}

void GPIO_set(void *const gpio, uint32_t pins)
{
}

void GPIO_clear(void *const gpio, uint32_t pins)
{
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
