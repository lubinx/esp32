/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#include <ultracore/kernel.h>
#include <ultracore/nvm.h>
#include <sys/limits.h>
#include <sys/stime.h>

#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#include "sh/repo.h"
#include "sh/cmdline.h"
#include "sh/ucsh.h"

/***************************************************************************/
/** @internal
****************************************************************************/
static SHELL_REPO_t SHELL_repo = {0};

/// @functions
static void *SHELL_thread(struct SHELL_env *env);

/***************************************************************************/
/** @const
****************************************************************************/
static struct SHELL_REPO_reg const ucsh_static[] =
{
    {.cmd = "help",     .callback = (void *)SHELL_help},
    {.cmd = "ver",      .callback = (void *)SHELL_version},
    {.cmd = "rst",      .callback = (void *)SHELL_reset},
    {.cmd = "dt",       .callback = (void *)SHELL_datetime},
    {.cmd = "nvm",      .callback = (void *)SHELL_nvm},
    {.cmd = "ls",       .callback = (void *)SHELL_ls},
    {.cmd = "cat",      .callback = (void *)SHELL_cat},
    {.cmd = "pwd",      .callback = (void *)SHELL_pwd},
    {.cmd = "cd",       .callback = (void *)SHELL_chdir},
    {.cmd = "chdir",    .callback = (void *)SHELL_chdir},
    {.cmd = "mkdir",    .callback = (void *)SHELL_mkdir},
    {.cmd = "rmdir",    .callback = (void *)SHELL_rmdir},
    {.cmd = "rm",       .callback = (void *)SHELL_unlink},
    {.cmd = "unlink",   .callback = (void *)SHELL_unlink},
    {.cmd = "format",   .callback = (void *)SHELL_format},
};

/***************************************************************************/
/** @export
****************************************************************************/
int SHELL_init(void)
{
    return SHELL_REPO_init(&SHELL_repo, ucsh_static, lengthof(ucsh_static));
}

int SHELL_register(char const *cmd, SHELL_callback_t callback)
{
    return SHELL_REPO_register(&SHELL_repo, cmd, (void *)callback);
}

void SHELL_init_instance(struct SHELL_env *env, int fd, uint32_t *stack, size_t stack_size)
{
    env->fd = fd;
    env->bufsize = (uint16_t)stack_size;
    env->buf = (char *)stack;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    thread_attr_setstack(&attr, stack, stack_size);

    pthread_create(&attr, (pthread_routine_t)SHELL_thread, env);
    pthread_attr_destroy(&attr);
}

struct SHELL_env *SHELL_alloc_instance(int fd, size_t stack_size)
{
    struct SHELL_env *env =
        KERNEL_malloc(sizeof(struct SHELL_env) + stack_size);

    if (env)
        SHELL_init_instance(env, fd, (uint32_t *)(env + 1), stack_size);
    else
        __set_errno_nullptr(ENOMEM);

    return env;
}

/***************************************************************************/
/** @weak linked
****************************************************************************/
__attribute__((weak))
uint8_t SHELL_leading_count(void)
{
    return 0;
}

__attribute__((weak))
bool SHELL_leading_filter(char *leading, size_t count)
{
    ARG_UNUSED(leading, count);
    return true;
}

__attribute__((weak))
void SHELL_startup_handle(struct SHELL_env *env)
{
    SHELL_puts(env, "\r\nUltraCore rtos-shell...\r\n");
}

__attribute__((weak))
void SHELL_prompt_handle(struct SHELL_env *env)
{
    SHELL_puts(env, "$ ");
}

__attribute__((weak))
void SHELL_error_handle(struct SHELL_env *env, int err)
{
    if (0 != err)
    {
        char const *msg;

        switch (err)
        {
        case E_CMD_NOT_UNDERSTOOD:
            msg = "Command not understood";
            break;

        default:
            msg = strerror(err);
            break;
        }

        SHELL_printf(env, "%d: %s\r\n", err, msg);
    }
}

__attribute__((weak))
int SHELL_help(struct SHELL_env *env)
{
    SHELL_puts(env, "Supported command:\r\n");

    for (struct SHELL_REPO_reg const *iter = SHELL_REPO_iterator_begin(&SHELL_repo);
        iter != SHELL_REPO_iterator_end(&SHELL_repo);
        iter = SHELL_REPO_iterator_next(&SHELL_repo, iter))
    {
        SHELL_printf(env, "\t%s\r\n", iter->cmd);
    }
    return 0;
}

__attribute__((weak))
int SHELL_version(struct SHELL_env *env)
{
    SHELL_puts(env, "SHELL_version(): 0.0.0\r\n");
    return 0;
}

