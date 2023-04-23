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
#ifndef __HW_GPIO_HAL_H
#define __HW_GPIO_HAL_H                 1

#include "../gpio.h"

__BEGIN_DECLS

/***************************************************************************/
/**  GPIO @Input Interrupt Control
****************************************************************************/
extern __attribute__((nothrow))
    void GPIO_HAL_intr_enable(void *gpio, uint32_t pins, enum GPIO_trig_t trig);

extern __attribute__((nothrow))
    void GPIO_HAL_intr_disable(void *gpio, uint32_t pins);

/***************************************************************************/
/**  GPIO INT callback
****************************************************************************/
    /**
     *  GPIO HW intr handler call this to get debounce & hold repeat logic
     *      this function is staticly provide by common_gpio.c
     */
extern __attribute__((nothrow))
    void GPIO_HAL_intr_callback(void *gpio, uint32_t pins, void *arg);

    /**
     *  GPIO HW actually execute user provide's callback function
     */
extern __attribute__((nothrow))
    void GPIO_HAL_execute_callback(void *gpio, uint32_t pins, void *arg);

__END_DECLS
#endif
