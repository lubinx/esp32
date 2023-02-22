#include <string.h>
#include <sys/errno.h>
#include <sys/random.h>

/**
 *  not posix, but linux
 *      TODO: plan to implement /dev/random in file system
*/
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    ARG_UNUSED(flags);

    if (NULL == buf)
        return __set_errno_neg(EFAULT);

    size_t len = buflen;
    int align_offset = (uintptr_t)buf & ~(sizeof(int) - 1);

    if (align_offset)
    {
        int rng = rand();
        memcpy(buf, &rng, align_offset);

        buf = (uint8_t *)buf + align_offset;
        len -= align_offset;
    }

    while (len > sizeof(int))
    {
        *(int *)buf = rand();

        buf = (int *)buf + 1;
        len -= sizeof(int);
    }

    if (len > 0)
    {
        int rng = rand();
        memcpy(buf, &rng, len);
    }

    return (ssize_t)buflen;
}
