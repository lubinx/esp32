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
#ifndef __HW_TIMER_H
#define __HW_TIMER_H                1

#include <features.h>

/***************************************************************************
 *  TIMER @alias
 ***************************************************************************/
    #define HW_TIMER0                   0
    #define HW_TIMER1                   1
    #define HW_TIMER2                   2
    #define HW_TIMER3                   3
    #define HW_TIMER4                   4
    #define HW_TIMER5                   5
    #define HW_TIMER6                   6
    #define HW_TIMER7                   7
    #define HW_TIMER8                   8
    #define HW_TIMER9                   9

/***************************************************************************
 *  TIMER @common
 ***************************************************************************/
    enum TIMER_mode_t
    {
        TIMER_UNCONFIGURED          = 0,
        TIMER_FREE_RUNNING,
        TIMER_MATCH,
        TIMER_PWM,
        TIMER_CAPTURE
    };

__BEGIN_DECLS
    typedef void (*TIMER_callback_t)(uint16_t ID, void *arg, uint32_t loop);

    /**
     *  TIMER_configure()
     *      TIMER_free_running_configure(nb, freq)
     *      TIMER_match_configure(nb, freq, CB)
     *      TIMER_PWM_get() will internally calling TIMER_configure()
     *  @errors
     *      ENODEV
     *      EBUSY when TIMER was configured
     */
extern __attribute__((nothrow))
    int TIMER_configure(int nb, uint32_t const freq, enum TIMER_mode_t mode, TIMER_callback_t callback);

extern __attribute__((nothrow))
    bool TIMER_is_configured(int nb);

    /**
     *  TIMER_deconfigure()
     *      TIMER => unconfigured
     *  @errors
     *      ENODEV
     */
extern __attribute__((nothrow))
    int TIMER_deconfigure(int nb);

    /**
     *  TIMER_start()
     *      start/resume TIMER, TIMER PWM mode should use TIMER_PWM_update() to start
     *  @errors
     *      ENODEV
     *      EPERM when TIMER was unconfigured or configured as PWM
     *      EACCES when TIMER was already started
     *      ENOENT when nothing to execute
     */
extern __attribute__((nothrow))
    int TIMER_start(int nb);

    /**
     *  TIMER_stop()
     *      stop/pause TIMER
     *      .TIMER PWM mode can not be paused, because it may leave GPIO as short circuit
     *  @errors
     *      ENODEV
     *      EPERM when TIMER was unconfigured or configured as PWM
     */
extern __attribute__((nothrow))
    int TIMER_stop(int nb);

    /**
     *  TIMER_is_running()
     *  @errors
     *      ENODEV
     */
extern __attribute__((nothrow))
    bool TIMER_is_running(int nb);

    /**
     *  TIMER_tick()
     *      get timer raw tick value
     *  @errors
     *      ENODEV
     */
extern __attribute__((nothrow))
    uint32_t TIMER_tick(int nb);

    /**
     *  TIMER_spin()
     *      timer spin
     *  @errors
     *      ENODEV
     *      EPERM: timer is not running
     */
extern __attribute__((nothrow))
    int TIMER_spin(int nb, uint32_t ticks);

/***************************************************************************
 *  TIMER @free running
 ***************************************************************************/
    /**
     *  TIMER_free_running_configure()
     *      configure timer as free running counter, once TIMER_start()
     *      counter can be reading by TIMER_tick() to measure tick programmly
     *  @errors
     *      ENODEV
     *      EBUSY when TIMER was configured
     */
extern __attribute__((nothrow))
    int TIMER_free_running_configure(int nb, uint32_t const freq);

/***************************************************************************
 *  TIMER @match
 ***************************************************************************/
    /**
     *  TIMER_match_configure(): configure timer as matching callback mode:
     *      .TIMER_match_section() used to append a section
     *      .TIMER_match() used to append matching point of last appended section
     *      .TIMER matching point and section is simulated by software, so
     *          the number is unlimited but add some fixed latency depends on
     *          System Core CLKs, best guess is 2~3μs
     *      .CALLBACK parameter is section'ID or matching'ID
     *
     *      timer may executing very fast in μs level, but threads was driven by
     *    System Core milliseconds level, to executing matching smoothly,
     *    application must cache enough matching point section by section
     *  @errors
     *      ENODEV
     *      EBUSY when TIMER was configured
     */
extern __attribute__((nothrow, nonnull))
    int TIMER_match_configure(int nb, uint32_t const freq, TIMER_callback_t callback);

    /**
     *  TIMER_match_clear(): clear all matching sections and matching points
     *  @errors
     *      ENODEV
     *      EBUSY when TIMER was not stopped
     */
extern __attribute__((nothrow))
    int TIMER_match_clear(int nb);

    /**
     *  TIMER_match_section() attach a new section to matching point
     *      each section can have multiply matching points
     *  @errors
     *      ENODEV
     *      EPERM when TIMER was not configured as matching
     *      ENOMEM
     */
extern __attribute__((nothrow))
    int TIMER_match_section(int nb, uint16_t id, uint32_t repeat);

    /**
     *  TIMER_match() attach a new matching point to current section
     *      attatch big->small to *avoid *iterator the exist matching points
     *  @errors
     *      ENODEV
     *      EPERM when TIMER was not configured as matching
     *      ENOMEM
     */
extern __attribute__((nothrow))
    int TIMER_match(int nb, uint16_t id, uint32_t val, void *arg);

    /**
     *  TIMER_match_cached_sections(): get sections count was scheduled
     */
extern __attribute__((nothrow, pure))
    uint32_t TIMER_match_cached_sections(int nb);

/***************************************************************************
 *  TIMER @PWM
 ***************************************************************************/
    /**
     *  TIMER_PWM_get(): configure timer as PWM mode with optional callback support
     *
     *      .TIMER_PWM_get() used to get nb was associated by PORT'PIN
     *      .TIMER_PWM_update() used to update PWM duty/cycle, this function will
     *          auto starting the timer
     *      .TIMER_PWM_update_duty() use to update PWM duty only, this function
     *          may not auto start the timer depend cycle was setted before
     *      .CALLBACK parameter is PWM's device no
     *  @errors
     *      ENODEV
     *      EBUSY when TIMER was configured
     *      ENOMEM
     *      EINVAL when PORT'PIN can not configure as PWM output
     */
extern __attribute__((nothrow, nonnull(2)))
    int TIMER_PWM_get(uint32_t const freq, void *gpio, uint32_t pin, TIMER_callback_t callback);

    /**
     *  TIMER_PWM_update() / TIMER_PWM_update_duty()
     *  @errors
     *      ENODEV
     *      EPERM when TIMER was not configured as PWM
     */
extern __attribute__((nothrow))
    int TIMER_PWM_update(int nb, uint32_t duty, uint32_t cycle);
extern __attribute__((nothrow))
    int TIMER_PWM_update_duty(int nb, uint32_t duty);

__END_DECLS
#endif
