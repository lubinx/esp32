#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>

#include <sys/random.h>
#include "esp_random.h"

/***************************************************************************
 *  @implements: pseudo-rng
 ***************************************************************************/
struct PRNG_ctx
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};
static struct PRNG_ctx PRNG_ctx;

__attribute__((weak))
int rand(void)
{
    #define rotate(x, k)    (((x) << (k)) | ((x) >> (32 - (k))))

    uint32_t e = PRNG_ctx.a - rotate(PRNG_ctx.b, 27);
    PRNG_ctx.a = PRNG_ctx.b ^ rotate(PRNG_ctx.c, 17);

    PRNG_ctx.b = PRNG_ctx.c + PRNG_ctx.d;
    PRNG_ctx.c = PRNG_ctx.d + e;
    PRNG_ctx.d = e + PRNG_ctx.a;

    return (int)PRNG_ctx.d;

    #undef rotate
}

__attribute__((weak))
void __srand(unsigned __seed)
{
    uint32_t i;
    PRNG_ctx.a = 0xf1ea5eed;
    PRNG_ctx.b = PRNG_ctx.c = PRNG_ctx.d = __seed;

    // more unpredictable
    for (i = 0; i < (rand() & 0xFF); i ++)
        rand();
}

/***************************************************************************
 *  @implements: sys/random.h
 *      TODO: plan to implement /dev/random in file system
 ***************************************************************************/
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    ARG_UNUSED(flags);

    if (NULL == buf)
        return __set_errno_neg(EFAULT);

    size_t len = buflen;
    int align_offset = (uintptr_t)buf & (sizeof(int) - 1);

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

/***************************************************************************
 *  @implements
 ***************************************************************************/
#pragma GCC diagnostic ignored "-Wattribute-alias"

uint32_t esp_random(void)
    __attribute__((alias("rand")));

int esp_fill_random(void *buf, size_t len)
    __attribute__((alias("getrandom")));
