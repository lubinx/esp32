#include <stdio.h>
#include <unistd.h>
#include <endian.h>

#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/types.h>
#include <semaphore.h>

#include <esp_log.h>

#include <rtos/kernel.h>
#include <soc/soc_caps.h>
#include <soc/i2c_reg.h>

#include "clk-tree.h"
#include "esp_intr_alloc.h"

#include "i2c.h"
#include "gpio.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
void I2C0_IntrHandler(void *arg);
void I2C1_IntrHandler(void *arg);

// TODO: sdkconfig to configure i2c filter, 0: disable
#define I2C_SCL_FILTER_CYCLE        (5)
#define I2C_SDA_FILTER_CYCLE        (5)

// I2C ext. errno, NEED to process before return to caller
#define I2C_ERR_NACK                    (ESP_ERR_BASE + 1)

// I2C command
enum i2c_command
{
    I2C_CMD_WRITE       = 1,
    I2C_CMD_STOP,
    I2C_CMD_READ,
    I2C_CMD_END,
    I2C_CMD_RESTART     = 6,
};

union i2c_cmd_reg
{
    struct
    {
        uint32_t io_bytes           : 8;
        uint32_t ack_check_en       : 1;
        uint32_t ack_expect_nack    : 1;    // basicly always expect ACK: 0
        uint32_t ack_nack           : 1;
        uint32_t op_code            : 3;
        uint32_t reserved           : 17;
        uint32_t done               : 1;
    };
    uint32_t val;
};
typedef union i2c_cmd_reg           i2c_cmd_reg_t;
static_assert(sizeof(i2c_cmd_reg_t) == sizeof(uint32_t), "sizeof(i2c_cmd_reg_t) *must == sizeof(uint32_t)");

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

struct I2C_context
{
    i2c_dev_t *dev;
    intr_handle_t intr_hdl;
    uint32_t fd_count;

    mutex_t lock;
    sem_t evt;

    int err;
    uint32_t timeout;
    uint8_t page_size;

    uint8_t *sa; uint8_t sa_bytes;
    uint8_t *tx_buf, *tx_buf_end;
    uint8_t *rx_buf, *rx_buf_end;
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
static PERIPH_module_t I2C_periph_module(i2c_dev_t *dev, struct I2C_context **context);
static void I2C_configure_bps(i2c_dev_t *dev, uint32_t kbps);

// command
static void I2C_command_start(i2c_dev_t *dev);
static void I2C_command(i2c_dev_t *dev, uint8_t *idx, uint32_t regval);

// fifo
static unsigned I2C_fifo_write(i2c_dev_t *dev, void const *buf, unsigned count);
static unsigned I2C_fifo_read(i2c_dev_t *dev, void *buf, unsigned bufsize);

static ssize_t I2C_read(int fd, void *buf, size_t bufsize);
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
    {
        struct I2C_context *context = &i2c_context[0];
        context->dev = &I2C0;

        mutex_init(&context->lock, MUTEX_FLAG_RECURSIVE);
        sem_init_np(&context->evt, 0, 0, 1);
        esp_intr_alloc(ETS_I2C_EXT0_INTR_SOURCE, 0, I2C0_IntrHandler, context, &context->intr_hdl);
    }

    {
        struct I2C_context *context = &i2c_context[1];
        context->dev = &I2C1;

        mutex_init(&context->lock, MUTEX_FLAG_RECURSIVE);
        sem_init_np(&context->evt, 0, 0, 1);
        esp_intr_alloc(ETS_I2C_EXT1_INTR_SOURCE, 0, I2C1_IntrHandler, context, &context->intr_hdl);
    }

}

/****************************************************************************
 *  @implements
 ****************************************************************************/
