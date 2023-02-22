#include <sched.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/errno.h>

#include "esp_log.h"
#include "clk_tree.h"
#include "esp_intr_alloc.h"
#include "soc/soc_caps.h"

#include "uart.h"
#include "ultracore/kernel.h"

static char const *UART_TAG = "uart";
static char const *RS485_TAG = "rs485";
static char const *IRDA_TAG = "rs485";

/****************************************************************************
 *  @declaration
 ****************************************************************************/
extern int __stdout_fd;
void UART0_IntrHandler(void *arg);
void UART2_IntrHandler(void *arg);
void UART1_IntrHandler(void *arg);

/****************************************************************************
 *  @declaration
 ****************************************************************************/
struct UART_context
{
    uart_dev_t *dev;
    intr_handle_t intr_hdl;

    sem_t read_rdy;
    sem_t write_rdy;

    // tx write to ptr
    uint8_t *tx_ptr;
    uint8_t *tx_end;
};

enum
{
    UART_MODE_UART                  = 0,
    UART_MODE_RS485_HALF_DUPLEX,
    UART_MODE_IRDA,
    UART_MODE_RS485_COLLISION_DETECT,
    UART_MODE_RS485_APP_CTRL
};
enum
{
    UART_DATA_5_BITS                = 0,
    UART_DATA_6_BITS,
    UART_DATA_7_BITS,
    UART_DATA_8_BITS
};
enum
{
    UART_HW_FLOWCTRL_DISABLE        = 0,
    UART_HW_FLOWCTRL_RTS,
    UART_HW_FLOWCTRL_CTS,
    UART_HW_FLOWCTRL_CTS_RTS
};
enum
{
    UART_SIGNAL_INV_DISABLE         = 0,
    UART_SIGNAL_IRDA_TX_INV,
    UART_SIGNAL_IRDA_RX_INV,
    UART_SIGNAL_RXD_INV,
    UART_SIGNAL_CTS_INV,
    UART_SIGNAL_DSR_INV,
    UART_SIGNAL_TXD_INV,
    UART_SIGNAL_RTS_INV,
    UART_SIGNAL_DTR_INV
};
enum
{
    UART_INTR_RXFIFO_FULL           = (1 << 0),
    UART_INTR_TXFIFO_EMPTY          = (1 << 1),
    UART_INTR_PARITY_ERR            = (1 << 2),
    UART_INTR_FRAME_ERR             = (1 << 3),
    UART_INTR_RXFIFO_OVF            = (1 << 4),
    UART_INTR_DSR_CHG               = (1 << 5),
    UART_INTR_CTS_CHG               = (1 << 6),
    UART_INTR_BRK_DET               = (1 << 7),
    UART_INTR_RXFIFO_TOUT           = (1 << 8),
    UART_INTR_SW_XON                = (1 << 9),
    UART_INTR_SW_XOFF               = (1 << 10),
    UART_INTR_GLITCH_DET            = (1 << 11),
    UART_INTR_TX_BRK_DONE           = (1 << 12),
    UART_INTR_TX_BRK_IDLE           = (1 << 13),
    UART_INTR_TX_DONE               = (1 << 14),
    UART_INTR_RS485_PARITY_ERR      = (1 << 15),
    UART_INTR_RS485_FRAME_ERR       = (1 << 16),
    UART_INTR_RS485_CLASH           = (1 << 17),
    UART_INTR_CMD_CHAR_DET          = (1 << 18),
    UART_INTR_WAKEUP                = (1 << 19),
};

/****************************************************************************
 *  @internal
 ****************************************************************************/
// configure
static uint64_t UART_sclk_freq(uart_sclk_sel_t sel);
// io
static ssize_t UART_read(int fd, void *buf, size_t bufsize);
static ssize_t UART_write(int fd, void const *buf, size_t count);
static int UART_close(int fd);

// const
static struct FD_implement const implement =
{
    .close  = UART_close,
    .read   = UART_read,
    .write  = UART_write,
};

// var
static struct UART_context uart_context[SOC_UART_NUM] = {0};

/****************************************************************************
 *  @implements
 ****************************************************************************/
__attribute__((constructor))
void UART_initialize(void)
{
    esp_intr_alloc(ETS_UART0_INTR_SOURCE, 0, UART0_IntrHandler, &uart_context[0], &uart_context[0].intr_hdl);
    esp_intr_alloc(ETS_UART1_INTR_SOURCE, 0, UART1_IntrHandler, &uart_context[1], &uart_context[1].intr_hdl);
    esp_intr_alloc(ETS_UART2_INTR_SOURCE, 0, UART2_IntrHandler, &uart_context[2], &uart_context[2].intr_hdl);
}

