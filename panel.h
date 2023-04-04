#ifndef __PANEL_DRV_H
#define __PANEL_DRV_H

#include <features.h>

#include <time.h>
#include <gpio.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    void PANEL_init(void);

extern __attribute__((nothrow))
    void PANEL_test(void);

extern __attribute__((nothrow))
    int PANEL_pwm(uint8_t val);

    enum tmpr_degree_t
    {
        CELSIUS,
        FAHRENHEIT
    };

extern __attribute__((nothrow))
    void PANEL_update_tmpr(int tmpr, enum tmpr_degree_t deg);

extern __attribute__((nothrow))
    void PANEL_update_humidity(int humidity);

__END_DECLS
#endif
