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
#ifndef __HW_PWM_H
#define __HW_PWM_H                      1

#include <features.h>
#include <stdint.h>
#include <stdbool.h>

__BEGIN_DECLS

/***************************************************************************/
/** PWM @configure
****************************************************************************/
    /**
     *
     */
typedef void (*PWM_callback_t)(uint32_t);

    /**
     *  PWM_get()
     *      get PWM device no associate on PORT.PIN
     */
extern __attribute__((nothrow, nonnull(2)))
    int PWM_get(uint32_t const freq, void *gpio, uint32_t pin, PWM_callback_t callback);

extern __attribute__((nothrow))
    int PWM_release(int nb);

/***************************************************************************/
/** PWM @control
****************************************************************************/
    /**
     *  PWM_update()
     *      @returns
     *          On Success 0 is returned
     *          On error errno is is returned to indicate the error
     *      @errors
     *          ENODEV, EINVAL
     */
extern __attribute__((nothrow))
    int PWM_update(int nb, uint32_t duty, uint32_t cycle);

    /**
     *  PWM_update()
     *      @returns
     *          On Success 0 is returned
     *          On error errno is is returned to indicate the error
     *      @errors
     *          ENODEV, EINVAL
     */
extern __attribute__((nothrow))
    int PWM_update_duty(int nb, uint32_t duty);

__END_DECLS
#endif
