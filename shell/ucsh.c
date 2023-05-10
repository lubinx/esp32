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
#include <pthread.h>
#include <sys/glist.h>
#include <sys/errno.h>

#include <stdlib.h>
#include <string.h>

#include "sh/cmdline.h"
#include "sh/ucsh.h"

/***************************************************************************/
/** @def
****************************************************************************/
struct UCSH_reg
{
    char const *cmd;
    UCSH_callback_t func;
};

struct UCSH_dyn_list
{
    pthread_mutex_t lock;
    glist_t list;
};

struct UCSH_dynamic_reg
{
    struct UCSH_dynamic_reg *glist_next;
    struct UCSH_reg reg;
};

/***************************************************************************/
/** @private
****************************************************************************/
static struct UCSH_reg const *UCSH_find_cmd(char const *cmd);
static void *UCSH_thread(struct UCSH_env *env);

// const
static struct UCSH_reg const statical_list[] =
{
    {.cmd = "ver",      .func = UCSH_version},
    {.cmd = "rst",      .func = UCSH_reset},
    {.cmd = "dt",       .func = UCSH_datetime},
    {.cmd = "ls",       .func = UCSH_ls},
    {.cmd = "cat",      .func = UCSH_cat},
    {.cmd = "pwd",      .func = UCSH_pwd},
    {.cmd = "cd",       .func = UCSH_chdir},
    {.cmd = "chdir",    .func = UCSH_chdir},
    {.cmd = "mkdir",    .func = UCSH_mkdir},
    {.cmd = "rmdir",    .func = UCSH_rmdir},
    {.cmd = "rm",       .func = UCSH_unlink},
    {.cmd = "unlink",   .func = UCSH_unlink},
    {.cmd = "nvm",      .func = UCSH_nvm},
    {.cmd = "format",   .func = UCSH_format},
};

// var
static struct UCSH_dyn_list dynamic_list = {0};

/***************************************************************************/
/** @export
****************************************************************************/
int UCSH_init(void)
{
    glist_initialize(&dynamic_list.list);
    return pthread_mutex_init(&dynamic_list.lock, NULL);
}

int UCSH_register(char const *cmd, UCSH_callback_t func)
{
    struct UCSH_dynamic_reg *item = malloc(sizeof(struct UCSH_dynamic_reg));
    if (NULL == item)
        return ENOMEM;

    item->reg.cmd = cmd;
    item->reg.func = func;

    int retval = pthread_mutex_lock(&dynamic_list.lock);
    if (0 == retval)
    {
        glist_push_back(&dynamic_list.list, item);
        return pthread_mutex_unlock(&dynamic_list.lock);
    }
    else
        free(item);

    return retval;
}

void UCSH_init_instance(struct UCSH_env *env, int fd, uint32_t *stack, size_t stack_size)
{
    env->fd = fd;
    env->bufsize = (uint16_t)stack_size;
    env->buf = (char *)stack;

    pthread_attr_t attr;
    pthread_t id;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, stack, stack_size);

    pthread_create(&id, &attr, (pthread_routine_t)UCSH_thread, env);
    pthread_attr_destroy(&attr);
}

struct UCSH_env *UCSH_alloc_instance(int fd, size_t stack_size)
{
    struct UCSH_env *env =
        malloc(sizeof(struct UCSH_env) + stack_size);

    if (env)
        UCSH_init_instance(env, fd, (uint32_t *)(env + 1), stack_size);
    else
        (void)__set_errno_nullptr(ENOMEM);

    return env;
}

/***************************************************************************/
/** @weak linked
****************************************************************************/
__attribute__((weak))
uint8_t UCSH_leading_count(void)
{
    return 0;
}

__attribute__((weak))
bool UCSH_leading_filter(char *leading, size_t count)
{
    ARG_UNUSED(leading, count);
    return true;
}

__attribute__((weak))
void UCSH_startup_handle(struct UCSH_env *env)
{
    UCSH_puts(env, "\r\nUltraCore shell...\r\n");
}