int I2C_createfd(int nb, uint8_t da, uint16_t kbps, uint8_t page_size, uint32_t highest_addr)
{
    if (SOC_I2C_NUM <= nb)
        return __set_errno_neg(ENODEV);
    if (1000 < kbps || 0 == kbps)
        return __set_errno_neg(EINVAL);

    struct I2C_context *context = context = &i2c_context[nb];
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
    int retval;

    PERIPH_module_t i2c_module = I2C_periph_module(dev, NULL);
    if (PERIPH_MODULE_MAX == i2c_module)
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
        dev->int_clr.val= (uint32_t)~0;
        // fifo
        dev->fifo_conf.nonfifo_en = 0;
        dev->fifo_conf.tx_fifo_rst = dev->fifo_conf.rx_fifo_rst = 1;
        dev->fifo_conf.tx_fifo_rst = dev->fifo_conf.rx_fifo_rst = 0;
        // filter
        if (I2C_SCL_FILTER_CYCLE)
        {
            dev->filter_cfg.scl_filter_en = 1;
            dev->filter_cfg.scl_filter_thres = I2C_SCL_FILTER_CYCLE;
        }
        if (I2C_SDA_FILTER_CYCLE)
        {
            dev->filter_cfg.sda_filter_en = 1;
            dev->filter_cfg.sda_filter_thres = I2C_SDA_FILTER_CYCLE;
        }

        // i2c ctr master
        dev->ctr.val = 0;
        dev->ctr.ms_mode = 1;
        dev->ctr.clk_en = 1;
        dev->ctr.arbitration_en = 1;
        // dev->ctr.clk_en = 1; what is this?
        // scl/sda output mode. 0: direct output; 1: open drain output.
        dev->ctr.sda_force_out = 1;
        dev->ctr.scl_force_out = 1;
        // MSB
        dev->ctr.rx_lsb_first =  dev->ctr.tx_lsb_first = 0;
        // bps
        I2C_configure_bps(dev, kbps);
        // update
        dev->ctr.conf_upgate = 1;
        dev->clk_conf.sclk_active = 1;
        // intr
        dev->int_ena.val = I2C_NACK_INT_ENA_M | I2C_ARBITRATION_LOST_INT_ENA_M | I2C_TIME_OUT_INT_ENA_M |
            I2C_TRANS_COMPLETE_INT_ENA_M | I2C_END_DETECT_INT_ENA_M;
    }
    else
    {
        /// TODO: I2C slave mode
        // i2c ctr master
        dev->ctr.val = 0;
        dev->ctr.ms_mode = 0;

        // update
        dev->ctr.conf_upgate = 1;
        dev->clk_conf.sclk_active = 1;

        // intr
        dev->int_ena.val = I2C_RXFIFO_WM_INT_ENA_M | I2C_TRANS_COMPLETE_INT_ENA_M | I2C_TXFIFO_WM_INT_ENA_M;

        retval = ENOSYS;
        goto i2c_configure_fail_exit;
    }

    if (false)
    {
i2c_configure_fail_exit:
        CLK_periph_disable(i2c_module);
    }
    else
        retval = 0;

    return retval;
}

int I2C_deconfigure(i2c_dev_t *dev)
{
    int retval = CLK_periph_disable(I2C_periph_module(dev, NULL));

    if (0 == retval)
    {
        if (dev == &I2C0)
        {
            IOMUX_route_disconnect_signal(I2CEXT0_SCL_IN_IDX);
            IOMUX_route_disconnect_signal(I2CEXT0_SDA_IN_IDX);

            IOMUX_route_disconnect_signal(I2CEXT0_SCL_OUT_IDX);
            IOMUX_route_disconnect_signal(I2CEXT0_SDA_OUT_IDX);
        }
        else if (dev == &I2C1)
        {
            IOMUX_route_disconnect_signal(I2CEXT1_SCL_IN_IDX);
            IOMUX_route_disconnect_signal(I2CEXT1_SDA_IN_IDX);

            IOMUX_route_disconnect_signal(I2CEXT1_SCL_OUT_IDX);
            IOMUX_route_disconnect_signal(I2CEXT1_SDA_OUT_IDX);
        }
    }
}

