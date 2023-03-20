#ifndef __ESP32S3_GPIO_H
#define __ESP32S3_GPIO_H                1

#include <features.h>
#include <hw/gpio.h>

    #define GPIO_H                      ((void *)1UL << 31) // PIN: 26~48
    #define GPIO_L                      ((void *)1UL << 30) // PIN:  0~21

// PIN0 ~ PIN21
    #define PIN0                        ((uintptr_t)GPIO_L | (1 << 0))
    #define PIN1                        ((uintptr_t)GPIO_L | (1 << 1))
    #define PIN2                        ((uintptr_t)GPIO_L | (1 << 2))
    #define PIN3                        ((uintptr_t)GPIO_L | (1 << 3))
    #define PIN4                        ((uintptr_t)GPIO_L | (1 << 4))
    #define PIN5                        ((uintptr_t)GPIO_L | (1 << 5))
    #define PIN6                        ((uintptr_t)GPIO_L | (1 << 6))
    #define PIN7                        ((uintptr_t)GPIO_L | (1 << 7))
    #define PIN8                        ((uintptr_t)GPIO_L | (1 << 8))
    #define PIN9                        ((uintptr_t)GPIO_L | (1 << 9))
    #define PIN10                       ((uintptr_t)GPIO_L | (1 << 10))
    #define PIN11                       ((uintptr_t)GPIO_L | (1 << 11))
    #define PIN12                       ((uintptr_t)GPIO_L | (1 << 12))
    #define PIN13                       ((uintptr_t)GPIO_L | (1 << 13))
    #define PIN14                       ((uintptr_t)GPIO_L | (1 << 14))
    #define PIN15                       ((uintptr_t)GPIO_L | (1 << 15))
    #define PIN16                       ((uintptr_t)GPIO_L | (1 << 16))
    #define PIN17                       ((uintptr_t)GPIO_L | (1 << 17))
    #define PIN18                       ((uintptr_t)GPIO_L | (1 << 18))
    #define PIN19                       ((uintptr_t)GPIO_L | (1 << 19))
    #define PIN20                       ((uintptr_t)GPIO_L | (1 << 20))
    #define PIN21                       ((uintptr_t)GPIO_L | (1 << 21))

// PIN26 ~ PIN48
    #define PIN26                       ((uintptr_t)GPIO_H | 1 << 0)
    #define PIN27                       ((uintptr_t)GPIO_H | 1 << 1)
    #define PIN28                       ((uintptr_t)GPIO_H | 1 << 2)
    #define PIN29                       ((uintptr_t)GPIO_H | 1 << 3)
    #define PIN30                       ((uintptr_t)GPIO_H | 1 << 4)
    #define PIN31                       ((uintptr_t)GPIO_H | 1 << 5)
    #define PIN32                       ((uintptr_t)GPIO_H | 1 << 6)
    #define PIN33                       ((uintptr_t)GPIO_H | 1 << 7)
    #define PIN34                       ((uintptr_t)GPIO_H | 1 << 8)
    #define PIN35                       ((uintptr_t)GPIO_H | 1 << 9)
    #define PIN36                       ((uintptr_t)GPIO_H | 1 << 10)
    #define PIN37                       ((uintptr_t)GPIO_H | 1 << 11)
    #define PIN38                       ((uintptr_t)GPIO_H | 1 << 12)
    #define PIN39                       ((uintptr_t)GPIO_H | 1 << 13)
    #define PIN40                       ((uintptr_t)GPIO_H | 1 << 14)
    #define PIN41                       ((uintptr_t)GPIO_H | 1 << 15)
    #define PIN42                       ((uintptr_t)GPIO_H | 1 << 16)
    #define PIN43                       ((uintptr_t)GPIO_H | 1 << 17)
    #define PIN44                       ((uintptr_t)GPIO_H | 1 << 18)
    #define PIN45                       ((uintptr_t)GPIO_H | 1 << 19)
    #define PIN46                       ((uintptr_t)GPIO_H | 1 << 20)
    #define PIN47                       ((uintptr_t)GPIO_H | 1 << 21)
    #define PIN48                       ((uintptr_t)GPIO_H | 1 << 22)

    #define GPIO0_VALID_MASK            ((1 << 22) - 1)
    #define GPIO1_VALID_MASK            ((1 << (48 - 26 + 1)) - 1)

    // default pull-up when gpio is disabled for powersave
    #define GPIO_DISABLE_PULL           (PULL_UP)

__BEGIN_DECLS
/***************************************************************************/
/**  GPIO io mux table console
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    void GPIO_print_iomux(void);

/***************************************************************************/
/**  GPIO configure by PIN number
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int GPIO_disable_pin_nb(enum GPIO_pad_pull_t pull, uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_input_pin_nb(uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_output_pin_nb(enum GPIO_output_mode_t mode, uint8_t pin_nb);

__END_DECLS
#endif