int UART_createfd(int nb, uint32_t bps, enum UART_parity_t parity, enum UART_stopbits_t stopbits)
{
    if (SOC_UART_NUM < (unsigned)nb)
        return __set_errno_neg(EINVAL);

    struct UART_context *context = &uart_context[nb];
    if (NULL != context->dev)
        return __set_errno_neg(EBUSY);

    uart_dev_t *dev;
    uart_sclk_sel_t sclk;

    switch (nb)
    {
    case 0:
        dev = &UART0;
        sclk = UART_SCLK_SEL_XTAL;  // TODO: configure this by KCONFIG
        break;
    case 1:
        dev = &UART1;
        sclk = UART_SCLK_SEL_XTAL;
        break;
    case 2:
        dev = &UART2;
        sclk = UART_SCLK_SEL_XTAL;
        break;
    }
    context->dev = dev;
    dev->clk_conf.sclk_sel = sclk;

    sem_init(&context->read_rdy, 0, 1);
    sem_init(&context->write_rdy, 0, 1);

    int retval = UART_configure(dev, sclk, bps, parity, stopbits);

    if (0 == retval)
    {
        retval = KERNEL_createfd(FD_TAG_CHAR, &implement, context);
        // use first uart as stdout when no stdout fd is assigned
        if (-1 == __stdout_fd)
            __stdout_fd = retval;

        sem_post(&context->write_rdy);
    }
    else
        retval = __set_errno_neg(retval);

    return retval;
}

int UART_configure(uart_dev_t *dev, uart_sclk_sel_t sclk, uint32_t bps, enum UART_parity_t parity, enum UART_stopbits_t stopbits)
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

    if (clk_tree_module_is_enable(uart_module))
    {
        dev->int_ena.val = 0;
        while (0 != dev->status.txfifo_cnt) sched_yield();
    }

    clk_tree_module_enable(uart_module);
    clk_tree_module_reset(uart_module);

    // uart normal
    dev->rs485_conf.val = 0;
    dev->conf0.irda_en = 0;
    // 8 bits only
    dev->conf0.bit_num = UART_DATA_8_BITS;

    if (UART_PARITY_NONE != parity)
    {
        switch ((parity))
        {
        default:
            retval = EINVAL;
            goto uart_configure_fail_exit;
        case UART_PARITY_NONE:
            dev->conf0.parity_en = 0;
            break;
        case UART_PARITY_ODD:
            dev->conf0.parity = 1;
            dev->conf0.parity_en = 1;
            break;
        case UART_PARITY_EVEN:
            dev->conf0.parity = 0;
            dev->conf0.parity_en = 1;
            break;
        }
    }

    switch (stopbits)
    {
    default:
        retval = EINVAL;
        goto uart_configure_fail_exit;

    case UART_STOP_BITS_ONE:
        dev->conf0.stop_bit_num = 1;
        break;
    case UART_STOP_BITS_ONE_HALF:
        dev->conf0.stop_bit_num = 2;
        break;
    case UART_STOP_BITS_TWO:
        dev->conf0.stop_bit_num = 3;
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

    dev->conf1.rxfifo_full_thrhd = 1;

    dev->int_ena.val = UART_INTR_PARITY_ERR | UART_INTR_FRAME_ERR |
        UART_INTR_RS485_PARITY_ERR | UART_INTR_RS485_FRAME_ERR |
        0;

    if (false)
    {
uart_configure_fail_exit:
        clk_tree_module_disable(uart_module);
    }
    else
        retval = 0;

    return retval;
}

