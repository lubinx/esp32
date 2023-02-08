#include <sys/types.h>
#include <semaphore.h>
#include <sys/errno.h>

#include "soc/soc_caps.h"
#include "clk_tree.h"

#include "uart.h"

/****************************************************************************
 *  @declaration
 ****************************************************************************/
#define UART_RX_BUFFER_SIZE             128

struct UART_context
{
    uart_dev_t *dev;

    sem_t read_rdy;
    sem_t write_rdy;

    // tx write to ptr
    uint8_t *tx_ptr;
    uint32_t tx_pos, tx_size;

    // cycle queue read/write position
    uint32_t rx_que_rpos, rx_que_wpos;
    // cycle queue of input buffer
    uint8_t *rx_que;
};

typedef enum {
    UART_MODE_UART = 0x00,                      /*!< mode: regular UART mode*/
    UART_MODE_RS485_HALF_DUPLEX = 0x01,         /*!< mode: half duplex RS485 UART mode control by RTS pin */
    UART_MODE_IRDA = 0x02,                      /*!< mode: IRDA  UART mode*/
    UART_MODE_RS485_COLLISION_DETECT = 0x03,    /*!< mode: RS485 collision detection UART mode (used for test purposes)*/
    UART_MODE_RS485_APP_CTRL = 0x04,            /*!< mode: application control RS485 UART mode (used for test purposes)*/
} uart_mode_t;

/**
 * @brief UART word length constants
 */
typedef enum {
    UART_DATA_5_BITS   = 0x0,    /*!< word length: 5bits*/
    UART_DATA_6_BITS   = 0x1,    /*!< word length: 6bits*/
    UART_DATA_7_BITS   = 0x2,    /*!< word length: 7bits*/
    UART_DATA_8_BITS   = 0x3,    /*!< word length: 8bits*/
    UART_DATA_BITS_MAX = 0x4,
} uart_word_length_t;

/**
 * @brief UART hardware flow control modes
 */
typedef enum {
    UART_HW_FLOWCTRL_DISABLE = 0x0,   /*!< disable hardware flow control*/
    UART_HW_FLOWCTRL_RTS     = 0x1,   /*!< enable RX hardware flow control (rts)*/
    UART_HW_FLOWCTRL_CTS     = 0x2,   /*!< enable TX hardware flow control (cts)*/
    UART_HW_FLOWCTRL_CTS_RTS = 0x3,   /*!< enable hardware flow control*/
    UART_HW_FLOWCTRL_MAX     = 0x4,
} uart_hw_flowcontrol_t;

/**
 * @brief UART signal bit map
 */
typedef enum {
    UART_SIGNAL_INV_DISABLE  =  0,            /*!< Disable UART signal inverse*/
    UART_SIGNAL_IRDA_TX_INV  = (0x1 << 0),    /*!< inverse the UART irda_tx signal*/
    UART_SIGNAL_IRDA_RX_INV  = (0x1 << 1),    /*!< inverse the UART irda_rx signal*/
    UART_SIGNAL_RXD_INV      = (0x1 << 2),    /*!< inverse the UART rxd signal*/
    UART_SIGNAL_CTS_INV      = (0x1 << 3),    /*!< inverse the UART cts signal*/
    UART_SIGNAL_DSR_INV      = (0x1 << 4),    /*!< inverse the UART dsr signal*/
    UART_SIGNAL_TXD_INV      = (0x1 << 5),    /*!< inverse the UART txd signal*/
    UART_SIGNAL_RTS_INV      = (0x1 << 6),    /*!< inverse the UART rts signal*/
    UART_SIGNAL_DTR_INV      = (0x1 << 7),    /*!< inverse the UART dtr signal*/
} uart_signal_inv_t;


/****************************************************************************
 *  @internal
 ****************************************************************************/
// constructor
static int UART_init_context(struct UART_context *context);
// configure
static uint64_t UART_sclk_freq(uart_dev_t *dev);
// io
static ssize_t USART_read(int fd, void *buf, size_t bufsize);
static ssize_t USART_write(int fd, void const *buf, size_t count);
static int USART_close(int fd);

// var
static struct UART_context uart_context[SOC_UART_NUM] = {0};

/****************************************************************************
 *  @implements
 ****************************************************************************/
