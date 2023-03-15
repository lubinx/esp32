#include <stdio.h>
#include <unistd.h>

#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/types.h>
#include <semaphore.h>

#include <rtos/kernel.h>
#include <soc/soc_caps.h>
#include <soc/i2c_reg.h>

#include "clk-tree.h"
#include "esp_intr_alloc.h"

#include "i2c.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
void I2C0_IntrHandler(void *arg);
void I2C1_IntrHandler(void *arg);

#define I2C_CONFIG_CLK_FREQ             (10 * _MHZ)

// TODO: sdkconfig to configure i2c filter, 0: disable
#define I2C_SCL_FILTER_APB_CYCLE        (7)
#define I2C_SDA_FILTER_APB_CYCLE        (7)

struct I2C_addressing
{
    uint16_t da;                    // address of device
    uint8_t ridx_bytes;             // bytes of sub address been used: 0, 1, 2, 3
    uint8_t ridx[4];                // sub addresses of device (highest REGISTER no)
};

struct I2C_context
{
    i2c_dev_t *dev;
    intr_handle_t intr_hdl;

    uint32_t fd_count;

    mutex_t lock;
    sem_t evt;
    int err;

    struct I2C_addressing addressing;
    uint8_t ridx_wpos;
    uint8_t page_size;
    bool ridx_sending;
    bool hi_testing;

    uint8_t *buf;
    uint32_t bufsize;
};

struct I2C_clk_config
{
    uint16_t clk_div_10m;
    uint16_t scl_low;
    uint16_t scl_high;
    uint16_t scl_wait_high;
    uint16_t sda_hold;
    uint16_t sda_sample;
    uint16_t setup;             // start/stop setup period
    uint16_t hold;              // start/stop hold period
    uint16_t tout;              // bus timeout
};

struct I2C_fd_ext
{
    struct I2C_context *context;

    uint16_t da;
    uint8_t page_size;
    uint8_t ridx_bytes;
    uint32_t highest_addr;

    struct I2C_clk_config clk_conf;
};

/***************************************************************************
 *  @internal
 ***************************************************************************/
static void I2C_calculate_bps(i2c_dev_t *dev, struct I2C_clk_config *cfg);

static struct I2C_context *I2C_lock(int fd, uint32_t timeout);
static void I2C_unlock(struct I2C_context *context);
static void I2C_addressing(int fd, struct I2C_fd_ext *ext);

static ssize_t I2C_read(int fd, void *buf, size_t count);
static ssize_t I2C_write(int fd, void const *buf, size_t count);
static off_t I2C_seek(int fd, off_t offset, int mode);
static int I2C_close(int fd);

// const
static struct FD_implement const implement =
{
    .close  = I2C_close,
    .read   = I2C_read,
    .write  = I2C_write,
};

// var
struct I2C_context i2c_context[SOC_I2C_NUM] = {0};

/****************************************************************************
 *  constructor
 ****************************************************************************/
__attribute__((constructor))
void I2C_initialize()
{
    esp_intr_alloc(ETS_I2C_EXT0_INTR_SOURCE, 0, I2C0_IntrHandler, &i2c_context[0], &i2c_context[0].intr_hdl);
    esp_intr_alloc(ETS_I2C_EXT1_INTR_SOURCE, 0, I2C1_IntrHandler, &i2c_context[1], &i2c_context[1].intr_hdl);
}

/****************************************************************************
 *  @implements
 ****************************************************************************/
