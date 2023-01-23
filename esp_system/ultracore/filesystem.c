#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>

#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/reent.h>

#include "esp_log.h"

static const char *TAG = "filesystem";

void __FILESYSTEM_init(void)
{
    // nothing to do but import retarget functions
}

int _open_r(struct _reent *r, char const *path, int flags, int mode)
{
    __set_errno_neg(r, ENOSYS);
    return 0x1234;
}

int _close_r(struct _reent *r, int fd)
{
    return __set_errno_neg(r, ENOSYS);
}

int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    return __set_errno_neg(r, ENOSYS);
}

int _fcntl_r(struct _reent *r, int fd, int cmd, int arg)
{
    return __set_errno_neg(r, ENOSYS);
}

int _stat_r(struct _reent *r, char const *path, struct stat *st)
{
    int fd = _open_r(r, path, 0, 0);

    if (0 > fd)
        return fd;

    int retval = _fstat_r(r, fd, st);
    _close_r(r, fd);

    return retval;
}