#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/limits.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_pwd(struct UCSH_env *env)
{
    char *path = getcwd(env->buf, PATH_MAX);

    writeln(env->fd, path, strlen(path));
    return 0;
}
