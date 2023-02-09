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

enum
{
    UART_MODE_UART              = 0,
    UART_MODE_RS485_HALF_DUPLEX,
    UART_MODE_IRDA,
    UART_MODE_RS485_COLLISION_DETECT,
    UART_MODE_RS485_APP_CTRL
};

enum
{
    UART_DATA_5_BITS            = 0,
    UART_DATA_6_BITS,
    UART_DATA_7_BITS,
    UART_DATA_8_BITS
};

enum
{
    UART_STOP_BITS_1            = 1,
    UART_STOP_BITS_1_5,
    UART_STOP_BITS_2
};

enum
{
    UART_PARITY_EVEN            = 0,
    UART_PARITY_ODD
};

enum
{
    UART_HW_FLOWCTRL_DISABLE    = 0x0,
    UART_HW_FLOWCTRL_RTS,
    UART_HW_FLOWCTRL_CTS,
    UART_HW_FLOWCTRL_CTS_RTS
};

enum
{
    UART_SIGNAL_INV_DISABLE     = 0,
    UART_SIGNAL_IRDA_TX_INV,
    UART_SIGNAL_IRDA_RX_INV,
    UART_SIGNAL_RXD_INV,
    UART_SIGNAL_CTS_INV,
    UART_SIGNAL_DSR_INV,
    UART_SIGNAL_TXD_INV,
    UART_SIGNAL_RTS_INV,
    UART_SIGNAL_DTR_INV
};


/****************************************************************************
 *  @internal
 ****************************************************************************/
// constructor
static int UART_init_context(struct UART_context *context);
// configure
static uint64_t UART_sclk_freq(soc_uart_sclk_sel_t sel);
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

int UART_configure(uart_dev_t *dev, soc_uart_sclk_sel_t sclk, uint32_t bps, parity_t parity, uint8_t stop_bits)
{
    periph_module_t uart_module;
    int retval;

    if (&UART0 == dev)
        uart_module = PERIPH_UART0_MODULE;
    else if (&UART1 == dev)
        uart_module = PERIPH_UART1_MODULE;
    else if (&UART2 == dev)
        uart_module = PERIPH_UART2_MODULE;
    else
        return ENODEV;

    clk_tree_module_enable(uart_module);
    clk_tree_module_reset(uart_module);

    // uart normal
    dev->rs485_conf.val = 0;
    dev->conf0.irda_en = 0;
    // 8 bits only
    dev->conf0.bit_num = UART_DATA_8_BITS;

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
            dev->conf0.parity = UART_PARITY_ODD;
            dev->conf0.parity_en = 1;
            break;
        case paEven:
            dev->conf0.parity = UART_PARITY_EVEN;
            dev->conf0.parity_en = 1;
            break;
        }
    }

    switch (stop_bits)
    {
    default:
        retval = EINVAL;
        goto uart_configure_fail_exit;
    case 1:
        dev->conf0.stop_bit_num = UART_STOP_BITS_1;
        break;
    case 2:
        dev->conf0.stop_bit_num = UART_STOP_BITS_2;
        break;
    /*
        1.5 stopbits?
        dev->conf0.stop_bit_num = 2;
    */
    }

    dev->clk_conf.sclk_sel = sclk;
    uint64_t sclk_freq = UART_sclk_freq(sclk);

    // calucate baudrate
    int sclk_div = (sclk_freq + 4095 * bps - 1) / (4095 * bps);
    uint32_t clk_div = ((sclk_freq) << 4) / (bps * sclk_div);
    // The baud rate configuration register is divided into // an integer part and a fractional part.
    dev->clkdiv.clkdiv = clk_div >> 4;
    dev->clkdiv.clkdiv_frag = clk_div &  0xF;
    dev->clk_conf.sclk_div_num =  sclk_div - 1;

    if (false)
    {
uart_configure_fail_exit:
        clk_tree_module_disable(uart_module);
    }
    else
        retval = 0;

    return retval;
}

int UART_sclk_sel(uart_dev_t *dev, enum soc_uart_sclk_sel_t sel)
{
    dev->clk_conf.sclk_sel = sel;
}

uint32_t UART_get_baudrate(uart_dev_t *dev)
{
    return (UART_sclk_freq(dev->clk_conf.sclk_sel) << 4) / (
        ((dev->clkdiv.clkdiv << 4) | dev->clkdiv.clkdiv_frag) * (dev->clk_conf.sclk_div_num + 1)
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

static uint64_t UART_sclk_freq(soc_uart_sclk_sel_t sel)
{
    switch(sel)
    {
    case SOC_UART_CLK_SRC_APB:
        return clk_tree_apb_freq();
    case SOC_UART_CLK_SRC_RC_FAST:
        return clk_tree_sclk_freq(SOC_SCLK_INT_RC_FAST);
    case SOC_UART_CLK_SRC_XTAL:
        return clk_tree_sclk_freq(SOC_SCLK_XTAL);
    }
}