int I2C_createfd(int nb, uint8_t da, uint16_t kbps, uint8_t page_size, uint32_t highest_addr)
{
    if (1000 < kbps || 0 == kbps)
        return __set_errno_neg(EINVAL);

    struct I2C_context *context = NULL;

    if (0 == nb)
    {
        context = &i2c_context[0];

        if (! context->dev)
        {
            context->dev = &I2C0;
            goto init_context_syncobjs;
        };
    }
    else if (1 == nb)
    {
        context = &i2c_context[1];

        if (! context->dev)
        {
            context->dev = &I2C1;
            goto init_context_syncobjs;
        };
    }

    if (false)
    {
init_context_syncobjs:
        mutex_init(&context->lock, MUTEX_FLAG_RECURSIVE);
        sem_init(&context->evt, 0, 1);
    }

    if (NULL == context)
        return __set_errno_neg(ENODEV);

    struct I2C_fd_ext *ext = KERNEL_mallocz(sizeof(struct I2C_fd_ext));
    if (NULL == ext)
        return __set_errno_neg(ENOMEM);

    int fd = KERNEL_createfd(FD_TAG_CHAR, &implement, ext);
    if (-1 != fd)
    {
        context->fd_count ++;
        AsFD(fd)->read_rdy = AsFD(fd)->write_rdy = &context->lock;

        I2C_configure(context->dev, I2C_MASTER_MODE, kbps);

        ext->context = context;
        ext->da = da;
        ext->page_size = page_size;
        ext->highest_addr = highest_addr;

        if (0 == highest_addr)                      // no Addressing
            ext->ridx_bytes = 0;
        else if (highest_addr < 256)                // 256 Bytes
            ext->ridx_bytes = 1;
        else if (highest_addr < 256 * 256)          // 64k
            ext->ridx_bytes = 2;
        else if (highest_addr < 256 * 256 * 256)    // 16m
            ext->ridx_bytes = 3;
        else
            ext->ridx_bytes = 4;
    }
    else
        KERNEL_mfree(ext);

    return fd;
}

int I2C_configure(i2c_dev_t *dev, enum I2C_mode_t mode, uint32_t kbps)
{
    if (I2C_MASTER_MODE != mode)
    {
        /// TODO: I2C slave mode
        return ENOSYS;
    }

    PERIPH_module_t i2c_module;
    int retval;

    if (&I2C0 == dev)
        i2c_module = PERIPH_I2C0_MODULE;
    else if (&I2C1 == dev)
        i2c_module = PERIPH_I2C1_MODULE;
    else
        return ENODEV;

    if (CLK_periph_is_enabled(i2c_module))
        return EBUSY;
    else
        CLK_periph_enable(i2c_module);

    if (I2C_MASTER_MODE == mode)
    {
        // disable before setup
        dev->clk_conf.sclk_active = 0;
        dev->int_ena.val = 0;
        dev->int_clr.val= ~0;
        // fifo
        dev->fifo_conf.nonfifo_en = 0;
        dev->fifo_conf.tx_fifo_rst = dev->fifo_conf.rx_fifo_rst = 1;
        dev->fifo_conf.tx_fifo_rst = dev->fifo_conf.rx_fifo_rst = 0;
        // filter
        if (I2C_SCL_FILTER_APB_CYCLE)
        {
            dev->filter_cfg.scl_filter_en = 1;
            dev->filter_cfg.scl_filter_thres = I2C_SCL_FILTER_APB_CYCLE;
        }
        if (I2C_SDA_FILTER_APB_CYCLE)
        {
            dev->filter_cfg.sda_filter_en = 1;
            dev->filter_cfg.sda_filter_thres = I2C_SDA_FILTER_APB_CYCLE;
        }

        // i2c ctr master
        dev->ctr.val = 0;
        dev->ctr.ms_mode = 1;
        // dev->ctr.clk_en = 1; what is this?
        // scl/sda output mode. 0 open-drain, 1 push-pull
        dev->ctr.sda_force_out = 1;
        dev->ctr.scl_force_out = 1;
        // MSB
        dev->ctr.rx_lsb_first =  dev->ctr.tx_lsb_first = 0;
        // bps
        dev->clk_conf.sclk_div_num = CLK_i2c_sclk_freq(dev) / I2C_CONFIG_CLK_FREQ;
        uint32_t half_cycle = I2C_CONFIG_CLK_FREQ / 1000 / kbps / 2;

        // update
        dev->ctr.conf_upgate = 1;
        dev->clk_conf.sclk_active = 1;
    }
    else
    {
    }

    if (false)
    {
i2c_configure_fail_exit:
        CLK_periph_disable(i2c_module);
    }
    else
        retval = 0;

    return 0;
}

