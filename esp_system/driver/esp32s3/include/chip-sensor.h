#ifndef __ESP32S3_SENSOR_H
#define __ESP32S3_SENSOR_H              1

#include <features.h>
#include "soc/sens_struct.h"

enum SAR_ADC_power_t
{
    SAR_CTRL_LL_POWER_FSM,
    SAR_CTRL_LL_POWER_ON,
    SAR_CTRL_LL_POWER_OFF,
} ;

__BEGIN_DECLS

static inline
    void SAR_ADC_power(enum SAR_ADC_power_t mode)
    {
        switch(mode)
        {
        case SAR_CTRL_LL_POWER_FSM:
            SENS.sar_peri_clk_gate_conf.saradc_clk_en = 1;
            SENS.sar_power_xpd_sar.force_xpd_sar = 0x0;
            break;
        case SAR_CTRL_LL_POWER_ON:
            SENS.sar_peri_clk_gate_conf.saradc_clk_en = 1;
            SENS.sar_power_xpd_sar.force_xpd_sar = 0x3;
            break;
        default:
        case SAR_CTRL_LL_POWER_OFF:
            SENS.sar_peri_clk_gate_conf.saradc_clk_en = 0;
            SENS.sar_power_xpd_sar.force_xpd_sar = 0x2;
        }
    }

__END_DECLS
#endif
