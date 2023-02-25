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
#ifndef __HW_PWM_HAL_H
#define __HW_PWM_HAL_H                  1

#include "../gpio.h"
#include "../pwm.h"

__BEGIN_DECLS

/***************************************************************************/
/** PWM @features
****************************************************************************/
    struct PWM_HAL_feature_t
    {
        uint32_t DEV_count;
    };

extern
    struct PWM_HAL_feature_t const PWM_HAL_feature;

/***************************************************************************/
/** PWM @initialize
****************************************************************************/
    struct PWM_config
    {
        void *gpio;
        uint32_t pin;

        PWM_callback_t callback;
        uint32_t callback_count;
    };

extern __attribute__((nothrow))
    struct PWM_config *PWM_HAL_init(void);

/***************************************************************************/
/** PWM @configure
****************************************************************************/
    /**
     *  PWM_HAL_get_channel()
     *      get PWM device no associate with PORT.PIN
     */
extern __attribute__((nothrow, nonnull))
    int PWM_HAL_get_channel(uint32_t const freq, void *const gpio, uint32_t pin);

extern __attribute__((nothrow, nonnull))
    void PWM_HAL_release(void *const gpio, uint32_t pin);

extern __attribute__((nothrow))
    void PWM_HAL_intr_enable(int nb);

extern __attribute__((nothrow))
    void PWM_HAL_intr_disable(int nb);

/***************************************************************************/
/** PWM @interrupt callback
****************************************************************************/
extern __attribute__((nothrow))
    void PWM_HAL_intr_callback(int nb);

__END_DECLS
#endif
