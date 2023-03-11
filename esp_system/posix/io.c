#include <unistd.h>
#include <sched.h>
#include <reent.h>
#include <stropts.h>

#include <sys/uio.h>
#include <sys/errno.h>

#include <rtos/kernel.h>

#include "esp_log.h"

void __IO_retarget(void)
{
    // nothing to do but import retarget functions
}

/***************************************************************************/
/** exports
****************************************************************************/
int __stdin_fd = -1;
int __stdout_fd = -1;
int __stderr_fd = -1;

/***************************************************************************/
/** @io.h implement
****************************************************************************/
int isastream(int fd)
{
    return fd > 0 && CID_FD == AsFD(fd)->cid;
}

int _isatty_r(struct _reent *r, int fd)
{
    if (STDIN_FILENO == fd || STDOUT_FILENO == fd || STDERR_FILENO == fd)
        return 1;
    if (0 >= fd)
        return (int)__set_errno_r_nullptr(r, EBADF);

    if (FD_TAG_CHAR & AsFD(fd)->tag)
        return 1;
    else
        return (int)__set_errno_r_nullptr(r, ENOTTY);
}

int _close_r(struct _reent *r, int fd)
{
    if (0 >= fd || CID_FD != AsFD(fd)->cid)
        return __set_errno_r_neg(r, EBADF);

    int err = KERNEL_handle_release((handle_t)fd);

    if (0 == err)
        return 0;
    else
        return __set_errno_r_neg(r, err);
}

/*
int ioctl(int fd, unsigned long int request, ...)
{
    if (0 >= fd || CID_FD != AsFD(fd)->cid)
        return EBADF;

    va_list vl;
    va_start(vl, request);
    int retval = 0;

    switch (request)
    {
    case OPT_RD_TIMEO:
        AsFD(fd)->read_timeo = va_arg(vl, unsigned int);
        break;

    case OPT_WR_TIMEO:
        AsFD(fd)->write_timeo = va_arg(vl, unsigned int);
        break;

    default:
        if (AsFD(fd)->implement->ioctl)
            retval = AsFD(fd)->implement->ioctl(fd, request, vl);
        else
            retval = __set_errno_neg(ENOSYS);
        break;
    }

    va_end(vl);
    return retval;
}
*/

off_t _lseek_r(struct _reent *r, int fd, off_t offset, int origin)
{
    if (0 >= fd || CID_FD != AsFD(fd)->cid)
        return __set_errno_r_neg(r, EBADF);
    if ((FD_TAG_SOCKET & AsFD(fd)->tag) || (FD_TAG_FIFO & AsFD(fd)->tag))
        return __set_errno_r_neg(r, ESPIPE);

    if (NULL == AsFD(fd)->implement->seek)
        return __set_errno_r_neg(r, EPERM);
    else
        return AsFD(fd)->implement->seek(fd, offset, origin);
}

off_t tell(int fd)
{
    return lseek(fd, 0, SEEK_CUR);
}

ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t bufsize)
{
    if (fd == STDIN_FILENO)
    {
        if (-1 == __stdin_fd)
            return __set_errno_r_neg(r, ENOSYS);
        else
            fd = __stdin_fd;
    }

    if (0 >= fd || CID_FD != AsFD(fd)->cid)
        return __set_errno_r_neg(r, EBADF);
    if (0 == bufsize)
        return 0;

    /*
    if (FD_TAG_DIR & AsFD(fd)->tag)
        return __set_errno_r_neg(r, EISDIR);
    */

    if (NULL == AsFD(fd)->implement->read)
        return __set_errno_r_neg(r, EPERM);
    else
        return AsFD(fd)->implement->read(fd, buf, bufsize);
}

ssize_t pread(int fd, void *buf, size_t bufsize, off_t offset)
{
    if (lseek(fd, offset, SEEK_SET) != offset)
        return __set_errno_neg(EINVAL);
    else
        return read(fd, buf, bufsize);
}

