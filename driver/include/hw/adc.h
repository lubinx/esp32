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
#ifndef __HW_ADC_H
#define __HW_ADC_H                      1

#include <features.h>
#include <stdint.h>

/***************************************************************************
 *  ADC device @alias
 ***************************************************************************/
__BEGIN_DECLS

    typedef struct ADC_attr_t ADC_attr_t;
    typedef void (* ADC_callback_t)(int volt, int raw, void *arg);

    struct ADC_common_attr_t
    {
        struct ADC_common_attr_t *next;
        struct ADC_common_attr_t **iter;

        ADC_callback_t callback;
        void *arg;

        uint16_t q_numerator;
        uint16_t q_denominator;
    };

/***************************************************************************
 *  ADC @attr
 ***************************************************************************/
    /**
     *  ADC_attr_init()
     */
extern __attribute__((nothrow, nonnull(1, 3)))
    int ADC_attr_init(struct ADC_attr_t *attr, uint32_t sps, ADC_callback_t cb);

    /**
     *  ADC_attr_destroy()
     */
extern __attribute__((nothrow, nonnull))
    int ADC_attr_destroy(struct ADC_attr_t *attr);

    /**
     *  ADC_attr_set_resolution()
     */
extern __attribute__((nothrow, nonnull))
    int ADC_attr_set_resolution(struct ADC_attr_t *attr, uint8_t resolution);

    /**
     *  ADC_attr_positive_input()
     *      @param PORT
     *          can be ADC_INTERNAL_PORT for none GPIO port
     *      @param PIN
     *          can be special channel of chip defined
     *      @returns
     *          On Success 0 is returned
     *          On error, errno is returned
     *      @errors
     *          EINVAL
     */
extern __attribute__((nothrow, nonnull(1)))
    int ADC_attr_positive_input(struct ADC_attr_t *attr, uint32_t pin);

    /**
     *  ADC_attr_negative_input()
     *      @param PORT
     *          can be ADC_INTERNAL_PORT for none GPIO port
     *      @param PIN
     *          can be special channel of chip defined
     *      @returns
     *          On Success 0 is returned
     *          On error, errno is returned
     *      @errors
     *          EINVAL
     */
extern __attribute__((nothrow, nonnull(1)))
    int ADC_attr_negative_input(struct ADC_attr_t *attr, uint32_t pin);

    /**
     *  ADC_attr_scale()
     */
extern __attribute__((nothrow, nonnull))
    int ADC_attr_scale(struct ADC_attr_t *attr, uint16_t numerator, uint16_t denominator);

    /**
     *  ADC_attr_vref()
     *      @param sel
     *          @platform selection
     *      @param volt
     *          optional volt value
     */
extern __attribute__((nothrow, nonnull))
    int ADC_attr_vref(struct ADC_attr_t *attr, uint32_t sel, int volt);

/***************************************************************************
 *  ADC @control
 ***************************************************************************/
    /**
     *  ADC_shutdown()
     *      hmm...direct shutdown ADC cause too many trouble
     */
    /*
extern __attribute__((nothrow))
    void ADC_shutdown(void);
    */

    /**
     *  ADC_start_convert()
     */
extern __attribute__((nothrow))
    int ADC_start_convert(struct ADC_attr_t *attr, void *arg);

    /**
     *  ADC_stop_convert()
     */
extern __attribute__((nothrow))
    int ADC_stop_convert(struct ADC_attr_t *attr);

__END_DECLS
#endif
