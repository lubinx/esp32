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

/****************************************************************************
 *  @internal
 ****************************************************************************/
static int UART_init_context(struct UART_context *context);
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
        return __set_errno_neg(ENOSYS);
    }
    else
        return __set_errno_neg(retval);
}

int UART_sclk_sel(uart_dev_t *dev, enum soc_uart_sclk_sel_t sel)
{
    dev->clk_conf.sclk_sel = sel;
}

uint64_t UART_sclk_freq(uart_dev_t *dev)
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

int UART_set_baudrate(uart_dev_t *dev, uint32_t bps)
{
}

uint32_t UART_get_baudrate(uart_dev_t *dev)
{
    return (UART_sclk_freq(dev) << 4) / (
        ((dev->clkdiv.clkdiv << 4) | dev->clkdiv.clkdiv_frag) *  (dev->clk_conf.sclk_div_num + 1)
    );
    /*
    uart_sclk_t src_clk;
    uart_ll_get_sclk(dev, &src_clk);

    uint32_t sclk_freq;

    // ESP_RETURN_ON_ERROR(uart_get_sclk_freq(src_clk, &sclk_freq), UART_TAG, "Invalid src_clk");
    return uart_ll_get_baudrate(dev, sclk_freq);
    */
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

        return 0;
    }
    else
        return ENOMEM;
}
