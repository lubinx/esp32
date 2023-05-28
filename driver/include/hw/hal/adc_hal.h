#ifndef __HW_ADC_HAL_H
#define __HW_ADC_HAL_H                  1

#include <features.h>
#include <stdbool.h>

#include "../adc.h"

__BEGIN_DECLS

/***************************************************************************
 *  ADC HAL configure
 ***************************************************************************/
extern __attribute__((nothrow))
    int ADC_HAL_attr_init(struct ADC_attr_t *attr, uint32_t sps);

extern __attribute__((nothrow))
    int ADC_HAL_attr_destroy(struct ADC_attr_t *attr);

/***************************************************************************
 *  ADC HAL power reference
 ***************************************************************************/
extern __attribute__((nothrow))
    void ADC_HAL_power_ref(void);

extern __attribute__((nothrow))
    void ADC_HAL_power_release(void);

/***************************************************************************
 *  ADC HAL control
 ***************************************************************************/
extern __attribute__((nothrow))
    int ADC_HAL_start(struct ADC_attr_t *attr);

extern __attribute__((nothrow))
    void ADC_HAL_stop(void);

/***************************************************************************
 *  ADC @interrupt callback
 ***************************************************************************/
extern __attribute__((nothrow))
    void ADC_HAL_intr_callback(int v, int raw);

__END_DECLS
#endif
