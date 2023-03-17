#ifndef __ESP32S3_GPIO_H
#define __ESP32S3_GPIO_H                1

#include <hw/gpio.h>

    #define GPIO0                       0   // PIN:  0~21
    #define GPIO1                       26  // PIN: 26~48

    #define PIN0                        (1 << 0)
    #define PIN1                        (1 << 0)
    #define PIN2                        (1 << 0)
    #define PIN3                        (1 << 0)
    #define PIN4                        (1 << 0)
    #define PIN5                        (1 << 0)
    #define PIN6                        (1 << 0)
    #define PIN7                        (1 << 0)
    #define PIN8                        (1 << 0)
    #define PIN9                        (1 << 0)
    #define PIN10                       (1 << 0)
    #define PIN11                       (1 << 0)
    #define PIN12                       (1 << 0)
    #define PIN13                       (1 << 0)
    #define PIN14                       (1 << 0)
    #define PIN15                       (1 << 0)
    #define PIN16                       (1 << 0)
    #define PIN17                       (1 << 0)
    #define PIN18                       (1 << 0)
    #define PIN19                       (1 << 0)
    #define PIN20                       (1 << 0)
    #define PIN21                       (1 << 0)

    #define GPIO0_VALID_MASK            ((1 << 22) - 1)
    #define GPIO1_VALID_MASK            ((1 << (48 - 26 + 1)) - 1)

#endif
