#include <soc.h>
#include "sh/ucsh.h"

int UCSH_reset(struct UCSH_env *env)
{
    ARG_UNUSED(env);
    SOC_reset();
}
