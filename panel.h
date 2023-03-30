#ifndef __PANEL_DRV_H
#define __PANEL_DRV_H

#include <features.h>

#include <time.h>
#include <gpio.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    void PANEL_init(int i2c_nb, uint8_t da);

static inline
    void PANEL_init_default(void) { PANEL_init(0, 0x73); }

extern __attribute__((nothrow))
    void PANEL_test(void);

extern __attribute__((nothrow))
    void PANEL_pwm(uint8_t val);

extern __attribute__((nothrow))
    void PANEL_update_tmpr(int tmpr);

extern __attribute__((nothrow))
    void PANEL_update_humidity(int humidity);

__END_DECLS
#endif
