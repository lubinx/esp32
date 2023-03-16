#include "gpio.h"
#include "hw/hal/gpio_hal.h"

/***************************************************************************/
/** @implements: configure
****************************************************************************/
int GPIO_setdir_input(void *const gpio, uint32_t pins)
{
    return 0;
}

int GPIO_setdir_output(enum GPIO_output_mode_t mode, void *const gpio, uint32_t pins)
{
    return 0;
}

    /**
     *  GPIO debounce for INPUT mode only
     */
extern __attribute__((nothrow))
    int GPIO_debounce(void *const gpio, uint32_t pins, uint32_t millisecond);

    /**
     *  GPIO hold to repeating for INPUT mode only
     */
extern __attribute__((nothrow))
    int GPIO_hold_repeating(void *const gpio, uint32_t pins, uint32_t millisecond);

/***************************************************************************/
/** GPIO @IO
****************************************************************************/
    /**
     *  GPIO_peek(): peek Input LEVEL
     */
extern __attribute__((nothrow, nonnull))
    uint32_t GPIO_peek(void *const gpio, uint32_t pins);

    /**
     *  GPIO_peek_driven(): peek Output drived LEVEL
     */
extern __attribute__((nothrow, nonnull))
    uint32_t GPIO_peek_driven(void *const gpio, uint32_t pins);

    /**
     *  GPIO_toggle(): Output driven toggle LEVEL
     */
extern  __attribute__((nothrow, nonnull))
    void GPIO_toggle(void *const gpio, uint32_t pins);

    /**
     *  GPIO_set(): Output driven HIGH LEVEL
     */
extern __attribute__((nothrow, nonnull))
    void GPIO_set(void *const gpio, uint32_t pins);

    /**
     *  GPIO_clear(): Output driven LOW LEVEL
     */
extern __attribute__((nothrow, nonnull))
    void GPIO_clear(void *const gpio, uint32_t pins);


void GPIO_HAL_intr_enable(void *const gpio, uint32_t pins, enum GPIO_trig_t trig)
{
}

void GPIO_HAL_intr_disable(void *const gpio, uint32_t pins)
{

}
