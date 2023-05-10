#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_rmdir(struct UCSH_env *env)
{
    if (env->argc != 2)
        return EINVAL;
    else
        return unlinkat(-1, env->argv[1], AT_FDCWD | AT_REMOVEDIR);
}