int I2C_deconfigure(i2c_dev_t *dev)
{
    PERIPH_module_t i2c_module;

    if (&I2C0 == dev)
        i2c_module = PERIPH_I2C0_MODULE;
    else if (&I2C1 == dev)
        i2c_module = PERIPH_I2C1_MODULE;
    else
        return ENODEV;

    CLK_periph_disable(i2c_module);
    return 0;
}

unsigned I2C_get_bps(i2c_dev_t dev)
{

}

/****************************************************************************
 *  @implements: direct I2C start/stop
 ****************************************************************************/
void I2C_start(i2c_dev_t *dev)
{
}

void I2C_stop(i2c_dev_t *dev)
{
}

/****************************************************************************
 *  @implements: direct I2C fifo I/O
 ****************************************************************************/
int I2C_fifo_write(i2c_dev_t *dev, void const *buf, unsigned count)
{
    dev->data.fifo_rdata = *(uint8_t *)buf;
}

int I2C_fifo_read(i2c_dev_t *dev, void *buf, unsigned bufsize)
{
}

/***************************************************************************
 *  @internal
 ***************************************************************************/
static struct I2C_context *I2C_lock(int fd, uint32_t timeout)
{
    struct I2C_fd_ext *ext = (struct I2C_fd_ext *)AsFD(fd)->ext;
    if (ext->highest_addr && AsFD(fd)->position > ext->highest_addr)
        return __set_errno_nullptr(ERANGE);

    struct I2C_context *context = ext->context;
    i2c_dev_t *dev = context->dev;

    if (0 == mutex_trylock(&context->lock, timeout))
    {
        context->page_size = ext->page_size;
        context->hi_testing = false;

        dev->clk_conf.sclk_active = 1;
        // dev->int_ena.val =

        /*
        DEV->CLKDIV = ext->REG.CLKDIV;
        DEV->CTRL = ext->REG.CTRL | I2C_CTRL_AUTOSN;

        DEV->IF_CLR = _I2C_IF_MASK;
        DEV->IEN = I2C_IEN_MSTOP | I2C_IEN_TXC | I2C_IEN_RXDATAV | I2C_IEN_ERRORS;
        DEV->EN = I2C_EN_EN;
        */

        I2C_addressing(fd, ext);
        return context;
    }
    else
        return NULL;
}

static void I2C_unlock(struct I2C_context *context)
{
    if (0 == mutex_unlock(&context->lock))
    {
        /*
        I2C_TypeDef *DEV = context->dev;

        if (I2C_STATE_BUSY & DEV->STATE)
            DEV->CMD = I2C_CMD_CLEARPC | I2C_CMD_CLEARTX;
        (void)(DEV->RXDATA);  // clear RX fifo

        DEV->CTRL = 0;
        DEV->IEN = 0;
        DEV->EN = 0;

        if (I2C0 == DEV)
            CMU->CLKEN0_CLR = CMU_CLKEN0_I2C0;
        else if (I2C1 == DEV)
            CMU->CLKEN0_CLR = CMU_CLKEN0_I2C1;
        else
            ASSERT_BKPT("NODEV");
        */
    }
}

