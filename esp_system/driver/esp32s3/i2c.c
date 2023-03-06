#include <sys/errno.h>
#include "clk_tree.h"

#include "i2c.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
void UART0_IntrHandler(void *arg);
void UART2_IntrHandler(void *arg);
void UART1_IntrHandler(void *arg);

struct I2C_addressing
{
    uint16_t da;                    // address of device
    uint8_t ridx_bytes;             // bytes of sub address been used: 0, 1, 2, 3
    uint8_t ridx[4];                // sub addresses of device (highest REGISTER no)
};

struct I2C_context
{
    i2c_dev_t *DEV;
    uint32_t fd_count;

    // sem_t lock;
    // sem_t evt;
    int err;

    struct I2C_addressing addressing;
    uint8_t ridx_wpos;
    uint8_t page_size;
    bool ridx_sending;
    bool hi_testing;

    uint8_t *buf;
    uint32_t bufsize;
};

struct I2C_fd_ext
{
    struct I2C_context *context;

    uint16_t da;
    uint8_t page_size;
    uint8_t ridx_bytes;
    uint32_t highest_addr;

    struct
    {
        uint32_t CTRL;
        uint32_t CLKDIV;
    } REG;
};
/****************************************************************************
 *  constructor
 ****************************************************************************/
__attribute__((constructor))
void I2C_initialize()
{

}

/****************************************************************************
 *  @implements
 ****************************************************************************/
int I2C_configure(i2c_dev_t *dev, enum I2C_mode_t mode, uint32_t kbps)
{
    PERIPH_module_t i2c_module;
    int retval;

    if (&I2C0 == dev)
        i2c_module = PERIPH_I2C0_MODULE;
    else if (&I2C1 == dev)
        i2c_module = PERIPH_I2C1_MODULE;
    else
        return ENODEV;

    CLK_periph_enable(i2c_module);

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

/****************************************************************************
 *  intr
 ****************************************************************************/
static void I2C_IntrHandler(struct I2C_context *context)
{
}
