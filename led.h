#ifndef __LED_DRV_H
#define __LED_DRV_H

#include <features.h>

#include <time.h>
#include <gpio.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    void LED_init(int i2c_nb, uint8_t da);

static inline
    void LED_init_default(void) { LED_init(0, 0x73); }

extern __attribute__((nothrow))
    void LED_test(void);

extern __attribute__((nothrow))
    void LED_pwm(uint8_t val);

extern __attribute__((nothrow))
    void LED_update_clock(void);

extern __attribute__((nothrow))
    void LED_update_tmpr(int tmpr);

extern __attribute__((nothrow))
    void LED_update_humidity(int humidity);

__END_DECLS
#endif
