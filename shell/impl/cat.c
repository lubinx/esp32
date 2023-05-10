#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_cat(struct UCSH_env *env)
{
    char *filename = NULL;
    int flags = 0;
    size_t size = SIZE_MAX;

    for (int i = 1; i < env->argc; i ++)
    {
        char *param = env->argv[i];

        if (param[0] == '>')
        {
            int idx = 1;
            flags = O_RDWR | O_CREAT | O_TRUNC;

            if (param[idx] == '>')
            {
                idx ++;
                flags = (flags & ~O_TRUNC) | O_APPEND;
            }
            if (param[idx])
                filename = &param[idx];
        }
        else if (CMD_param_isoptional(param))
        {
            if (0 == strncmp(param, "s", 1))
                size = (size_t)atoi(CMD_paramvalue(param));
        }
        else
            filename = param;
    }
    if (! filename)
        return EINVAL;

    if (filename[0] == '>')
    {
        flags = O_RDWR | O_CREAT | O_TRUNC;
        filename ++;

        if (filename[0] == '>')
        {
            flags = (flags & ~O_TRUNC) | O_APPEND;
            filename ++;
        }
    }

    int fd = open(filename, flags);
    if (-1 == fd)
        return errno;

    int output_fd;
    if (O_CREAT & flags)
    {
        output_fd = fd;
        fd = env->fd;
    }
    else
        output_fd = env->fd;

    int retval = 0;
    char break_cond = '\0';

    while (size > 0)
    {
        retval = read(fd, env->buf, size > 64 ? 64 : size);

        /// break condition: @shell input (ctrl + d and ctrl + d)
        if (fd == env->fd && 1 == retval && 0x4 == env->buf[0])
        {
            if (break_cond == env->buf[0])
            {
                retval = 0;
                break;
            }
            else
            {
                break_cond = env->buf[0];
                continue;
            }
        }
        else if (break_cond && sizeof(break_cond) != write(output_fd, env->buf, sizeof(break_cond)))
        {
            retval = -1;
            break;
        }
        break_cond = '\0';

        if (retval > 0)
        {
            if (retval != writebuf(output_fd, env->buf, (size_t)retval))
            {
                retval = -1;
                break;
            }

            size -= (size_t)retval;
        }
        else
            break;
    }

    if (fd != env->fd)
        close(fd);
    else
        close(output_fd);

    writeln(env->fd, NULL, 0);
    return retval;
}
