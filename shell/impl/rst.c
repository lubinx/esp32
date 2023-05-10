#include <ultracore.h>
#include "sh/ucsh.h"

int UCSH_reset(struct UCSH_env *env)
{
    ARG_UNUSED(env);

    SYSTEM_reset();
    while (1);
}
