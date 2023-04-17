/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#include <rtos/kernel.h>

#include "hw/gpio.h"
#include "hw/hal/gpio_hal.h"

#pragma GCC diagnostic ignored "-Wunused-function"
/***************************************************************************/
/** @declaration
****************************************************************************/
struct GPIO_common_context
{
    /// @debounce
    /*
    timeout_t *debounce_timeout_id;
    void *GPIO_debounce_mask;
    uint32_t PIN_debounce_mask;
    */

    /// @hold to repeat
    /*
    timeout_t *hold_timeout_id;
    void *GPIO_repeat_mask;
    uint32_t PIN_repeat_mask;
    */

    /// stored state
    void *gpio;
    uint32_t pins;
    uint32_t dlvl;
};

struct GPIO_callback_context
{
    void *glist_next;
    GPIO_callback_t callback;
    void *arg;

    void *gpio;
    uint32_t pins;
    enum GPIO_trig_t trig;
};

/***************************************************************************/
/** @internal
****************************************************************************/
static void GPIO_timeout_callback(void *arg);

/// @var
static spinlock_t GPIO_atomic = SPINLOCK_INITIALIZER;
static struct GPIO_common_context GPIO_context = {0};

/**
 *  callback_list may eliminate by link when HAL fullly redefine the weaks
 *      GPIO_intr_enable
 *      GPIO_intr_disable
 *      GPIO_intr_disable_cb
 *      GPIO_HAL_execute_callback
 *
 */
static glist_t callback_list = GLIST_INITIALIZER(callback_list);

/***************************************************************************/
/**  GPIO @configure
****************************************************************************/
/*
int GPIO_debounce(void *const gpio, uint32_t pins, uint32_t millisecond)
{
    if (0 != GPIO_context.debounce_timeout_id)
    {
        timeout_update(GPIO_context.debounce_timeout_id, millisecond);
        timeout_stop(GPIO_context.debounce_timeout_id);
    }
    else
        GPIO_context.debounce_timeout_id = timeout_create(millisecond, GPIO_timeout_callback, 0);

    GPIO_context.GPIO_debounce_mask = gpio;
    GPIO_context.PIN_debounce_mask = pins;
    return 0;
}
*/

/*
int GPIO_hold_repeating(void *const gpio, uint32_t pins, uint32_t millisecond)
{
    if (0 != GPIO_context.hold_timeout_id)
    {
        timeout_stop(GPIO_context.hold_timeout_id);
        timeout_update(GPIO_context.hold_timeout_id, millisecond);
    }
    else
        GPIO_context.hold_timeout_id = timeout_create(millisecond, GPIO_timeout_callback, 0);

    GPIO_context.GPIO_repeat_mask = gpio;
    GPIO_context.PIN_repeat_mask = pins;
    return 0;
}
*/

/***************************************************************************/
/**  GPIO @Input Interrupt Control
****************************************************************************/
__attribute__((weak))
int GPIO_intr_enable(void *const gpio, uint32_t pins, enum GPIO_trig_t trig, GPIO_callback_t callback, void *arg)
{
    int retval = 0;
    struct GPIO_callback_context *ctx = NULL;
    spin_lock(&GPIO_atomic);

    struct GPIO_callback_context **iter = glist_iter_begin(&callback_list);
    while(iter != glist_iter_end(&callback_list))
    {
        struct GPIO_callback_context *iter_ctx = *iter;

        if (callback == iter_ctx->callback)
        {
            /*
            if (iter_ctx->trig != trig)
            {
                ASSERT_BKPT("GPIO callback must same trigger");
                retval = EINVAL;
                break;
            }
            */

            if (gpio == iter_ctx->gpio)
            {
                pins &= ~(iter_ctx->pins & pins);   // mask unset pins
                iter_ctx->pins |= pins;

                ctx = iter_ctx;
                break;
            }

            iter = glist_iter_next(&callback_list, iter);
        }
    }

    if (! ctx)
    {
        ctx = KERNEL_mallocz(sizeof(struct GPIO_callback_context));
        if (ctx)
        {
            ctx->callback = callback;
            ctx->arg = arg;

            ctx->gpio = gpio;
            ctx->pins = pins;
            ctx->trig = trig;

            glist_push_back(&callback_list, ctx);
        }
        else
            retval = ENOMEM;
    }

    if (0 == retval && pins)
        GPIO_HAL_intr_enable(gpio, pins, trig);

    spin_unlock(&GPIO_atomic);
    return retval;
}

