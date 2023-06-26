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

#include "hw/pwm.h"
#include "hw/hal/pwm_hal.h"

/***************************************************************************/
/** @internal
****************************************************************************/
/// @var
static struct PWM_config *PWM_config;

/***************************************************************************/
/** @constructor
****************************************************************************/
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

__attribute__((constructor(102)))
static void PWM_initialize(void)
{
    PWM_config = PWM_HAL_init();
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

/***************************************************************************/
/** @PWM configure
****************************************************************************/
int PWM_get(uint32_t const freq, uint32_t pin, PWM_callback_t callback)
{
    int retval = PWM_HAL_get_channel(freq, pin);

    if (-1 != retval)
    {
        struct PWM_config *config = &PWM_config[retval];

        config->pin = pin;
        config->callback = callback;

        if (config->callback)
            PWM_HAL_intr_enable(retval);
    }
    return retval;
}

int PWM_release(int nb)
{
    if ((uint32_t)nb > PWM_HAL_feature.DEV_count)
        return ENODEV;

    PWM_HAL_release(PWM_config[nb].pin);
    return 0;
}

/***************************************************************************/
/** @INT callback
****************************************************************************/
void PWM_HAL_intr_callback(int nb)
{
    PWM_config[nb].callback_count ++;
    PWM_config[nb].callback(PWM_config[nb].callback_count);
}