ssize_t readbuf(int fd, void *buf, size_t bufsize)
{
    int readed = 0;
    struct _reent *r = __getreent();

    while ((size_t)readed < bufsize)
    {
        int reading = _read_r(r, fd, (uint8_t *)buf + readed, bufsize - (size_t)readed);

        if (reading <= 0)
        {
            if (EAGAIN == r->_errno)
            {
                sched_yield();
                continue;
            }
            else
            {
                readed = -1;
                break;
            }
        }
        else
            readed += reading;
    }
    return readed;
}

ssize_t readln(int fd, char *buf, size_t bufsize)
{
    int readed = 0;
    struct _reent *r = __getreent();

    while ((size_t)readed < bufsize)
    {
        char CH;

        if (_read_r(r, fd, &CH, sizeof(CH)) < 0)
        {
            if (EAGAIN == r->_errno)
            {
                sched_yield();
                continue;
            }
            else
                return -1;
        }

        if ('\n' == CH)
        {
            if ( '\r' == *(buf - 1))
            {
                buf --;
                readed --;
            }

            *buf = '\0';
            return readed;
        }
        else
        {
            *buf = CH;

            buf ++;
            readed ++;
        }
    }

    return __set_errno_r_neg(r, EMSGSIZE);
}

__attribute__((weak))
ssize_t console_write(void const *buf, size_t count)
{
    return count;
}

ssize_t _write_r(struct _reent *r, int fd, void const *buf, size_t count)
{
    bool console_io;

    switch (fd)
    {
    case STDOUT_FILENO:
        fd = __stdout_fd;
        console_io = true;
        break;
    case STDERR_FILENO:
        fd = __stderr_fd;
        console_io = true;
        break;
    default:
        console_io = false;
    }

    if (0 >= fd)
    {
        if (console_io)
            return console_write(buf, count);
        else
            return __set_errno_r_neg(r, ENOSYS);
    }

    if (CID_FD != AsFD(fd)->cid)
        return __set_errno_r_neg(r, EBADF);
    if (0 == count)
        return 0;

    if (NULL == AsFD(fd)->implement->write)
        return __set_errno_r_neg(r, EPERM);
    else
        return AsFD(fd)->implement->write(fd, buf, count);
}

ssize_t pwrite(int fd, void const *buf, size_t count, off_t offset)
{
    if (lseek(fd, offset, SEEK_SET) != offset)
        return __set_errno_neg(EINVAL);
    else
        return write(fd, buf, count);
}

ssize_t writebuf(int fd, void const *buf, size_t count)
{
    int written = 0;
    struct _reent *r = __getreent();

    while ((size_t)written < count)
    {
        int writting = _write_r(r, fd, (uint8_t const *)buf + written, count - (size_t)written);

        if (writting <= 0)
        {
            if (EAGAIN == r->_errno)
            {
                sched_yield();
                continue;
            }
            else
            {
                written = -1;
                break;
            }
        }
        else
            written += writting;
    }
    return written;
}

ssize_t writeln(int fd, char const *buf, size_t count)
{
    if (count > 0)
    {
        if (writebuf(fd, buf, count) != (ssize_t)count)
            return -1;
    }

    if (2 != writebuf(fd, "\r\n", 2))
        return -1;
    else
        return (ssize_t)count;
}

/***************************************************************************/
/** @uio.h implement
****************************************************************************/
ssize_t readv(int fd, struct iovec const *iov, int iovcnt)
{
    if (iovcnt <= 0 || iovcnt > IOV_MAX)
        return __set_errno_neg(EINVAL);
    else
        return readbuf(fd, iov->oiv_base, iov->iov_len * (size_t)iovcnt);
}

ssize_t writev(int fd, struct iovec const *iov, int iovcnt)
{
    if (iovcnt <= 0 || iovcnt > IOV_MAX)
        return __set_errno_neg(EINVAL);
    else
        return writebuf(fd, iov->oiv_base, iov->iov_len * (size_t)iovcnt);
}