__attribute__((weak))
int SHELL_reset(struct SHELL_env *env)
{
    ARG_UNUSED(env);
    SYSTEM_reset();
}

__attribute__((weak))
int SHELL_datetime(struct SHELL_env *env)
{
    time_t val;

    if (env->argc == 1)
    {
        val = time(NULL);
        struct tm tv;
        localtime_r(&val, &tv);

        SHELL_printf(env, "%d/%d/%d %02d:%02d:%02d\r\n",
            tv.tm_year + 1900, tv.tm_mon + 1, tv.tm_mday, tv.tm_hour, tv.tm_min, tv.tm_sec);
    }
    else if (env->argc == 2)
    {
        val = (time_t)atoi(CMD_paramvalue_byname("epoch", env->argc, env->argv));
        if (val == 0)
            goto dt_show_usage;
        stime(val);
    }
    else if (env->argc == 3)
    {
        struct tm tv;
        /// datetime with @format year/mon/day hh:nn:ss
        char *str = env->argv[1];
        tv.tm_year = strtol(str, &str, 10);

        str ++;
        tv.tm_mon  = strtol(str, &str, 10) - 1;

        str ++;
        tv.tm_mday = strtol(str, NULL, 10);

        str = env->argv[2];
        tv.tm_hour = strtol(str, &str, 10);
        str ++;
        tv.tm_min  = strtol(str, &str, 10);
        str ++;
        tv.tm_sec  = strtol(str, NULL, 10);

        if (// tv.tm_year < 2000 || tv.tm_year > 2050 ||
            tv.tm_mon < 0 || tv.tm_mon > 11 ||
            tv.tm_mday < 0 || tv.tm_mday > 31 ||
            tv.tm_hour < 0 || tv.tm_hour > 23 ||
            tv.tm_min < 0 || tv.tm_min > 59 ||
            tv.tm_sec < 0 || tv.tm_sec > 59)
        {
            goto dt_show_usage;
        }

        tv.tm_year -= 1900;
        val = mktime((struct tm *)&tv);
        if (val == (time_t)(-1))
            goto dt_show_usage;
        else
            stime(val);
    }
    else
    {
dt_show_usage:
        SHELL_puts(env, "Usage: \r\n\tdt year/mon/day hour:min:sec\r\n\tdt epoch=val\r\n");
    }

    return 0;
}

__attribute__((weak))
int SHELL_cat(struct SHELL_env *env)
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

static bool NVM_enum_callback(struct NVM_attr const *attr, void const *data, void *arg)
{
    struct SHELL_env *env = (struct SHELL_env *)arg;

    if (NVM_TAG_KEY_ID != attr->key)
    {
        char *ch = (char *)&attr->key;
        SHELL_printf(env, "key: 0x%08X(%c%c%c%c)  size: %d\n", (unsigned)attr->key, ch[0], ch[1], ch[2], ch[3], attr->data_size);
    }
    else
        SHELL_printf(env, "tag name \"%s\"\n", (char *)data);

    return true;
}

__attribute__((weak))
int SHELL_nvm(struct SHELL_env *env)
{
    NVM_enum(NVM_enum_callback, env);
    writeln(env->fd, "", 0);

    return 0;
}

__attribute__((weak))
int SHELL_ls(struct SHELL_env *env)
{
    static char const *_xlat_rwx[] =
    {
        "---",
        "--x",
        "-w-",
        "-wx",
        "r--",
        "r-x",
        "rw-",
        "rwx",
    };
    char const *pathname = ".";

    for (int i = 1; i < env->argc; i ++)
    {
        if (! CMD_param_isoptional(env->argv[i]))
        {
            pathname = env->argv[i];
            break;
        }
    }

    DIR *dir = opendir(pathname);

    if (dir)
    {
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL)
        {
            /// https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxa500/lscmd.htm
            char fmt;

            if (S_ISBLK(ent->d_mode))
                fmt = 'b';
            else if (S_ISCHR(ent->d_mode))
                fmt = 'c';
            else if (S_ISDIR(ent->d_mode))
                fmt = 'd';
            else if (S_ISFIFO(ent->d_mode))
                fmt = 'p';
            else if (S_ISREG(ent->d_mode))
                fmt = '-';
            else if (S_ISLNK(ent->d_mode))
                fmt = 'l';
            else if (S_ISSOCK(ent->d_mode))
                fmt = 's';
            else
                fmt = '-';

            struct tm tv;
            localtime_r(&ent->d_modificaion_ts, &tv);

           SHELL_printf(env, "%c%s%s%s %d %s %s %8u %d/%02d/%02d %02d:%02d  %s\n",
                fmt,
                _xlat_rwx[(ent->d_mode >> 6) & 0x07],
                _xlat_rwx[(ent->d_mode >> 3) & 0x07],
                _xlat_rwx[ent->d_mode & 0x07],
                1, "root", "root",
                ent->d_size,
                1900 + tv.tm_year, tv.tm_mon + 1, tv.tm_mday, tv.tm_hour, tv.tm_min,
                ent->d_name
            );
        }

        return closedir(dir);
    }
    else
        return errno;
}