__attribute__((constructor))
void UART_init(void)
{
    uart_context[0].dev = &UART0;
    uart_context[1].dev = &UART1;
    uart_context[2].dev = &UART2;
}

int UART_createfd(int nb, uint32_t bps, parity_t parity, uint8_t stop_bits)
{
    if (SOC_UART_NUM < (unsigned)nb)
        return __set_errno_neg(EINVAL);

    struct UART_context *context = &uart_context[nb];
    if (NULL != context->rx_que)
        return __set_errno_neg(EBUSY);

    int retval = UART_init_context(context);
    if (0 == retval)
    {
        uart_dev_t *dev = context->dev;
        retval = UART_configure(dev, dev->clk_conf.sclk_sel, bps, parity, stop_bits);
    }

    if (0 == retval)
        return retval;
    else
        return __set_errno_neg(retval);
}

int UART_io_mutex(int RXD, int TXD, int RTS, int CTS)
{
}

int UART_configure(uart_dev_t *dev, soc_uart_sclk_sel_t sclk, uint32_t bps, parity_t parity, uint8_t stop_bits)
{
    int retval;
    periph_module_t uart_module;

    if (&UART0 == dev)
        uart_module = PERIPH_UART0_MODULE;
    else if (&UART1 == dev)
        uart_module = PERIPH_UART1_MODULE;
    else if (&UART2 == dev)
        uart_module = PERIPH_UART2_MODULE;
    else
        return ENODEV;

    clk_tree_module_enable(PERIPH_UART0_MODULE);
    clk_tree_module_reset(PERIPH_UART0_MODULE);

    switch (stop_bits)
    {
    default:
        retval = EINVAL;
        goto uart_configure_fail_exit;
    case 1:
        dev->conf0.stop_bit_num = 1;
        break;
    case 2:
        dev->conf0.stop_bit_num = 3;
        break;
    /*
        1.5 stopbits?
        dev->conf0.stop_bit_num = 2;
    */
    }

    if (paNone != parity)
    {
        switch ((parity))
        {
        default:
            retval = EINVAL;
            goto uart_configure_fail_exit;
        case paNone:
            dev->conf0.parity_en = 0;
            break;
        case paOdd:
            dev->conf0.parity = 1;
            dev->conf0.parity_en = 1;
            break;
        case paEven:
            dev->conf0.parity = 0;
            dev->conf0.parity_en = 1;
            break;
        }
    }
    /*
        0 = 5bits
        1 = 6bits
        2 = 7bits
        3 = 8bits
    */
    dev->conf0.bit_num = 3;

    dev->clk_conf.sclk_sel = sclk;

    if (false)
    {
uart_configure_fail_exit:
        clk_tree_module_disable(uart_module);
    }
    return retval;
}

int UART_sclk_sel(uart_dev_t *dev, enum soc_uart_sclk_sel_t sel)
{
    dev->clk_conf.sclk_sel = sel;
}

uint32_t UART_get_baudrate(uart_dev_t *dev)
{
    return (UART_sclk_freq(dev) << 4) / (
        ((dev->clkdiv.clkdiv << 4) | dev->clkdiv.clkdiv_frag) *  (dev->clk_conf.sclk_div_num + 1)
    );
}

/****************************************************************************
 *  @internal
 ****************************************************************************/
static int UART_init_context(struct UART_context *context)
{
    uint8_t *rx_buf = malloc(UART_RX_BUFFER_SIZE);

    if (NULL != rx_buf)
    {
        sem_init(&context->read_rdy, 0, UART_RX_BUFFER_SIZE);
        sem_init(&context->write_rdy, 0, 1);
        context->rx_que = rx_buf;

        return 0;
    }
    else
        return ENOMEM;
}

static uint64_t UART_sclk_freq(uart_dev_t *dev)
{
    switch(dev->clk_conf.sclk_sel)
    {
    case SOC_UART_CLK_SRC_APB:
        return clk_tree_apb_freq();
    case SOC_UART_CLK_SRC_RC_FAST:
        return clk_tree_sclk_freq(SOC_SCLK_INT_RC_FAST);
    case SOC_UART_CLK_SRC_XTAL:
        return clk_tree_sclk_freq(SOC_SCLK_XTAL);
    }
}
