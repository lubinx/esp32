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

#ifndef UCSH_LINE_BUFFER
    #define UCSH_LINE_BUFFER            (512U * sizeof(char))
#endif

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
static void *UCSH_thread(struct UCSH_env *env) __attribute__((noreturn));

// const
static struct UCSH_reg const statical_list[] =
{
    {.cmd = "help",     .func = UCSH_help},
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

void UCSH_loop(struct UCSH_env *env, int fd, uint16_t const line_bufsize)
{
    char buffer[line_bufsize / sizeof(char)];

    env->fd = fd;
    env->buf = buffer;
    env->bufsize = line_bufsize;

    UCSH_thread(env);
}

int UCSH_init_instance(struct UCSH_env *env, int fd, size_t stack_size, uint32_t *stack)
{
    if (! stack)
        stack = malloc(stack_size);
    if (! stack)
        return ENOMEM;

    env->fd = fd;
    env->buf = (char *)stack;
    {
        uint16_t half_stacksize = (uint16_t)((stack_size >> 1) & ~0x7U);
        env->bufsize = UCSH_LINE_BUFFER < half_stacksize ? UCSH_LINE_BUFFER : (uint16_t)half_stacksize;
    }

    stack = (uint32_t *)((uint8_t *)stack + env->bufsize);
    stack_size -= env->bufsize;

    pthread_attr_t attr;
    pthread_t id;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, stack, stack_size);

    pthread_create(&id, &attr, (pthread_routine_t)UCSH_thread, env);
    return pthread_attr_destroy(&attr);
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
        UCSH_printf(env, "%d: %s\r\n", err, strerror(err));
}

__attribute__((weak))
int UCSH_version(struct UCSH_env *env)
{
    UCSH_puts(env, "UCSH_version(): 0.0.0\r\n");
    return 0;
}

__attribute__((weak))
int UCSH_help(struct UCSH_env *env)
{
    UCSH_puts(env, "Supported commands:\r\n");

    for (unsigned i = 0; i < lengthof(statical_list); i ++)
    {
        struct UCSH_reg const *reg = &statical_list[i];
        UCSH_puts(env, "\t");
        UCSH_puts(env, reg->cmd);
        UCSH_puts(env, "\r\n");
    }

        if (0 == pthread_mutex_lock(&dynamic_list.lock))
    {
        for (struct UCSH_dynamic_reg **iter = glist_iter_begin(&dynamic_list.list);
            iter != glist_iter_end(&dynamic_list.list);
            iter = glist_iter_next(&dynamic_list.list, iter))
        {
            UCSH_puts(env, "\t");
            UCSH_puts(env, (*iter)->reg.cmd);
            UCSH_puts(env, "\r\n");
        }
        pthread_mutex_unlock(&dynamic_list.lock);
    }
    return 0;
}

/***************************************************************************/
/** @internal
****************************************************************************/
static void *UCSH_thread(struct UCSH_env *env)
{
    UCSH_startup_handle(env);

    uint16_t const bufsize = env->bufsize;
    char *cmdline = env->buf;
    uint16_t comdline_wpos;

    while (true)
    {
UCSH_infinite_loop:
        comdline_wpos = 0;
        env->bufsize = bufsize;
        env->argc = 0;

        UCSH_prompt_handle(env);

UCSH_continue_read:
        if (sizeof(char) != read(env->fd, &cmdline[comdline_wpos], 1))
            goto UCSH_continue_read;

        comdline_wpos ++;
        cmdline[comdline_wpos] = '\0';

        if (! UCSH_leading_filter(cmdline, comdline_wpos))
            continue;
        if (comdline_wpos < UCSH_leading_count())
            goto UCSH_continue_read;
        else
            comdline_wpos -= UCSH_leading_count();

        while (true)
        {
            char ch;
            if (sizeof(ch) != read(env->fd, &ch, sizeof(ch)))
                goto UCSH_infinite_loop;

            if (comdline_wpos > 0)
            {
                if (ch == 0x08)     // backspace
                {
                    comdline_wpos -= (uint16_t)sizeof(char);
                    continue;
                }
                else if (' ' == ch && ' ' == cmdline[comdline_wpos - 1])
                    continue;
            }
            if (ch == '\n')
            {
                if (comdline_wpos > 0 && cmdline[comdline_wpos -1] == '\r')
                    comdline_wpos -= (uint16_t)sizeof(char);

                cmdline[comdline_wpos] = '\0';
                break;
            }

            cmdline[comdline_wpos] = ch;
            comdline_wpos += (uint16_t)sizeof(char);;

            if (comdline_wpos > bufsize)
            {
                UCSH_error_handle(env, E2BIG);
                goto UCSH_infinite_loop;
            }
        }

        cmdline[comdline_wpos] = '\0';
        env->argc = (int16_t)CMD_parse(cmdline, lengthof(env->argv), env->argv);

        /// @setup for env buf, align to 8
        comdline_wpos = (uint16_t)((comdline_wpos + 7) & ~0x07);
        env->buf = &cmdline[comdline_wpos];
        env->bufsize = bufsize - comdline_wpos;

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
