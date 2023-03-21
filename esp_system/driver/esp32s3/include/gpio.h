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

    #define GPIO0_VALID_MASK            ((1 << 22) - 1)
    #define GPIO1_VALID_MASK            ((1 << (48 - 26 + 1)) - 1)

    // default pull-up when gpio is disabled for powersave
    #define GPIO_DISABLE_PULL           (HIGH_Z)

    // set matrix signal
    #define SIG_BYPASS_IDX              (~0)

__BEGIN_DECLS
/****************************************************************************
 *  initialization: called by SOC_initialize()
 ****************************************************************************/
extern __attribute__((nothrow, nonnull))
    void GPIO_initialize(void);

/***************************************************************************/
/**  GPIO io mux table console
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    void GPIO_print_iomux(void);

/***************************************************************************/
/**  GPIO configure by PIN number
****************************************************************************/
extern __attribute__((nothrow, nonnull))
    int GPIO_disable_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_input_pin_nb(uint8_t pin_nb, enum GPIO_pad_pull_t pp, bool filter_en);

extern __attribute__((nothrow, nonnull))
    int GPIO_setdir_output_pin_nb(uint8_t pin_nb, enum GPIO_output_mode_t mode);

    /**
     *  GPIO_connect_in_signal()
     *      replace esp_rom_gpio_connect_in_signal()
     *      connect pin_nb to input matrix
     *  @param inv input is inverted
    */
extern __attribute__((nothrow, nonnull))
    int GPIO_connect_in_signal(uint8_t pin_nb, uint16_t sig_idx, bool inv);

    /**
     *  GPIO_connect_out_signal()
     *      replace esp_rom_gpio_connect_out_signal()
     *      connect pin_jb to output matrix
     *  @param inv output is inverted
     *  @param oen_inv output enable control is inverted
     */
