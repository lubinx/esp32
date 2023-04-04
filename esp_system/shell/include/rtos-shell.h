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
#ifndef __ULTRACORE_SHELL_H
#define __ULTRACORE_SHELL_H             1

#include <features.h>
#include <unistd.h>
#include <stdarg.h>

#include "repo.h"
#include "cmdline.h"

    /**
     *  UCSH error handle
     */
    #define E_CMD_NOT_UNDERSTOOD        (1 + /* Command not understood */ + ULTRACORE_ERRNO_BASE)

__BEGIN_DECLS
    struct SHELL_env
    {
        int fd;             // shell input/output fd

        char *argv[12];
        void *arg;          // fill any value by user

        int16_t argc;
        uint16_t bufsize;
        char *buf;
    };

    /**
     *  UCH_printf()
     *      wrapper to sprintf() => write(env->fd, ...)
     */
#define SHELL_printf(env, fmt, ...)      \
    write((env)->fd, (env)->buf, (unsigned)sprintf((env)->buf, (fmt), __VA_ARGS__))

    /**
     *  UCH_puts()
     *      wrapper to sprintf() => write(env->fd, ...)
     */
#define SHELL_puts(env, text)            \
    write(env->fd, env->buf, (unsigned)sprintf(env->buf, text))

    /**
     *  SHELL_init(): initialize UltraCore'shell context
     *  @RETURN see SHELL_REPO_init();
     */
extern __attribute__((nothrow))
    int SHELL_init(void);

    /**
     *  SHELL_callback_t:
     *      UCSH callback function declaration
     *          void FOO(SHELL_callback_t *env);
     *      fd and param was setup by SHELL_init_instance();
     *      @argc @argv like main()
     */
    typedef int (* SHELL_callback_t)(struct SHELL_env *env);

    /**
     *  SHELL_register(): register a command callback
     *  @RETURN see SHELL_REPO_register();
     */
extern __attribute__((nonnull, nothrow))
    int SHELL_register(char const *cmd, const SHELL_callback_t callback);

    /**
     *  SHELL_init_instance(): initialize a UCSH thread
     *      buffer sharing with stack from top->bottom
     */
extern __attribute__((nothrow))
    void SHELL_init_instance(struct SHELL_env *env, int fd, uint32_t *stack, size_t stack_size);

    /**
     *  SHELL_alloc_instance(): initialize a UCSH thread with allocated stack
     */
extern __attribute__((nothrow))
    struct SHELL_env *SHELL_alloc_instance(int fd, size_t stack_size);

    /**
     *  SHELL_leading_count() / SHELL_leading_filter()
     *      **weak** linked UCSH leading character filter
     *      default: leading count = 0
     */
extern __attribute__((nothrow))
    uint8_t SHELL_leading_count(void);
extern __attribute__((nothrow))
    bool SHELL_leading_filter(char *buf, size_t count);

    /**
     *  SHELL_startup_handle()
     *      **weak** linked when UCSH startup
     */
extern __attribute__((nonnull, nothrow))
    void SHELL_startup_handle(struct SHELL_env *env);

    /**
     *  SHELL_startup_handle()
     *      **weak** linked when UCSH print prompt
     */
extern __attribute__((nonnull, nothrow))
    void SHELL_prompt_handle(struct SHELL_env *env);

    /**
     *  SHELL_error_handle()
     *      **weak** linked error handle function
     *  @err
     *      0       indicated no error
     *      [E_CMD_NOT_UNDERSTOOD]  indicated the command is not understood
     *      [E2BIG] indicated the parameter is too long to handle
     *      @other call to POSIX functions
     */
extern __attribute__((nonnull, nothrow))
    void SHELL_error_handle(struct SHELL_env *env, int err);

/****************************************************************************
 *  preset @commands
 ***************************************************************************/
extern __attribute__((nonnull, nothrow))
    int SHELL_reset(struct SHELL_env *env);

extern __attribute__((nonnull, nothrow))
    int SHELL_help(struct SHELL_env *env);

extern __attribute__((nonnull, nothrow))
    int SHELL_version(struct SHELL_env *env);

extern __attribute__((nonnull, nothrow))
    int SHELL_datetime(struct SHELL_env *env);

extern __attribute__((nonnull, nothrow))
    int SHELL_nvm(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_ls(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_cat(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_pwd(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_unlink(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_mkdir(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_chdir(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_rmdir(struct SHELL_env *env);
extern __attribute__((nonnull, nothrow))
    int SHELL_format(struct SHELL_env *env);


__END_DECLS
#endif
