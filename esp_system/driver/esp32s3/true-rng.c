#include <unistd.h>

#include "soc/wdev_reg.h"
#include "clk_tree.h"

__attribute__((constructor))
void TRNG_initialize(void)
{
    /*
        RC_FAST_CLK
        SAR ADC
        High Speed ADC
    */
    periph_module_enable(PERIPH_RNG_MODULE);
    CLK_SCLK_RC_FAST_ref();

    // what aobut SAR ADC?
}

static unsigned rand_seed = 0;

void __srand(unsigned seed)
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
