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
#ifndef __HW_GPIO_H
#define __HW_GPIO_H                     1

#include <features.h>
#include <stdint.h>
#include <stdbool.h>

    #define GPIO_ALL_PIN                (0xFFFFFFFFUL)

    enum GPIO_trig_t
    {
    /*
        +----------+----------+----------+----------------+
        |   EXT    | POLARITY |   LEVEL  |    Feature     |
        +----------+----------+----------+----------------+
        |    0     |    0     |     0    |  FAILLING EDGE |
        |    0     |    0     |     1    |  LOW LEVEL     |
        |    0     |    1     |     0    |  RISING EDGE   |
        |    0     |    1     |     1    |  HIGH LEVEL    |
        +----------+----------+----------+----------------+
        |    1     |    0     |     0    |  BOTH EDGE     |
        +----------+----------+----------+----------------+
    */
        TRIG_BY_FALLING_EDGE        = 0,
        TRIG_BY_LOW_LEVEL,
        TRIG_BY_RISING_EDGE,
        TRIG_BY_HIGH_LEVEL,
        // extension
        TRIG_BY_BOTH_EDGE           = 4
    };
    #define GPIO_TRIG_IS_LEVEL(TRIG)    (0 != (TRIG & 0x01))
    #define GPIO_TRIG_IS_EDGE(TRIG)     (0 == (TRIG & 0x01))

    enum GPIO_pad_pull_t
    {
        HIGH_Z                      = 0,
        PULL_DOWN,
        PULL_UP,
        // alias
        FLOATING  = HIGH_Z
    };

    enum GPIO_output_mode_t
    {
        PUSH_PULL                   = 0,
        PUSH_PULL_DOWN,
        PUSH_PULL_UP,
        OPEN_SOURCE,
        OPEN_SOURCE_WITH_PULL_DOWN,
        OPEN_DRAIN,
        OPEN_DRAIN_WITH_PULL_UP,
        OPEN_DRAIN_WITH_FILTER,
        OPEN_DRAIN_WITH_PULL_UP_FILTER,
    // alias
        WIRED_OR = OPEN_SOURCE,
        WIRED_OR_WITH_PULL_DOWN = OPEN_SOURCE_WITH_PULL_DOWN,
        WIRED_AND = OPEN_DRAIN,
        WIRED_AND_WITH_PULL_UP = OPEN_DRAIN_WITH_PULL_UP,
        WIRED_AND_WITH_FILTER = OPEN_DRAIN_WITH_FILTER,
        WIRED_AND_WITH_PULL_UP_FILTER = OPEN_DRAIN_WITH_PULL_UP_FILTER,
    };

__BEGIN_DECLS

/***************************************************************************/
/**  GPIO common
****************************************************************************/
    /**
     *  GPIO debounce for INPUT mode only
     */
extern __attribute__((nothrow))
    int GPIO_debounce(uint32_t pins, uint32_t millisecond);

    /**
     *  GPIO hold to repeating for INPUT mode only
     */
extern __attribute__((nothrow))
    int GPIO_hold_repeating(uint32_t pins, uint32_t millisecond);

/***************************************************************************/
/**  GPIO direction configure
****************************************************************************/
    /**
     *  GPIO_disable()
     *      disable GPIO, if chip supported it
     *      otherwise should set direction as input with HIGH-Z / GPIO_disable_with_pullup(): PULL-UP
    */
extern __attribute__((nothrow, nonnull))
    int GPIO_disable(uint32_t pins);
extern __attribute__((nothrow, nonnull))
    int GPIO_disable_with_pullup(uint32_t pins);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_input(uint32_t pins);
extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_input_pp(uint32_t pins, enum GPIO_pad_pull_t pp, bool filter_en);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_output(enum GPIO_output_mode_t mode, uint32_t pins);

/***************************************************************************/
/** GPIO IO
****************************************************************************/
    /**
     *  GPIO_peek(): peek Input LEVEL
     */
extern __attribute__((nothrow, nonnull))
    uint32_t GPIO_peek(uint32_t pins);

    /**
     *  GPIO_peek_output(): peek Output drived LEVEL
     */
extern __attribute__((nothrow, nonnull))
    uint32_t GPIO_peek_output(uint32_t pins);

    /**
     *  GPIO_toggle(): Output driven toggle LEVEL
     */
extern  __attribute__((nothrow, nonnull))
    void GPIO_toggle(uint32_t pins);

    /**
     *  GPIO_set(): Output driven HIGH LEVEL
     */
extern __attribute__((nothrow, nonnull))
    void GPIO_set(uint32_t pins);

    /**
     *  GPIO_clear(): Output driven LOW LEVEL
     */
extern __attribute__((nothrow, nonnull))
    void GPIO_clear(uint32_t pins);

/***************************************************************************/
/**  GPIO Interrupt Control
****************************************************************************/
    /**
     *  GPIO interrupt's @callback
     */
typedef void (*GPIO_callback_t)(uint32_t pins, void *arg);

    /**
     *  GPIO_intr_enable_cb()
     *      updates PORT.PINs's TRIG or add a new callback
     */
extern __attribute__((nothrow, nonnull(3)))
    int GPIO_intr_enable(uint32_t pins, enum GPIO_trig_t trig, GPIO_callback_t callback, void *arg);

    /**
     *  GPIO_intr_disable()
     *      removes all interrupt callback associated with PORT.PINs
     */
extern __attribute__((nothrow, nonnull))
    void GPIO_intr_disable(uint32_t pins);

    /**
     *  GPIO_intr_disable_cb()
     *      removes all interrupt callback associated with callback entry
     */
extern __attribute__((nothrow, nonnull))
    void GPIO_intr_disable_cb(GPIO_callback_t callback);

__END_DECLS
#endif
