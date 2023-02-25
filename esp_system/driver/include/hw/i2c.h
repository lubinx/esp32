/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __HW_I2C_H
#define __HW_I2C_H                      1

#include <features.h>
#include <stdint.h>

__BEGIN_DECLS

    #define HW_I2C0                     (0)
    #define HW_I2C1                     (1)
    #define HW_I2C2                     (2)
    #define HW_I2C3                     (3)
    #define HW_I2C4                     (4)
    #define HW_I2C5                     (5)
    #define HW_I2C6                     (6)
    #define HW_I2C7                     (7)
    #define HW_I2C8                     (8)
    #define HW_I2C9                     (9)

/****************************************************************************
 *  i2c fd
 ****************************************************************************/
extern __attribute__((nothrow))
    int I2C_createfd(int nb, uint8_t da, uint16_t kbps, uint8_t page_size, uint32_t highest_addr);

    #define __I2C_OPT                   ('I' << 8)
/****************************************************************************
 *  i2c ioctrl
 ****************************************************************************/
    /// sub address @highest value
    ///     256/65536 etc..
    #define I2C_SA_HIGHEST              (__I2C_OPT | 30)
    /// sub @address value
    #define I2C_SA                      (__I2C_OPT | 31)

/****************************************************************************
 *  linux i2c ioctl stuff
 *      merge from linux/i2c.h and linux/i2c-dev.h
 *      most of these stuff is not supported
 ****************************************************************************/
    /// number of times a device address should be polled when not acknowledging
    #define I2C_RETRIES	                (__I2C_OPT | 1)
    /// set timeout in units of 10 ms
    #define I2C_TIMEOUT                 (__I2C_OPT | 2)

    /// slave address, 7 or 10 bits address, 10bits slave address is not supported
    #define I2C_SLAVE                   (__I2C_OPT | 3)
    /// @unsupported: 0 for 7 bit addrs, != 0 for 10 bit
    #define I2C_TENBIT                  (__I2C_OPT | 4)
    /// @unsupported: Get the adapter functionality mask
    #define I2C_FUNCS                   (__I2C_OPT | 5)
    /// Use this slave address, even if it is already in use by a driver!
    #define I2C_SLAVE_FORCE	            (__I2C_OPT | 6)

    /// Combined R/W transfer (one STOP only)
    ///     ultracore can done this by simply lseek()
    #define I2C_RDWR                    (__I2C_OPT | 7)

    // != 0 to use PEC with @SMBus
    #define I2C_PEC                     (__I2C_OPT | 8)
    /// SMBus transfer
    #define I2C_SMBUS                   (__I2C_OPT | 20)

    // This is the structure as used in the I2C_RDWR ioctl call
    struct i2c_rdwr_ioctl_data
    {
        struct i2c_msg *msgs;
        uint32_t nmsgs;
    };
    #define  I2C_RDWR_IOCTL_MAX_MSGS    42

    struct i2c_msg
    {
        uint16_t addr;
        uint16_t flags;
            // read data, from slave to master I2C_M_RD is guaranteed to be 0x0001!
            #define I2C_M_RD            0x0001
            // this is a ten bit chip address
            #define I2C_M_TEN           0x0010
            // the buffer of this message is DMA safe makes only sense in kernelspace userspace buffers are copied anyway
            #define I2C_M_DMA_SAFE      0x0200
            // length will be first received byte
            #define I2C_M_RECV_LEN      0x0400
            //  if I2C_FUNC_PROTOCOL_MANGLING
            #define I2C_M_NO_RD_ACK     0x0800
            // if I2C_FUNC_PROTOCOL_MANGLING
            #define I2C_M_IGNORE_NAK    0x1000
            // if I2C_FUNC_PROTOCOL_MANGLING
            #define I2C_M_REV_DIR_ADDR  0x2000
            // if I2C_FUNC_NOSTART
            #define I2C_M_NOSTART       0x4000
            // if I2C_FUNC_PROTOCOL_MANGLING
            #define I2C_M_STOP          0x8000
        // msg length
        uint16_t len;
        // pointer to msg data
        uint8_t *buf;
    };

