#include <sys/errno.h>
#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_nvm(struct UCSH_env *env)
{
    ARG_UNUSED(env);
    return ENOSYS;
}