uint32_t I2C_get_bps(i2c_dev_t *dev)
{
    PERIPH_module_t module = I2C_periph_module(dev, NULL);

    if (PERIPH_MODULE_MAX == module)
        return (uint32_t)__set_errno_nullptr(ENODEV);
    if (! CLK_periph_is_enabled(module))
        return (uint32_t)__set_errno_nullptr(EACCES);

    uint32_t cycle = (uint32_t)(dev->scl_low_period.scl_low_period +
        dev->scl_high_period.scl_wait_high_period +
        dev->scl_high_period.scl_high_period
    );
    if (0 == cycle)
        return (uint32_t)__set_errno_nullptr(EINVAL);
    else
        return (uint32_t)(CLK_i2c_sclk_freq(dev) / (dev->clk_conf.sclk_div_num + 1) / cycle);
}

/***************************************************************************
 *  @implements: dev IO
 ***************************************************************************/
static void tx_next(i2c_dev_t *dev, struct I2C_context *context, uint8_t idx)
{
    unsigned count = I2C_fifo_write(dev, context->tx_buf, (unsigned)(context->tx_buf_end - context->tx_buf));
    context->tx_buf += count;

    if (count)
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_WRITE, .io_bytes = BIT_FIELD(8, count), .ack_check_en = 1};
        I2C_command(dev, &idx, reg.val);
    }

    if (context->tx_buf_end == context->tx_buf)
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_STOP};
        I2C_command(dev, &idx, reg.val);
    }
    else    // tx fifo full
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_END};
        I2C_command(dev, &idx, reg.val);
    }

    I2C_command_start(dev);
}

static void rx_next(i2c_dev_t *dev, struct I2C_context *context, uint8_t idx)
{
    context->rx_buf += I2C_fifo_read(dev, context->rx_buf, (unsigned)(context->rx_buf_end - context->rx_buf));
    unsigned count = (unsigned)(context->rx_buf_end - context->rx_buf);

    if (1 == count)
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_READ, .io_bytes = 1, .ack_nack = 1};
        I2C_command(dev, &idx, reg.val);

        reg.val = 0;
        reg.op_code = I2C_CMD_STOP;
        I2C_command(dev, &idx, reg.val);
    }
    else
    {
        count = (count > SOC_I2C_FIFO_LEN ? SOC_I2C_FIFO_LEN : count) - 1;

        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_READ, .io_bytes = BIT_FIELD(8, count)};
        I2C_command(dev, &idx, reg.val);
    }
}

int I2C_dev_pread(i2c_dev_t *dev, uint16_t da, uint8_t *sa, uint8_t sa_bytes, void *buf, size_t bufsize)
{
    struct I2C_context *context = NULL;
    I2C_periph_module(dev, &context);

    context->err = 0;
    uint8_t idx = 0;

    // start
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_RESTART};
        I2C_command(dev, &idx, reg.val);
    }
    // da
    {
        uint8_t da_bytes = (uint8_t)(da << 1) | (0 == sa_bytes ? 1 : 0);
        I2C_fifo_write(dev, &da_bytes, 1);

        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_WRITE, .io_bytes = 1, .ack_check_en = 1};
        I2C_command(dev, &idx, reg.val);
    }
    if (sa_bytes)
    {
        I2C_fifo_write(dev, sa, sa_bytes);

        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_WRITE, .io_bytes = BIT_FIELD(8, sa_bytes), .ack_check_en = 1};
        I2C_command(dev, &idx, reg.val);
    }
    // pause
    /*
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_END};
        I2C_command(dev, &idx, reg.val);
    }
    */

    rx_next(dev, context, idx);
    sem_wait(&context->evt);

    ARG_UNUSED(buf, bufsize);
    return 0;
}