/* To determine what functionality is present */

    #define I2C_FUNC_I2C                0x00000001
    #define I2C_FUNC_10BIT_ADDR         0x00000002
    /* I2C_M_IGNORE_NAK etc. */
    #define I2C_FUNC_PROTOCOL_MANGLING  0x00000004
    #define I2C_FUNC_SMBUS_PEC          0x00000008
    /* I2C_M_NOSTART */
    #define I2C_FUNC_NOSTART            0x00000010
    #define I2C_FUNC_SLAVE              0x00000020

    /* SMBus 2.0 */
    #define I2C_FUNC_SMBUS_BLOCK_PROC_CALL  0x00008000
    #define I2C_FUNC_SMBUS_QUICK        0x00010000
    #define I2C_FUNC_SMBUS_READ_BYTE    0x00020000
    #define I2C_FUNC_SMBUS_WRITE_BYTE   0x00040000
    #define I2C_FUNC_SMBUS_READ_BYTE_DATA   0x00080000
    #define I2C_FUNC_SMBUS_WRITE_BYTE_DATA  0x00100000
    #define I2C_FUNC_SMBUS_READ_WORD_DATA   0x00200000
    #define I2C_FUNC_SMBUS_WRITE_WORD_DATA  0x00400000
    #define I2C_FUNC_SMBUS_PROC_CALL    0x00800000
    #define I2C_FUNC_SMBUS_READ_BLOCK_DATA  0x01000000
    #define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA 0x02000000
    #define I2C_FUNC_SMBUS_READ_I2C_BLOCK   0x04000000  /* I2C-like block xfer  */
    #define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK  0x08000000  /* w/ 1-byte reg. addr. */
    #define I2C_FUNC_SMBUS_HOST_NOTIFY  0x10000000

    #define I2C_FUNC_SMBUS_BYTE             \
        (I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE)
    #define I2C_FUNC_SMBUS_BYTE_DATA        \
        (I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_WRITE_BYTE_DATA)
    #define I2C_FUNC_SMBUS_WORD_DATA        \
        (I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_WRITE_WORD_DATA)
    #define I2C_FUNC_SMBUS_BLOCK_DATA       \
        (I2C_FUNC_SMBUS_READ_BLOCK_DATA | I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)
    #define I2C_FUNC_SMBUS_I2C_BLOCK        \
        (I2C_FUNC_SMBUS_READ_I2C_BLOCK | I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)

    #define I2C_FUNC_SMBUS_EMUL             (\
        I2C_FUNC_SMBUS_QUICK |      \
        I2C_FUNC_SMBUS_BYTE |       \
        I2C_FUNC_SMBUS_BYTE_DATA |  \
        I2C_FUNC_SMBUS_WORD_DATA |  \
        I2C_FUNC_SMBUS_PROC_CALL |  \
        I2C_FUNC_SMBUS_WRITE_BLOCK_DATA | \
        I2C_FUNC_SMBUS_I2C_BLOCK |  \
        I2C_FUNC_SMBUS_PEC   \
    )

    /* This is the structure as used in the I2C_SMBUS ioctl call */
    struct i2c_smbus_ioctl_data
    {
        uint8_t read_write;
        uint8_t command;
        uint32_t size;
        union i2c_smbus_data *data;
    };

/*
 * Data for SMBus Messages
 */
    #define I2C_SMBUS_BLOCK_MAX         32 /* As specified in SMBus standard */
    union i2c_smbus_data
    {
        uint8_t byte;
        uint16_t word;
        // block[0] is used for length and one more for user-space compatibility
        uint8_t block[I2C_SMBUS_BLOCK_MAX + 2];
    };

    /* i2c_smbus_xfer read or write markers */
    #define I2C_SMBUS_READ              1
    #define I2C_SMBUS_WRITE             0

    /* SMBus transaction types (size parameter in the above functions)
    Note: these no longer correspond to the (arbitrary) PIIX4 internal codes! */
    #define I2C_SMBUS_QUICK             0
    #define I2C_SMBUS_BYTE              1
    #define I2C_SMBUS_BYTE_DATA         2
    #define I2C_SMBUS_WORD_DATA         3
    #define I2C_SMBUS_PROC_CALL         4
    #define I2C_SMBUS_BLOCK_DATA        5
    #define I2C_SMBUS_I2C_BLOCK_BROKEN  6
    #define I2C_SMBUS_BLOCK_PROC_CALL   7 /* SMBus 2.0 */
    #define I2C_SMBUS_I2C_BLOCK_DATA    8

__END_DECLS
#endif
