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
#ifndef __HW_TIMER_HAL_H
#define __HW_TIMER_HAL_H                1

#include <rtos/glist.h>
#include "../timer.h"

__BEGIN_DECLS

/***************************************************************************/
/** TIMER @features
****************************************************************************/
    struct TIMER_HAL_feature_t
    {
        uint32_t DEV_count;
        uint16_t MATCH_count;
        uint16_t CAPTURE_count;
    };

extern
    struct TIMER_HAL_feature_t const TIMER_HAL_feature;

/***************************************************************************/
/** TIMER @initialize
****************************************************************************/
    /// timer working mode @matching context
    struct TIMER_match_context
    {
        glist_t list;
        uint32_t list_count;

        // section matching was executing
        struct TIMER_match_section *section_executing;
        // section is appending match point
        struct TIMER_match_section *section_pushing;

        // current matching point about to loading
        struct TIMER_match_point *curr;
        /**
         *  tick shift of current hardware matching point
         *      hardware timer support only few matching point, but.we simulate unlimited
         *  matching point by software cooperate with hardware mathing point
         *      .the matching point was sort in order
         *      .the last HW matching point must designated as reset point
         *      .tick_shift is increased by reset point value
         */
        uint32_t tick_shift;
    };

    /// timer working mode @PWM context
    struct TIMER_pwm_context
    {
        void *gpio;
        uint32_t pin;

        uint16_t duty;
        uint16_t cycle;
    };

    /// single @TIMER context
    struct TIMER_context
    {
        void *dev;                      // pointer to TIMER[x] device
        struct TIMER_match_point **dev_match_points;

    ///----- runtime property, initialize to zero
        TIMER_callback_t callback;
        uint32_t curr_repeating;

        uint32_t mode;
        union
        {
            struct TIMER_pwm_context PWM;
            struct TIMER_match_context MATCH;
        } config;
    };
extern
    struct TIMER_context *TIMER_context;

extern __attribute__((nothrow))
    void TIMER_HAL_init(void);

/***************************************************************************/
/** TIMER @configure
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int TIMER_HAL_configure(void *const dev, int nb, uint32_t freq, enum TIMER_mode_t mode);

extern __attribute__((nothrow, nonnull))
    void TIMER_HAL_deconfigure(void *const dev, int nb);

extern __attribute__((nothrow))
    void TIMER_HAL_intr_enable(void *const dev, int nb, enum TIMER_mode_t mode);

extern __attribute__((nothrow))
    void TIMER_HAL_intr_disable(void *const dev, int nb);

/***************************************************************************/
/** TIMER @control
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    void TIMER_HAL_start(void *const dev);

extern __attribute__((nothrow, nonnull))
    void TIMER_HAL_stop(void *const dev);

extern __attribute__((nothrow, nonnull))
    bool TIMER_HAL_is_running(void *const dev);

extern __attribute__((nothrow, nonnull))
    uint32_t TIMER_HAL_tick(void *const dev);

/***************************************************************************/
/** TIMER @match
****************************************************************************/
void TIMER_HAL_setmatch(void *const dev, uint8_t ch, uint32_t val);

/***************************************************************************/
/** TIMER @PWM
****************************************************************************/
extern __attribute__((nothrow, nonnull(1)))
    int TIMER_PWM_HAL_get_channel(void *gpio, uint32_t pin);

extern __attribute__((nothrow, nonnull(2)))
    void TIMER_PWM_HAL_release(int nb, void *gpio, uint32_t pin);

    /**
     * TIMER_PWM_HAL_polarity()
     *  set PWM default polarity, depend by PORT'PIN default LEVEL (invert)
     */
extern __attribute__((nothrow, nonnull(2)))
    void TIMER_PWM_HAL_polarity(void *const dev, void *gpio, uint32_t pin);

    /**
     *  TIMER_PWM_HAL_update() should start TIMER'PWM immdiately
     */
extern __attribute__((nothrow))
    void TIMER_PWM_HAL_update(void *const dev, uint32_t duty, uint32_t cycle);

extern __attribute__((nothrow))
    void TIMER_PWM_HAL_update_duty(void *const dev, uint32_t duty);

/***************************************************************************/
/**  TIMER @interrupt callback
****************************************************************************/
extern __attribute__((nothrow))
    void TIMER_HAL_intr_callback(int nb, uint8_t ch, uint32_t val);

__END_DECLS
#endif