int I2C_dev_pwrite(i2c_dev_t *dev, uint16_t da, uint8_t *sa, uint8_t sa_bytes, void const *buf, size_t count)
{
    struct I2C_context *context = NULL;
    I2C_periph_module(dev, &context);

    context->err = 0;
    uint8_t idx = 0;

    // start
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_RESTART};
        I2C_command(dev, &idx, reg.val);
    }
    // da
    {
        uint8_t da_bytes = (uint8_t)(da << 1);
        I2C_fifo_write(dev, &da_bytes, 1);

        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_WRITE, .io_bytes = 1, .ack_check_en = 1};
        I2C_command(dev, &idx, reg.val);
    }
    if (sa_bytes)
    {
        I2C_fifo_write(dev, sa, sa_bytes);

        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_WRITE, .io_bytes = BIT_FIELD(8, sa_bytes), .ack_check_en = 1};
        I2C_command(dev, &idx, reg.val);
    }
    // pause
    /*
    {
        i2c_cmd_reg_t reg = {.op_code = I2C_CMD_END};
        I2C_command(dev, &idx, reg.val);
    }
    */
    context->tx_buf = (void *)buf;
    context->tx_buf_end = (uint8_t *)buf + count;

    tx_next(dev, context, idx);
    // I2C_command_start(dev);
    sem_wait(&context->evt);

    ARG_UNUSED(da, sa, sa_bytes, buf, count);
    return 0;
}

/***************************************************************************
 *  @internal
 ***************************************************************************/
static PERIPH_module_t I2C_periph_module(i2c_dev_t *dev, struct I2C_context **context)
{
    PERIPH_module_t module;

    if (&I2C0 == dev)
    {
        module = PERIPH_I2C0_MODULE;
        if (context)
            *context = &i2c_context[0];
    }
    else if (&I2C1 == dev)
    {
        module = PERIPH_I2C1_MODULE;
        if (context)
            *context = &i2c_context[1];
    }
    else
        module = PERIPH_MODULE_MAX;

    return module;
}

static void I2C_configure_bps(i2c_dev_t *dev, uint32_t kbps)
{
    uint32_t clk_freq = (uint32_t)CLK_i2c_sclk_freq(dev);
    uint32_t div = clk_freq / _MHZ / kbps;

    if (div > 0)
    {
        clk_freq = clk_freq / div;
        dev->clk_conf.sclk_div_num = BIT_FIELD(8, div - 1);
    }
    else
        dev->clk_conf.sclk_div_num = 0;

    uint32_t cycle = clk_freq / 1000 / kbps;
    uint32_t low_cycle = cycle / 2;
    uint32_t half_cycle = clk_freq / 1000 == cycle * kbps ? cycle - low_cycle : cycle - low_cycle + 1;

    // scl
    dev->scl_low_period.scl_low_period = BIT_FIELD(9, low_cycle);
    dev->scl_high_period.scl_wait_high_period = BIT_FIELD(7, 80 < kbps ? half_cycle / 4 : half_cycle / 3);
    dev->scl_high_period.scl_high_period = BIT_FIELD(9, half_cycle - dev->scl_high_period.scl_wait_high_period);
    // sda
    dev->sda_hold.sda_hold_time = BIT_FIELD(9, half_cycle / 2);
    dev->sda_sample.sda_sample_time = BIT_FIELD(9, half_cycle / 2 + dev->scl_high_period.scl_wait_high_period);

    // start
    dev->scl_start_hold.scl_start_hold_time = BIT_FIELD(9, half_cycle);
    dev->scl_rstart_setup.scl_rstart_setup_time = BIT_FIELD(9, half_cycle);
    // stop
    dev->scl_stop_setup.scl_stop_setup_time = BIT_FIELD(9, half_cycle);
    dev->scl_stop_hold.scl_stop_hold_time = BIT_FIELD(9, half_cycle);

    // timeout: 5 bits, max = 31;
    dev->to.time_out_value = 20;
    dev->to.time_out_en = 1;
}

static int I2C_lock(struct I2C_context *context)
{
    int retval = mutex_trylock(&context->lock, context->timeout);

    if (0 == retval)
    {
    }
    return retval;
}

static void I2C_unlock(struct I2C_context *context)
{
    if (0 == mutex_unlock(&context->lock))
    {
    }
}


/***************************************************************************
 *  @internal
 ***************************************************************************/
