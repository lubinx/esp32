#ifndef __ESP32S3_GPIO_H
#define __ESP32S3_GPIO_H                1

#include <features.h>
#include <hw/gpio.h>

    #define GPIO_L                      ((void *)1UL << 30) // PIN:  0~21
    #define GPIO_H                      ((void *)1UL << 31) // PIN: 26~48

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

    // #define GPIO0_VALID_MASK            ((1 << 22) - 1)
    // #define GPIO1_VALID_MASK            ((1 << (48 - 26 + 1)) - 1)

    enum iomux_sel
    {
        IOMUX_SEL0                = 0,
        IOMUX_SEL1,
        IOMUX_SEL2,
        IOMUX_SEL3,
        IOMUX_SEL4,
        // alias
        IOMUX_SEL_GPIO = IOMUX_SEL1,
    };

    enum iomux_def
    {
        // SPI
        IOMUX_SPICLK        = (30 << 4 | IOMUX_SEL0),
        IOMUX_SPICS0        = (29 << 4 | IOMUX_SEL0),
        IOMUX_SPICS1        = (26 << 4 | IOMUX_SEL0),
        IOMUX_SPIWP         = (28 << 4 | IOMUX_SEL0),
        IOMUX_SPIHD         = (27 << 4 | IOMUX_SEL0),
        IOMUX_SPIQ          = (31 << 4 | IOMUX_SEL0),
        IOMUX_SPID          = (32 << 4 | IOMUX_SEL0),
        // SPI IO4~7/DQS
        IOMUX_SPIIO4        = (33 << 4 | IOMUX_SEL4),
        IOMUX_SPIIO5        = (34 << 4 | IOMUX_SEL4),
        IOMUX_SPIIO6        = (35 << 4 | IOMUX_SEL4),
        IOMUX_SPIIO7        = (36 << 4 | IOMUX_SEL4),
        IOMUX_SPIDQS        = (37 << 4 | IOMUX_SEL4),
        // UART0
        IOMUX_UART0_RTS     = (15 << 4 | IOMUX_SEL2),
        IOMUX_UART0_CTS     = (16 << 4 | IOMUX_SEL2),
        IOMUX_UART0_TXD     = (43 << 4 | IOMUX_SEL0),
        IOMUX_UART0_RXD     = (44 << 4 | IOMUX_SEL0),
        // UART1
        IOMUX_UART1_TXD     = (17 << 4 | IOMUX_SEL2),
        IOMUX_UART1_RXD     = (18 << 4 | IOMUX_SEL2),
        IOMUX_UART1_RTS     = (19 << 4 | IOMUX_SEL2),
        IOMUX_UART1_CTS     = (20 << 4 | IOMUX_SEL2),
        //
        IOMUX_MTCK          = (39 << 4 | IOMUX_SEL0),
        IOMUX_MTDO          = (40 << 4 | IOMUX_SEL0),
        IOMUX_MTDI          = (41 << 4 | IOMUX_SEL0),
        IOMUX_MTMS          = (42 << 4 | IOMUX_SEL0),
        //
        IOMUX_SPICLK_P_DIFF = (47 << 4 | IOMUX_SEL0),
        IOMUX_SPICLK_N_DIFF = (48 << 4 | IOMUX_SEL0),
        // CLK OUT 1/2/3
        IOMUX_P20_CLK_OUT1  = (20 << 4 | IOMUX_SEL3),
        IOMUX_P41_CLK_OUT1  = (41 << 4 | IOMUX_SEL2),
        IOMUX_P43_CLK_OUT1  = (43 << 4 | IOMUX_SEL2),
        IOMUX_P19_CLK_OUT2  = (19 << 4 | IOMUX_SEL3),
        IOMUX_P40_CLK_OUT2  = (40 << 4 | IOMUX_SEL2),
        IOMUX_P44_CLK_OUT2  = (44 << 4 | IOMUX_SEL2),
        IOMUX_P18_CLK_OUT3  = (18 << 4 | IOMUX_SEL3),
        IOMUX_P39_CLK_OUT3  = (39 << 4 | IOMUX_SEL2),
        // SUB SPI?
        IOMUX_SUBSPICLK_P12 = (12 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPICLK_P36 = (36 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPICS0_P10 = (10 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPICS0_P34 = (34 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPICS1_P08 = ( 8 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPICS1_P39 = (39 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPIWP_P14  = (14 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPIWP_P38  = (38 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPIHD_P09  = ( 9 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPIHD_P33  = (33 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPID_P11   = (11 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPID_P35   = (35 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPIQ_P13   = (13 << 4 | IOMUX_SEL3),
        IOMUX_SUBSPIQ_P37   = (37 << 4 | IOMUX_SEL3),
        // F SPI?
        IOMUX_P12_FSPICLK   = (12 << 4 | IOMUX_SEL4),
        IOMUX_P36_FSPICLK   = (36 << 4 | IOMUX_SEL2),
        IOMUX_P10_FSPICS0   = (10 << 4 | IOMUX_SEL4),
        IOMUX_P34_FSPICS0   = (34 << 4 | IOMUX_SEL2),
        IOMUX_P14_FSPIWP    = (14 << 4 | IOMUX_SEL4),
        IOMUX_P38_FSPIWP    = (38 << 4 | IOMUX_SEL2),
        IOMUX_P09_FSPIHD    = ( 9 << 4 | IOMUX_SEL4),
        IOMUX_P33_FSPIHD    = (33 << 4 | IOMUX_SEL2),
        IOMUX_P11_FSPID     = (11 << 4 | IOMUX_SEL4),
        IOMUX_P35_FSPID     = (35 << 4 | IOMUX_SEL2),
        IOMUX_P13_FSPIQ     = (13 << 4 | IOMUX_SEL4),
        IOMUX_P37_FSPIQ     = (37 << 4 | IOMUX_SEL2),
        // F SPI IO4~7/DQS
        IOMUX_FSPIIO4       = (10 << 4 | IOMUX_SEL2),
        IOMUX_FSPIIO5       = (11 << 4 | IOMUX_SEL2),
        IOMUX_FSPIIO6       = (12 << 4 | IOMUX_SEL2),
        IOMUX_FSPIIO7       = (13 << 4 | IOMUX_SEL2),
        IOMUX_FSPIDQS       = (14 << 4 | IOMUX_SEL2),
    };

__BEGIN_DECLS
/****************************************************************************
 *  initialization: called by SOC_initialize()
 ****************************************************************************/
extern __attribute__((nothrow, nonnull))
    void GPIO_initialize(void);

/***************************************************************************/
/**  IO mux & matrix
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int IOMUX_configure(enum iomux_def mux);

extern __attribute__((nothrow, nonnull))
    int IOMUX_deconfigure(enum iomux_def mux);

static inline
    uint8_t IOMUX_pin_nb(enum iomux_def mux)
    {
        return (uint8_t)(mux >> 4);
    }

    /**
     *  IOMUX_matrix_route_input()
     *      replace esp_rom_IOMUX_matrix_route_input()
     *      connect pin_nb to input matrix
     *  @param inv input is inverted
    */
extern __attribute__((nothrow, nonnull))
    int IOMUX_matrix_route_input(uint8_t pin_nb, uint16_t sig_idx, bool inv);

    /**
     *  IOMUX_matrix_route_output()
     *      replace esp_rom_IOMUX_matrix_route_output()
     *      connect pin_jb to output matrix
     *  @param inv output is inverted
     *  @param oen_inv output enable control is inverted
     */
extern __attribute__((nothrow, nonnull))
    int IOMUX_matrix_route_output(uint8_t pin_nb, uint16_t sig_idx, bool inv, bool oen_inv);

    /**
     *  IOMUX_matrix_disconnect()
     *      disconnect pin_nb from matrix(input & output)
     */
extern __attribute__((nothrow, nonnull))
    int IOMUX_matrix_disconnect(uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    void IOMUX_print(void);

/***************************************************************************/
/**  GPIO by PIN number
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int GPIO_disable_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_input_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp, bool filter_en);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_output_pin_nb(uint8_t pin_nb, enum GPIO_output_mode_t mode);

extern __attribute__((nothrow, nonnull))
    int GPIO_output_set_pin_nb(uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_output_clear_pin_nb(uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_output_peek_pin_nb(uint8_t pin_nb);

extern __attribute__((nothrow, nonnull))
    int GPIO_output_toggle_pin_nb(uint8_t pin_nb);

__END_DECLS
#endif