static void I2C_addressing(int fd, struct I2C_fd_ext *ext)
{
    struct I2C_context *context = ext->context;

    context->ridx_wpos = 0;
    context->addressing.da = ext->da;
    context->addressing.ridx_bytes = ext->ridx_bytes;

    uint8_t *ridx = context->addressing.ridx;
    switch (ext->ridx_bytes)
    {
    case 4:
        *ridx ++ = (uint8_t)((AsFD(fd)->position >> 24) & 0xFF);
        goto fall_through_3;
fall_through_3:
    case 3:
        *ridx ++ = (uint8_t)((AsFD(fd)->position >> 16) & 0xFF);
        goto fall_through_2;
fall_through_2:
    case 2:
        *ridx ++ = (uint8_t)((AsFD(fd)->position >> 8 ) & 0xFF);
        goto fall_through_1;
fall_through_1:
    case 1:
        *ridx = (uint8_t)(AsFD(fd)->position & 0xFF);
        break;
    }
}

static ssize_t I2C_read(int fd, void *buf, size_t count)
{
    uint32_t timeout = AsFD(fd)->read_timeo;
    if (0 == timeout)
        timeout = INFINITE;

    struct I2C_context *context = I2C_lock(fd, timeout);
    if (NULL == context)
        return -1;

    context->buf = (uint8_t *)buf;
    context->bufsize = count;

    context->addressing.da |= 0x01;
    if (context->addressing.ridx_bytes > 0)
    {
        // seek I2C before reading
        context->ridx_sending = true;
        // context->dev->TXDATA = context->addressing.da & ~0x01UL;
    }
    else
        ; // context->dev->TXDATA = context->addressing.da;

    // I2C startting
    // context->dev->CMD = I2C_CMD_START;

    if (0 == sem_timedwait_ms(&context->evt, timeout))
    {
        count -= context->bufsize;
        AsFD(fd)->position += count;
    }
    else
    {
        /*
        context->dev->CMD = I2C_CMD_ABORT | I2C_CMD_CLEARPC | I2C_CMD_CLEARTX;
        context->dev->IF_CLR = _I2C_IF_MASK;
        */
        context->err = ETIMEDOUT;
    }

    I2C_unlock(context);

    if (0 == count || 0 != context->err)
    {
        if (! context->hi_testing)
            return __set_errno_neg(ENXIO);
        else
            return __set_errno_neg(context->err ? context->err : EIO);
    }
    else
        return (ssize_t)count;
}

static ssize_t I2C_write(int fd, void const *buf, size_t count)
{
    uint32_t timeout = AsFD(fd)->write_timeo;
    if (0 == timeout)
        timeout = INFINITE;

    struct I2C_context *context = I2C_lock(fd, timeout);
    if (NULL == context)
        return -1;

    if (context->page_size > 0)
    {
        uint32_t page_remain = context->page_size - AsFD(fd)->position % context->page_size;
        if (page_remain < count)
            count = page_remain;
    }
    context->buf = (uint8_t *)buf;
    context->bufsize = count;

    // starting
    context->addressing.da &= (uint16_t)~0x01U;
    context->ridx_sending = context->addressing.ridx_bytes > 0;

    /*
    context->dev->TXDATA = context->addressing.da;
    context->dev->CMD = I2C_CMD_START;
    */

    if (0 == sem_timedwait_ms(&context->evt, timeout))
    {
        count -= context->bufsize;
        AsFD(fd)->position += count;
    }
    else
    {
        /*
        context->dev->CMD = I2C_CMD_ABORT | I2C_CMD_CLEARPC | I2C_CMD_CLEARTX;
        context->dev->IF_CLR = _I2C_IF_MASK;
        */
        context->err = ETIMEDOUT;
    }

    // wait write cycle complete
    if (count > 0 && context->page_size > 0)
    {
        context->hi_testing = false;
        context->bufsize = 0;

        while (! context->hi_testing)
        {
            // starting
            /*
            context->dev->TXDATA = context->addressing.da;
            context->dev->CMD = I2C_CMD_START;
            */

            if (0 != sem_timedwait_ms(&context->evt, timeout))
            {
                context->err = ETIMEDOUT;
                break;
            }
            else
                msleep(1);
        }
    }
    I2C_unlock(context);

    if (0 == count || 0 != context->err)
    {
        if (! context->hi_testing)
            return __set_errno_neg(ENXIO);
        else
            return __set_errno_neg(context->err ? context->err : EIO);
    }
    else
        return (ssize_t)count;
}

