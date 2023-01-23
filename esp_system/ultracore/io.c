#include <unistd.h>
#include <sys/reent.h>
#include <sys/errno.h>

#include "esp_log.h"

static char const *TAG = "io";

void __IO_retarget(void)
{
    // nothing to do but import retarget functions
}

ssize_t _read_r(struct _reent *r, int fd, void * dst, size_t size)
{
    return __set_errno_neg(r, ENOSYS);
}

ssize_t _write_r(struct _reent *r, int fd, const void * data, size_t size)
{
    return __set_errno_neg(r, ENOSYS);
}

off_t _lseek_r(struct _reent *r, int fd, off_t size, int mode)
{
    return __set_errno_neg(r, ENOSYS);
}

/*
static void uart_tx_char(int fd, int c)
{
    uart_dev_t* uart = s_ctx[fd]->uart;
    const uint8_t ch = (uint8_t) c;

    while (uart_ll_get_txfifo_len(uart) < 2) {
        ;
    }

    uart_ll_write_txfifo(uart, &ch, 1);
}

static void uart_tx_char_via_driver(int fd, int c)
{
    char ch = (char) c;
    uart_write_bytes(fd, &ch, 1);
}
*/