__attribute__((weak))
void GPIO_intr_disable(void *const gpio, uint32_t pins)
{
    spin_lock(&GPIO_atomic);

    struct GPIO_callback_context **iter = glist_iter_begin(&callback_list);
    while(iter != glist_iter_end(&callback_list))
    {
        struct GPIO_callback_context *iter_ctx = *iter;
        uint32_t PIN_MASK;

        if (gpio != iter_ctx->gpio)
            goto iterate_next;

        PIN_MASK = pins & iter_ctx->pins;
        if (! PIN_MASK)
            goto iterate_next;

        iter_ctx->pins &= ~PIN_MASK;
        if (! iter_ctx->pins)
        {
            glist_iter_extract(&callback_list, iter);
            KERNEL_mfree(iter_ctx);
        }
        else
        {
iterate_next:
            iter = glist_iter_next(&callback_list, iter);
        }
    }

    GPIO_HAL_intr_disable(gpio, pins);
    spin_unlock(&GPIO_atomic);
}

__attribute__((weak))
void GPIO_intr_disable_cb(GPIO_callback_t callback)
{
    spin_lock(&GPIO_atomic);

    struct GPIO_callback_context **iter = glist_iter_begin(&callback_list);
    while(iter != glist_iter_end(&callback_list))
    {
        struct GPIO_callback_context *ctx = *iter;

        if (callback == ctx->callback)
        {
            GPIO_HAL_intr_disable(ctx->gpio, ctx->pins);

            glist_iter_extract(&callback_list, iter);
            KERNEL_mfree(ctx);
        }
        else
            iter = glist_iter_next(&callback_list, iter);
    }
    spin_unlock(&GPIO_atomic);
}

/***************************************************************************/
/** @INT callback
****************************************************************************/
void GPIO_HAL_intr_callback(void *const gpio, uint32_t pins, void *arg)
{
    /*
    if (0 != GPIO_context.debounce_timeout_id)
        timeout_stop(GPIO_context.debounce_timeout_id);
    if (0 != GPIO_context.hold_timeout_id)
        timeout_stop(GPIO_context.hold_timeout_id);
    */

    GPIO_context.gpio = gpio;
    GPIO_context.pins = pins;

    /*
    if (((uint32_t)gpio & (uint32_t)GPIO_context.GPIO_debounce_mask) &&
        (pins & GPIO_context.PIN_debounce_mask))
    {
        GPIO_context.dlvl = GPIO_peek(gpio, pins);
        timeout_start(GPIO_context.debounce_timeout_id, arg);
    }
    else
    */
        GPIO_HAL_execute_callback(gpio, pins, arg);
}

void GPIO_HAL_execute_callback(void *const gpio, uint32_t pins, void *arg)
{
    for (struct GPIO_callback_context **iter = glist_iter_begin(&callback_list);
        iter != glist_iter_end(&callback_list);
        iter = glist_iter_next(&callback_list, iter))
    {
        if (gpio != (*iter)->gpio)
            continue;

        uint32_t PIN_MASK = (*iter)->pins & pins;
        if (PIN_MASK)
            (*iter)->callback(gpio, PIN_MASK, (*iter)->arg);
    }

    /*
    if (0 != GPIO_context.hold_timeout_id)
    {
        if (((uint32_t)gpio & (uint32_t)GPIO_context.GPIO_repeat_mask) &&
            (pins & GPIO_context.PIN_repeat_mask))
        {
            GPIO_context.dlvl = GPIO_peek(gpio, pins);
            timeout_start(GPIO_context.hold_timeout_id, arg);
        }
        else
            timeout_stop(GPIO_context.hold_timeout_id);
    }
    */

    ARG_UNUSED(arg);
}

/***************************************************************************/
/** @internal
****************************************************************************/
static void GPIO_timeout_callback(void *arg)
{
    if (GPIO_context.dlvl == GPIO_peek(GPIO_context.gpio, GPIO_context.pins))
        GPIO_HAL_execute_callback(GPIO_context.gpio, GPIO_context.pins, arg);
}