static off_t I2C_seek(int fd, off_t offset, int mode)
{
    struct I2C_fd_ext *ext = (struct I2C_fd_ext *)AsFD(fd)->ext;
    uintptr_t old_pos = AsFD(fd)->position;

    switch (mode)
    {
    case SEEK_SET:
        AsFD(fd)->position = (uintptr_t)offset;
        break;

    case SEEK_CUR:
        AsFD(fd)->position += (uintptr_t)offset;
        break;

    case SEEK_END:
        AsFD(fd)->position = (uintptr_t)(ext->highest_addr - (unsigned)offset);
        break;

    default:
        return __set_errno_neg(EINVAL);
    }

    /* check position in accaptable range */
    if (AsFD(fd)->position > ext->highest_addr)
    {
        AsFD(fd)->position = old_pos;
        return __set_errno_neg(EINVAL);
    }
    else
        return (off_t)AsFD(fd)->position;
}

static int I2C_close(int fd)
{
    struct I2C_fd_ext *ext = (struct I2C_fd_ext *)AsFD(fd)->ext;
    struct I2C_context *context = ext->context;

    if (0 == -- context->fd_count)
    {
        // release GPIO to DISABLED when no fd is required?
    I2C_deconfigure(context->dev);
        // I2C_config(context->dev, false);
    }

    KERNEL_mfree(ext);
    return 0;
}


/****************************************************************************
 *  intr
 ****************************************************************************/
static void I2C_IntrHandler(struct I2C_context *context)
{
    i2c_dev_t *dev = context->dev;
    uint32_t flags = dev->int_status.val;
    /*
    uint32_t flags = DEV->IF;

    if (I2C_IF_ERRORS & flags)
    {
        DEV->CMD = I2C_CMD_ABORT | I2C_CMD_CLEARPC | I2C_CMD_CLEARTX;

        context->err = EIO;
        event_signal(&context->evt);
        goto i2c_exit;
    }
    if (I2C_IF_MSTOP & flags)
    {
        context->err = 0;
        event_signal(&context->evt);
        goto i2c_exit;
    }

    // device has responsed
    context->hi_testing = true;

    if (0 == context->bufsize)
    {
        DEV->CMD = I2C_CMD_STOP;
        goto i2c_exit;
    }

    if (I2C_IF_RXDATAV & flags)
    {
        *context->buf ++ = (uint8_t)DEV->RXDATA;
        context->bufsize --;

        DEV->IF_CLR = I2C_IF_RXDATAV;

        if (0 == context->bufsize)
            DEV->CMD = I2C_CMD_NACK;
        else
            DEV->CMD = I2C_CMD_ACK;
    }
    else if ((I2C_IF_TXC & flags) && (! (0x01 & context->addressing.da) || context->ridx_sending))
    {
        if (context->ridx_sending)
        {
            if (context->ridx_wpos == context->addressing.ridx_bytes)
            {
                context->ridx_sending = false;

                // restarting to reading
                if (0x01 & context->addressing.da)
                {
                    DEV->CMD = I2C_CMD_START;
                    DEV->TXDATA = context->addressing.da;
                }
                else
                    I2C_tx_next(DEV, context);
            }
            else
                DEV->TXDATA = context->addressing.ridx[context->ridx_wpos ++];
        }
        else
            I2C_tx_next(DEV, context);
    }

    */

i2c_intr_exit:
    dev->int_clr.val = flags;
}

void I2C0_IntrHandler(void *arg)
    __attribute__((alias("I2C_IntrHandler")));

void I2C1_IntrHandler(void *arg)
    __attribute__((alias("I2C_IntrHandler")));
