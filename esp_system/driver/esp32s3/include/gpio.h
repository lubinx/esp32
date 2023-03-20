#ifndef __ESP32S3_GPIO_H
#define __ESP32S3_GPIO_H                1

#include <hw/gpio.h>

    #define GPIO0                       0   // PIN:  0~21
    #define GPIO1                       26  // PIN: 26~48

// PIN0 ~ PIN21
    #define PIN0                        (1 << 0)
    #define PIN1                        (1 << 1)
    #define PIN2                        (1 << 2)
    #define PIN3                        (1 << 3)
    #define PIN4                        (1 << 4)
    #define PIN5                        (1 << 5)
    #define PIN6                        (1 << 6)
    #define PIN7                        (1 << 7)
    #define PIN8                        (1 << 8)
    #define PIN9                        (1 << 9)
    #define PIN10                       (1 << 10)
    #define PIN11                       (1 << 11)
    #define PIN12                       (1 << 12)
    #define PIN13                       (1 << 13)
    #define PIN14                       (1 << 14)
    #define PIN15                       (1 << 15)
    #define PIN16                       (1 << 16)
    #define PIN17                       (1 << 17)
    #define PIN18                       (1 << 18)
    #define PIN19                       (1 << 19)
    #define PIN20                       (1 << 20)
    #define PIN21                       (1 << 21)

// PIN26 ~ PIN48
    #define PIN26                       (1 << 26)
    #define PIN27                       (1 << 27)
    #define PIN28                       (1 << 28)
    #define PIN29                       (1 << 29)
    #define PIN30                       (1 << 30)
    #define PIN31                       (1 << 31)
    #define PIN32                       (1 << 32)
    #define PIN33                       (1 << 33)
    #define PIN34                       (1 << 34)
    #define PIN35                       (1 << 35)
    #define PIN36                       (1 << 36)
    #define PIN37                       (1 << 37)
    #define PIN38                       (1 << 38)
    #define PIN39                       (1 << 39)
    #define PIN40                       (1 << 40)
    #define PIN41                       (1 << 41)
    #define PIN42                       (1 << 42)
    #define PIN43                       (1 << 43)
    #define PIN44                       (1 << 44)
    #define PIN45                       (1 << 45)
    #define PIN46                       (1 << 46)
    #define PIN47                       (1 << 47)
    #define PIN48                       (1 << 48)

    #define GPIO0_VALID_MASK            ((1 << 22) - 1)
    #define GPIO1_VALID_MASK            ((1 << (48 - 26 + 1)) - 1)

    // default pull-up when gpio is disabled for powersave
    #define GPIO_DISABLE_PULL           (PULL_UP)

/***************************************************************************/
/**  GPIO configure by PIN number
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int GPIO_disable_pin_nb(enum GPIO_resistor_t pull, uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_input_pin_nb(uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_output_pin_nb(enum GPIO_output_mode_t mode, uint8_t pin_nb);

#endif