int UART_deconfigure(uart_dev_t *dev)
{
    periph_module_t uart_module;

    if (&UART0 == dev)
        uart_module = PERIPH_UART0_MODULE;
    else if (&UART1 == dev)
        uart_module = PERIPH_UART1_MODULE;
    else if (&UART2 == dev)
        uart_module = PERIPH_UART2_MODULE;
    else
        return ENODEV;

    clk_tree_module_disable(uart_module);
    return 0;
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
static uint64_t UART_sclk_freq(uart_sclk_sel_t sel)
{
    switch(sel)
    {
    case UART_SCLK_SEL_APB:
        return clk_tree_apb_freq();
    case UART_SCLK_SEL_INT_RC_FAST:
        return clk_tree_sclk_freq(SOC_SCLK_INT_RC_FAST);
    case UART_SCLK_SEL_XTAL:
        return clk_tree_sclk_freq(SOC_SCLK_XTAL);
    }
}

static ssize_t UART_read(int fd, void *buf, size_t bufsize)
{
    struct UART_context *context = AsFD(fd)->ext;
    uart_dev_t *dev = context->dev;
    int retval;

    if (! dev->status.rxfifo_cnt)
    {
        // rxfifo intr status is not automate cleared
        dev->int_clr.rxfifo_full_int_clr = 1;
        dev->int_ena.rxfifo_full_int_ena = 1;

        uint32_t timeo;
        if (! (FD_FLAG_NONBLOCK & AsFD(fd)->flags))
        {
            timeo = AsFD(fd)->read_timeo;
            if (0 == timeo)
                timeo = INFINITE;
        }
        else
            timeo = 0;

        retval = sem_timedwait_ms(&context->read_rdy, timeo);
    }
    else
        retval = 0;

    if (0 == retval)
        retval = UART_fifo_read(dev, buf, bufsize);

    return retval;
}

static ssize_t UART_write(int fd, void const *buf, size_t count)
{
    struct UART_context *context = AsFD(fd)->ext;
    uart_dev_t *dev = context->dev;
    int retval;

    uint32_t timeo;
    if (! (FD_FLAG_NONBLOCK & AsFD(fd)->flags))
    {
        timeo = AsFD(fd)->write_timeo;
        if (0 == timeo)
            timeo = INFINITE;
    }
    else
        timeo = 0;

    retval = sem_timedwait_ms(&context->write_rdy, timeo);
    if (0 == retval)
    {
        int written = UART_fifo_write(dev, buf, count);
        if (written < count)
        {
            context->tx_ptr = (uint8_t *)buf + written;
            context->tx_end = (uint8_t *)buf + count;

            dev->int_ena.tx_done_int_ena = 1;

            sem_wait(&context->write_rdy);
            retval = count;
        }
        else
            retval = written;

        sem_post(&context->write_rdy);
    }
    return retval;
}

static int UART_close(int fd)
{
    struct UART_context *context = AsFD(fd)->ext;
    int retval = UART_deconfigure(context->dev);

    if (0 == retval)
    {
        context->dev = NULL;
        return retval;
    }
    else
        return __set_errno_neg(retval);
}

/****************************************************************************
 *  intr
 ****************************************************************************/
static void UART_IntrHandler(struct UART_context *context)
{
    uart_dev_t *dev = context->dev;
    uint32_t flags = dev->int_st.val;
    char const *TAG = dev->rs485_conf.rs485_en ? RS485_TAG : UART_TAG;

    if (UART_INTR_RS485_CLASH & flags)
    {
        ESP_LOGE(RS485_TAG, "clushed");
    }
    if ((UART_INTR_PARITY_ERR | UART_INTR_RS485_PARITY_ERR) & flags)
    {
        ESP_LOGW(TAG, "parity error");
    }
    if ((UART_INTR_PARITY_ERR | UART_INTR_RS485_FRAME_ERR) & flags)
    {
        ESP_LOGW(TAG, "frame error");
    }
    if (UART_INTR_RXFIFO_OVF & flags)
    {
        ESP_LOGW(TAG, "rx fifo overflow");
    }

    if (UART_INTR_RXFIFO_FULL & flags)
    {
        dev->int_ena.rxfifo_full_int_ena = 0;
        sem_post(&context->read_rdy);
    }

    if (UART_INTR_TX_DONE & flags)
    {
        if (context->tx_ptr)
        {
            context->tx_ptr += UART_fifo_write(dev, context->tx_ptr, context->tx_end - context->tx_ptr);

            if (context->tx_ptr == context->tx_end)
            {
                context->tx_ptr = NULL;
                sem_post(&context->write_rdy);
            }
        }
        else
            dev->int_ena.tx_done_int_ena = 0;
    }

    dev->int_clr.val = flags;
}

void UART0_IntrHandler(void *arg)
    __attribute__((alias("UART_IntrHandler")));

void UART1_IntrHandler(void *arg)
    __attribute__((alias("UART_IntrHandler")));

void UART2_IntrHandler(void *arg)
    __attribute__((alias("UART_IntrHandler")));