__attribute__((weak))
void UCSH_prompt_handle(struct UCSH_env *env)
{
    UCSH_puts(env, "$ ");
}

__attribute__((weak))
void UCSH_error_handle(struct UCSH_env *env, int err)
{
    if (0 != err)
    {
        char const *msg;

        switch (err)
        {
        case ECMD:
            msg = "Command not understood";
            break;

        default:
            msg = strerror(err);
            break;
        }

        UCSH_printf(env, "%d: %s\r\n", err, msg);
    }
}

__attribute__((weak))
int UCSH_version(struct UCSH_env *env)
{
    UCSH_puts(env, "UCSH_version(): 0.0.0\r\n");
    return 0;
}

/***************************************************************************/
/** @internal
****************************************************************************/
static __attribute__((noreturn)) void *UCSH_thread(struct UCSH_env *env)
{
    char *inst_buf = env->buf;
    uint16_t inst_bufsize = env->bufsize / 2; /// @keep least half to the stack
    uint16_t inst_buf_wpos;

    UCSH_startup_handle(env);

    while (true)
    {
UCSH_infinite_loop:
        inst_buf_wpos = 0;

        env->argv[0] = NULL;
        env->argc = 0;

        // msleep(10);
        UCSH_prompt_handle(env);

UCSH_continue_read:
        if (-1 == read(env->fd, &inst_buf[inst_buf_wpos], 1))
        {
            // msleep(10);
            goto UCSH_continue_read;
        }

        inst_buf_wpos ++;
        inst_buf[inst_buf_wpos] = '\0';

        if (! UCSH_leading_filter(inst_buf, inst_buf_wpos))
            continue;
        if (inst_buf_wpos < UCSH_leading_count())
            goto UCSH_continue_read;
        else
            inst_buf_wpos -= UCSH_leading_count();

        while (true)
        {
            char CH;
            if (-1 == read(env->fd, &CH, sizeof(CH)))
                goto UCSH_infinite_loop;

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
                UCSH_error_handle(env, E2BIG);
                goto UCSH_infinite_loop;
            }
        }

        inst_buf[inst_buf_wpos] = '\0';
        env->argc = (int16_t)CMD_parse(inst_buf, lengthof(env->argv), env->argv);

        /// @setup for env buf, align to 4
        inst_buf_wpos = (uint16_t)((inst_buf_wpos + 4) & ~0x03);
        env->buf = &inst_buf[inst_buf_wpos];
        env->bufsize = inst_bufsize - inst_buf_wpos;

        if (env->argc < 0)
        {
            UCSH_error_handle(env, errno);
            continue;
        }
        if (env->argc == 0)
            continue;

        struct UCSH_reg const *reg = UCSH_find_cmd(env->argv[0]);
        if (reg)
        {
            int err = ((UCSH_callback_t)(reg->func))(env);
            if (-1 == err)
                err = errno;

            UCSH_error_handle(env, err);
        }
        else
            UCSH_error_handle(env, ECMD);
    }
}

static struct UCSH_reg const *UCSH_find_cmd(char const *cmd)
{
    struct UCSH_reg *reg = NULL;

    if (0 == pthread_mutex_lock(&dynamic_list.lock))
    {
        for (struct UCSH_dynamic_reg **iter = glist_iter_begin(&dynamic_list.list);
            iter != glist_iter_end(&dynamic_list.list);
            iter = glist_iter_next(&dynamic_list.list, iter))
        {
            if (0 == strcmp(cmd, (*iter)->reg.cmd))
            {
                reg = &(*iter)->reg;
                break;
            }
        }
        pthread_mutex_unlock(&dynamic_list.lock);
    }

    if (NULL == reg)
    {
        for (unsigned i = 0; i < lengthof(statical_list); i ++)
        {
            if (0 == strcmp(cmd, statical_list[i].cmd))
                return &statical_list[i];
        }
    }

    return reg;
}
