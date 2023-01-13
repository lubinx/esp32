#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/reent.h>
#include <sys/unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


// Warn about using deprecated option
#ifdef CONFIG_LWIP_USE_ONLY_LWIP_SELECT
#warning CONFIG_LWIP_USE_ONLY_LWIP_SELECT is deprecated: Please use CONFIG_VFS_SUPPORT_SELECT instead
#endif

#ifdef CONFIG_VFS_SUPPRESS_SELECT_DEBUG_OUTPUT
#define LOG_LOCAL_LEVEL ESP_LOG_NONE
#endif //CONFIG_VFS_SUPPRESS_SELECT_DEBUG_OUTPUT
#include "esp_log.h"

static const char *TAG = "vfs";

ssize_t esp_vfs_write(struct _reent *r, int fd, const void * data, size_t size)
{
}

off_t esp_vfs_lseek(struct _reent *r, int fd, off_t size, int mode)
{
}

ssize_t esp_vfs_read(struct _reent *r, int fd, void * dst, size_t size)
{
}

ssize_t esp_vfs_pread(int fd, void *dst, size_t size, off_t offset)
{
}

ssize_t esp_vfs_pwrite(int fd, const void *src, size_t size, off_t offset)
{
}

int esp_vfs_close(struct _reent *r, int fd)
{
}

int esp_vfs_fstat(struct _reent *r, int fd, struct stat * st)
{
}

int esp_vfs_fcntl_r(struct _reent *r, int fd, int cmd, int arg)
{
}

int esp_vfs_ioctl(int fd, int cmd, ...)
{
}

int esp_vfs_fsync(int fd)
{
}

#ifdef CONFIG_VFS_SUPPORT_DIR

int esp_vfs_stat(struct _reent *r, const char * path, struct stat * st)
{
}

int esp_vfs_utime(const char *path, const struct utimbuf *times)
{
}

int esp_vfs_link(struct _reent *r, const char* n1, const char* n2)
{
}

int esp_vfs_unlink(struct _reent *r, const char *path)
{
}

int esp_vfs_rename(struct _reent *r, const char *src, const char *dst)
{
}

DIR* esp_vfs_opendir(const char* name)
{
}

struct dirent* esp_vfs_readdir(DIR* pdir)
{
}

int esp_vfs_readdir_r(DIR* pdir, struct dirent* entry, struct dirent** out_dirent)
{
}

long esp_vfs_telldir(DIR* pdir)
{
}

void esp_vfs_seekdir(DIR* pdir, long loc)
{
}

void esp_vfs_rewinddir(DIR* pdir)
{
    seekdir(pdir, 0);
}

int esp_vfs_closedir(DIR* pdir)
{
}

int esp_vfs_mkdir(const char* name, mode_t mode)
{
}

int esp_vfs_rmdir(const char* name)
{
}

int esp_vfs_access(const char *path, int amode)
{
}

int esp_vfs_truncate(const char *path, off_t length)
{
}

int esp_vfs_ftruncate(int fd, off_t length)
{
}


/* Create aliases for newlib syscalls

   These functions are also available in ROM as stubs which use the syscall table, but linking them
   directly here saves an additional function call when a software function is linked to one, and
   makes linking with -stdlib easier.
 */
#ifdef CONFIG_VFS_SUPPORT_IO
int _open_r(struct _reent *r, const char * path, int flags, int mode)
    __attribute__((alias("esp_vfs_open")));
int _close_r(struct _reent *r, int fd)
    __attribute__((alias("esp_vfs_close")));
ssize_t _read_r(struct _reent *r, int fd, void * dst, size_t size)
    __attribute__((alias("esp_vfs_read")));
ssize_t _write_r(struct _reent *r, int fd, const void * data, size_t size)
    __attribute__((alias("esp_vfs_write")));
ssize_t pread(int fd, void *dst, size_t size, off_t offset)
    __attribute__((alias("esp_vfs_pread")));
ssize_t pwrite(int fd, const void *src, size_t size, off_t offset)
    __attribute__((alias("esp_vfs_pwrite")));
off_t _lseek_r(struct _reent *r, int fd, off_t size, int mode)
    __attribute__((alias("esp_vfs_lseek")));
int _fcntl_r(struct _reent *r, int fd, int cmd, int arg)
    __attribute__((alias("esp_vfs_fcntl_r")));
int _fstat_r(struct _reent *r, int fd, struct stat * st)
    __attribute__((alias("esp_vfs_fstat")));
int fsync(int fd)
    __attribute__((alias("esp_vfs_fsync")));
int ioctl(int fd, int cmd, ...)
    __attribute__((alias("esp_vfs_ioctl")));
#endif // CONFIG_VFS_SUPPORT_IO


#ifdef CONFIG_VFS_SUPPORT_DIR
int _stat_r(struct _reent *r, const char * path, struct stat * st)
    __attribute__((alias("esp_vfs_stat")));
int _link_r(struct _reent *r, const char* n1, const char* n2)
    __attribute__((alias("esp_vfs_link")));
int _unlink_r(struct _reent *r, const char *path)
    __attribute__((alias("esp_vfs_unlink")));
int _rename_r(struct _reent *r, const char *src, const char *dst)
    __attribute__((alias("esp_vfs_rename")));
int truncate(const char *path, off_t length)
    __attribute__((alias("esp_vfs_truncate")));
int ftruncate(int fd, off_t length)
    __attribute__((alias("esp_vfs_ftruncate")));
int access(const char *path, int amode)
    __attribute__((alias("esp_vfs_access")));
int utime(const char *path, const struct utimbuf *times)
    __attribute__((alias("esp_vfs_utime")));
int rmdir(const char* name)
    __attribute__((alias("esp_vfs_rmdir")));
int mkdir(const char* name, mode_t mode)
    __attribute__((alias("esp_vfs_mkdir")));
DIR* opendir(const char* name)
    __attribute__((alias("esp_vfs_opendir")));
int closedir(DIR* pdir)
    __attribute__((alias("esp_vfs_closedir")));
int readdir_r(DIR* pdir, struct dirent* entry, struct dirent** out_dirent)
    __attribute__((alias("esp_vfs_readdir_r")));
struct dirent* readdir(DIR* pdir)
    __attribute__((alias("esp_vfs_readdir")));
long telldir(DIR* pdir)
    __attribute__((alias("esp_vfs_telldir")));
void seekdir(DIR* pdir, long loc)
    __attribute__((alias("esp_vfs_seekdir")));
void rewinddir(DIR* pdir)
    __attribute__((alias("esp_vfs_rewinddir")));
#endif
