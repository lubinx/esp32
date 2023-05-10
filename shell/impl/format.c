#include <sys/errno.h>
#include <rtos/filesystem.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_format(struct UCSH_env *env)
{
    if (env->argc != 3)
        return EINVAL;
    else
        return FILESYSTEM_format(env->argv[1], env->argv[2]);
}
