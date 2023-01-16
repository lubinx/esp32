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
    ESP_LOGW(TAG, "_open_r");
    while (1);
}

int _close_r(struct _reent *r, int fd)
{
    ESP_LOGW(TAG, "_close_r");
    while (1);
}

int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    ESP_LOGW(TAG, "_fstat_r");
    while (1);
}

int _fcntl_r(struct _reent *r, int fd, int cmd, int arg)
{
    ESP_LOGW(TAG, "_fcntl_r");
    while (1);
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