static void I2C_command_start(i2c_dev_t *dev)
{
    i2c_cmd_reg_t *cmd = (void *)&dev->comd0;
    for (int i = 0; i < 8; i ++)
    {
        i2c_cmd_reg_t reg = {.val = cmd[i].val};
        // if (! reg.done)
            esp_rom_printf("idx: %d, cmd: %d, bytes:%d, ack_en: %d, done: %d\n", i, reg.op_code, reg.io_bytes, reg.ack_check_en, reg.done);
    }

    dev->ctr.trans_start = 1;
}

static void I2C_command(i2c_dev_t *dev, uint8_t *idx, uint32_t regval)
{
    (&dev->comd0)[*idx].val = regval;
    *idx = (*idx + 1) & 0x7;    // i2c command length is 8
}

static unsigned I2C_fifo_write(i2c_dev_t *dev, void const *buf, unsigned count)
{
    unsigned written = 0;

    while (written < count && SOC_I2C_FIFO_LEN > dev->sr.txfifo_cnt)
    {
        dev->data.fifo_rdata = *(uint8_t *)buf;

        (uint8_t *)buf ++;
        written ++;
    }
    return written;
}

static unsigned I2C_fifo_read(i2c_dev_t *dev, void *buf, unsigned bufsize)
{
    unsigned readed = 0;

    while (readed < bufsize && 0 != dev->sr.rxfifo_cnt)
    {
        *(uint8_t *)buf = (uint8_t)dev->data.fifo_rdata;

        (uint8_t *)buf ++;
        readed ++;
    }
    return readed;
}

/***************************************************************************
 *  @internal: fd IO
 ***************************************************************************/
static ssize_t I2C_read(int fd, void *buf, size_t bufsize)
{
    uint32_t timeout = AsFD(fd)->read_timeo;
    if (0 == timeout)
        timeout = INFINITE;

    ARG_UNUSED(buf, bufsize);
    return 0;
}

static ssize_t I2C_write(int fd, void const *buf, size_t count)
{
    uint32_t timeout = AsFD(fd)->write_timeo;
    if (0 == timeout)
        timeout = INFINITE;

    ARG_UNUSED(buf, count);
    return 0;
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
        I2C_deconfigure(context->dev);

    KERNEL_mfree(ext);
    return 0;
}

/****************************************************************************
 *  intr
 ****************************************************************************/
static void I2C_IntrHandler(struct I2C_context *context)
{
    i2c_dev_t *dev = context->dev;
    i2c_int_status_reg_t status = {.val = dev->int_status.val};

    if (status.arbitration_lost_int_st)
    {
        esp_rom_printf("arbitration_lost_int_st\n");
        context->err = ENXIO;
        dev->ctr.fsm_rst = 1;

        goto i2c_transfer_done;
    }
    if (status.time_out_int_st)
    {
        esp_rom_printf("time_out_int_st\n");
        context->err = ETIMEDOUT;
        goto i2c_transfer_done;
    }
    if (status.trans_complete_int_st)
    {
        esp_rom_printf("trans_complete_int_st\n");
        goto i2c_transfer_done;
    }

    if (status.end_detect_int_st)
    {
        esp_rom_printf("end_detect_int_st\n");

        if (0 != context->err)
            goto i2c_transfer_done;

        if (context->tx_buf && context->tx_buf < context->tx_buf_end)
            tx_next(dev, context, 0);
        else if (context->rx_buf && context->rx_buf < context->tx_buf_end)
            rx_next(dev, context, 0);
        else
            goto i2c_transfer_done;
    }

    if (status.nack_int_st)
    {
        esp_rom_printf("nack_int_st\n");
        context->err = ENXIO;
    }

    if (false)
    {
i2c_transfer_done:
        sem_post(&context->evt);
    }

    dev->int_clr.val = status.val;
}

void I2C0_IntrHandler(void *arg)
    __attribute__((alias("I2C_IntrHandler")));

void I2C1_IntrHandler(void *arg)
    __attribute__((alias("I2C_IntrHandler")));
