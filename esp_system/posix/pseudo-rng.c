#include <stdint.h>
#include <stdlib.h>

struct PRNG_ctx
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};
static struct PRNG_ctx ctx;

__attribute__((weak))
void __srand(unsigned __seed)
{
    uint32_t i;
    ctx.a = 0xf1ea5eed;
    ctx.b = ctx.c = ctx.d = __seed;

    // more unpredictable
    for (i = 0; i < (rand() & 0xFF); i ++)
        rand();
}

__attribute__((weak))
int rand(void)
{
    #define rotate(x, k)    (((x) << (k)) | ((x) >> (32 - (k))))

    uint32_t e = ctx.a - rotate(ctx.b, 27);
    ctx.a = ctx.b ^ rotate(ctx.c, 17);

    ctx.b = ctx.c + ctx.d;
    ctx.c = ctx.d + e;
    ctx.d = e + ctx.a;

    return (int)ctx.d;

    #undef rotate
}