__attribute__((weak))
int SHELL_pwd(struct SHELL_env *env)
{
    char *path = getcwd(env->buf, PATH_MAX);

    writeln(env->fd, path, strlen(path));
    return 0;
}

__attribute__((weak))
int SHELL_chdir(struct SHELL_env *env)
{
    if (env->argc != 2)
        return EINVAL;
    else
        return chdir(env->argv[1]);
}

__attribute__((weak))
int SHELL_mkdir(struct SHELL_env *env)
{
    ARG_UNUSED(env);
    return ENOSYS;
}

__attribute__((weak))
int SHELL_rmdir(struct SHELL_env *env)
{
    if (env->argc != 2)
        return EINVAL;
    else
        return unlinkat(-1, env->argv[1], AT_FDCWD | AT_REMOVEDIR);
}

__attribute__((weak))
int SHELL_unlink(struct SHELL_env *env)
{
    if (env->argc != 2)
        return EINVAL;
    else
        return unlink(env->argv[1]);
}

__attribute__((weak))
int SHELL_format(struct SHELL_env *env)
{
    if (env->argc != 3)
        return EINVAL;

    return FILESYSTEM_format(env->argv[1], env->argv[2]);
}

/***************************************************************************/
/** @internal
****************************************************************************/
__attribute__((noreturn))
static void *SHELL_thread(struct SHELL_env *env)
{
    char *inst_buf = env->buf;
    uint16_t inst_bufsize = env->bufsize / 2; /// @keep least half to the stack
    uint16_t inst_buf_wpos;

    SHELL_startup_handle(env);

    while (true)
    {
SHELL_infinite_loop:
        inst_buf_wpos = 0;

        env->argv[0] = NULL;
        env->argc = 0;

        // msleep(10);
        SHELL_prompt_handle(env);

SHELL_continue_read:
        if (-1 == read(env->fd, &inst_buf[inst_buf_wpos], 1))
        {
            // msleep(10);
            goto SHELL_continue_read;
        }

        inst_buf_wpos ++;
        inst_buf[inst_buf_wpos] = '\0';

        if (! SHELL_leading_filter(inst_buf, inst_buf_wpos))
            continue;
        if (inst_buf_wpos < SHELL_leading_count())
            goto SHELL_continue_read;
        else
            inst_buf_wpos -= SHELL_leading_count();

        while (true)
        {
            char CH;
            if (-1 == read(env->fd, &CH, sizeof(CH)))
                goto SHELL_infinite_loop;

            if (inst_buf_wpos > 0)
            {
                if (CH == 0x08)     // backspace
                {
                    inst_buf_wpos --;
                    continue;
                }
                else if (' ' == CH && ' ' == inst_buf[inst_buf_wpos - 1])
                    continue;
            }
            if (CH == '\n')
            {
                if (inst_buf_wpos > 0 && inst_buf[inst_buf_wpos -1] == '\r')
                    inst_buf_wpos --;

                inst_buf[inst_buf_wpos] = '\0';
                break;
            }

            inst_buf[inst_buf_wpos] = CH;
            inst_buf_wpos ++;

            if (inst_buf_wpos > inst_bufsize / 2)
            {
                SHELL_error_handle(env, E2BIG);
                goto SHELL_infinite_loop;
            }
        }

        inst_buf[inst_buf_wpos] = '\0';
        env->argc = (int16_t)CMD_parse(inst_buf, env->argv, lengthof(env->argv));

        /// @setup for env buf, align to 4
        inst_buf_wpos = (uint16_t)((inst_buf_wpos + 4) & ~0x03);
        env->buf = &inst_buf[inst_buf_wpos];
        env->bufsize = inst_bufsize - inst_buf_wpos;

        if (env->argc < 0)
        {
            SHELL_error_handle(env, errno);
            continue;
        }
        if (env->argc == 0)
            continue;

        struct SHELL_REPO_reg const *reg = SHELL_REPO_get(&SHELL_repo, env->argv[0]);
        if (reg)
        {
            int err = ((SHELL_callback_t)(reg->callback))(env);
            if (-1 == err)
                err = errno;

            SHELL_error_handle(env, err);
        }
        else
            SHELL_error_handle(env, E_CMD_NOT_UNDERSTOOD);
    }
}