extern __attribute__((nothrow, nonnull))
    int GPIO_connect_out_signal(uint8_t pin_nb, uint16_t sig_idx, bool inv, bool oen_inv);


    enum io_mux_pin
    {
        IO_MUX_FUNC0                = 0,
        IO_MUX_FUNC1,
        IO_MUX_FUNC2,
        IO_MUX_FUNC3,
        IO_MUX_FUNC4,
        // alias
        IO_MUX_DEF = IO_MUX_FUNC0,
        IO_MUX_GPIO = IO_MUX_FUNC1,
        //------------------------------------------------------------------------------------------------------------------------------
        //  FUNCTION 0/2                                FUNCTION 3                                      FUNCTION 4
        //------------------------------------------------------------------------------------------------------------------------------
        SPICS1_TO_P26  = (26 << 4 | IO_MUX_FUNC0),
        SPIHD_TO_P27   = (27 << 4 | IO_MUX_FUNC0),
        SPIWP_TO_P28   = (28 << 4 | IO_MUX_FUNC0),
        SPICS0_TO_P29  = (29 << 4 | IO_MUX_FUNC0),
        SPICLK_TO_P30  = (30 << 4 | IO_MUX_FUNC0),
        SPIQ_TO_P31    = (31 << 4 | IO_MUX_FUNC0),
        SPID_TO_P32    = (32 << 4 | IO_MUX_FUNC0),
        MTCK_TO_P39    = (39 << 4 | IO_MUX_FUNC0),
        MTDO_TO_P40    = (40 << 4 | IO_MUX_FUNC0),
        MTDI_TO_P41    = (41 << 4 | IO_MUX_FUNC0),
        MTMS_TO_P42    = (42 << 4 | IO_MUX_FUNC0),
        U0TXD_TO_P43   = (43 << 4 | IO_MUX_FUNC0),
        U0RXD_TO_P44   = (44 << 4 | IO_MUX_FUNC0),
        SPICLK_P_DIFF_TO_P47 = (47 << 4 | IO_MUX_FUNC0),
        SPICLK_N_DIFF_TO_P48 = (48 << 4 | IO_MUX_FUNC0),
                                                        SUBSPICS1_TO_P08 = ( 8 << 4 | IO_MUX_FUNC3),
                                                        SUBSPIHD_TO_P09  = ( 9 << 4 | IO_MUX_FUNC3),    FSPIHD_TO_P09    = ( 9 << 4 | IO_MUX_FUNC4),
        FSPIIO4_TO_P10 = (10 << 4 | IO_MUX_FUNC2),      SUBSPICS0_TO_P10 = (10 << 4 | IO_MUX_FUNC3),    FSPICS0_TO_P10   = (10 << 4 | IO_MUX_FUNC4),
        FSPIIO5_TO_P11 = (11 << 4 | IO_MUX_FUNC2),      SUBSPID_TO_P11   = (11 << 4 | IO_MUX_FUNC3),    FSPID_TO_P11     = (11 << 4 | IO_MUX_FUNC4),
        FSPIIO6_TO_P12 = (12 << 4 | IO_MUX_FUNC2),      SUBSPICLK_TO_P12 = (12 << 4 | IO_MUX_FUNC3),    FSPICLK_TO_P12   = (12 << 4 | IO_MUX_FUNC4),
        FSPIIO7_TO_P13 = (13 << 4 | IO_MUX_FUNC2),      SUBSPIQ_TO_P13   = (13 << 4 | IO_MUX_FUNC3),    FSPIQ_TO_P13     = (13 << 4 | IO_MUX_FUNC4),
        FSPIDQS_TO_P14 = (14 << 4 | IO_MUX_FUNC2),      SUBSPIWP_TO_P14  = (14 << 4 | IO_MUX_FUNC3),    FSPIWP_TO_P14    = (14 << 4 | IO_MUX_FUNC4),
        U0RTS_TO_P15   = (15 << 4 | IO_MUX_FUNC2),
        U0CTS_TO_P16   = (16 << 4 | IO_MUX_FUNC2),
        U1TXD_TO_P17   = (17 << 4 | IO_MUX_FUNC2),
        U1RXD_TO_P18   = (18 << 4 | IO_MUX_FUNC2),      CLK_OUT3_TO_P18  = (18 << 4 | IO_MUX_FUNC3),
        U1RTS_TO_P19   = (19 << 4 | IO_MUX_FUNC2),      CLK_OUT2_TO_P19  = (19 << 4 | IO_MUX_FUNC3),
        U1CTS_TO_P20   = (20 << 4 | IO_MUX_FUNC2),      CLK_OUT1_TO_P20  = (20 << 4 | IO_MUX_FUNC3),
        FSPIHD_TO_P33  = (33 << 4 | IO_MUX_FUNC2),      SUBSPIHD_TO_P33  = (33 << 4 | IO_MUX_FUNC3),    SPIIO4_TO_P33    = (33 << 4 | IO_MUX_FUNC4),
        FSPICS0_TO_P34 = (34 << 4 | IO_MUX_FUNC2),      SUBSPICS0_TO_P34 = (34 << 4 | IO_MUX_FUNC3),    SPIIO5_TO_P34    = (34 << 4 | IO_MUX_FUNC4),
        FSPID_TO_P35   = (35 << 4 | IO_MUX_FUNC2),      SUBSPID_TO_P35   = (35 << 4 | IO_MUX_FUNC3),    SPIIO6_TO_P35    = (35 << 4 | IO_MUX_FUNC4),
        FSPICLK_TO_P36 = (36 << 4 | IO_MUX_FUNC2),      SUBSPICLK_TO_P36 = (36 << 4 | IO_MUX_FUNC3),    SPIIO7_TO_P36    = (36 << 4 | IO_MUX_FUNC4),
        FSPIQ_TO_P37   = (37 << 4 | IO_MUX_FUNC2),      SUBSPIQ_TO_P37   = (37 << 4 | IO_MUX_FUNC3),    SPIDQS_TO_P37    = (37 << 4 | IO_MUX_FUNC4),
        FSPIWP_TO_P38  = (38 << 4 | IO_MUX_FUNC2),      SUBSPIWP_TO_P38  = (38 << 4 | IO_MUX_FUNC3),
        CLK_OUT3_TO_P39 = (39 << 4 | IO_MUX_FUNC2),     SUBSPICS1_TO_P39  = (39 << 4 | IO_MUX_FUNC3),
        CLK_OUT2_TO_P40 = (40 << 4 | IO_MUX_FUNC2),
        CLK_OUT1_TO_P41 = (41 << 4 | IO_MUX_FUNC2),
        CLK_OUT1_TO_P43 = (43 << 4 | IO_MUX_FUNC2),
        CLK_OUT2_TO_P44 = (44 << 4 | IO_MUX_FUNC2),
    };

    /**
     *  GPIO_disconnect_signal()
     *      disconnect pin_nb from matrix(input & output)
     */
extern __attribute__((nothrow, nonnull))
    int GPIO_disconnect_signal(uint8_t pin_nb, enum io_mux_pin mux);

__END_DECLS
#endif
