#include <features.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/random.h>
#include <sys/param.h>

#include "soc/wdev_reg.h"
#include "esp_attr.h"

static unsigned rand_seed = 0;

void srand(unsigned seed)
{
    rand_seed = seed;
}

int rand(void)
{
    int rng = REG_READ(WDEV_RND_REG);
    int retval;

    // mix pesudo-rng and true-rng
    if (rand_seed)
        retval = (int)((uint64_t)rng * rand_seed) ^ rand_seed;
    else
        retval = rng;

    rand_seed = rng;
    return retval;
}

/**
 *  not posix, but linux
 *      TODO: plan to implement /dev/random in file system
*/
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    if (NULL == buf)
    {
        errno = EFAULT;
        return -1;
    }

    int len = buflen;
    while (len > 0)
    {
        int rng = rand();
        int to_copy = MIN(sizeof(rng), len);
        memcpy(buf, &rng, to_copy);

        buf = (void *)((uintptr_t)buf + to_copy);
        len -= to_copy;
    }

    return buflen;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"

uint32_t esp_random(void)
    __attribute__((alias("rand")));

int esp_fill_random(void *buf, size_t len)
    __attribute__((alias("getrandom")));

#pragma GCC diagnostic pop
