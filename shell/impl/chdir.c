#include <unistd.h>
#include <sys/errno.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_chdir(struct UCSH_env *env)
{
    if (env->argc != 2)
        return EINVAL;
    else
        return chdir(env->argv[1]);
}
