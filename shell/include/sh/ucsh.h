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

#include "cmdline.h"

__BEGIN_DECLS
    struct UCSH_env
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
#define UCSH_printf(env, fmt, ...)      \
    write((env)->fd, (env)->buf, (unsigned)sprintf((env)->buf, (fmt), __VA_ARGS__))

    /**
     *  UCH_puts()
     *      wrapper to sprintf() => write(env->fd, ...)
     */
#define UCSH_puts(env, text)            \
    write(env->fd, env->buf, (unsigned)sprintf(env->buf, text))

    /**
     *  UCSH_init(): initialize UltraCore'shell context
     *  @RETURN see SHELL_REPO_init();
     */
extern __attribute__((nothrow))
    int UCSH_init(void);

    /**
     *  UCSH_callback_t:
     *      UCSH callback function declaration
     *          void FOO(UCSH_callback_t *env);
     *      fd and param was setup by UCSH_init_instance();
     *      @argc @argv like main()
     */
    typedef int (* UCSH_callback_t)(struct UCSH_env *env);

    /**
     *  UCSH_register(): register a command callback
     *  @RETURN see SHELL_REPO_register();
     */
extern __attribute__((nonnull, nothrow))
    int UCSH_register(char const *cmd, const UCSH_callback_t callback);

    /**
     *  UCSH_init_instance(): initialize a UCSH thread
     *      buffer sharing with stack from top->bottom
     */
extern __attribute__((nothrow))
    void UCSH_init_instance(struct UCSH_env *env, int fd, uint32_t *stack, size_t stack_size);

    /**
     *  UCSH_alloc_instance(): initialize a UCSH thread with allocated stack
     */
extern __attribute__((nothrow))
    struct UCSH_env *UCSH_alloc_instance(int fd, size_t stack_size);

    /**
     *  UCSH_leading_count() / UCSH_leading_filter()
     *      **weak** linked UCSH leading character filter
     *      default: leading count = 0
     */
extern __attribute__((nothrow))
    uint8_t UCSH_leading_count(void);
extern __attribute__((nothrow))
    bool UCSH_leading_filter(char *buf, size_t count);

    /**
     *  UCSH_startup_handle()
     *      **weak** linked when UCSH startup
     */
extern __attribute__((nonnull, nothrow))
    void UCSH_startup_handle(struct UCSH_env *env);

    /**
     *  UCSH_startup_handle()
     *      **weak** linked when UCSH print prompt
     */
extern __attribute__((nonnull, nothrow))
    void UCSH_prompt_handle(struct UCSH_env *env);

    /**
     *  UCSH_error_handle()
     *      **weak** linked error handle function
     *  @err
     *      0       indicated no error
     *      [ECMD]  indicated the command is not understood
     *      [E2BIG] indicated the parameter is too long to handle
     *      @other call to POSIX functions
     */
extern __attribute__((nonnull, nothrow))
    void UCSH_error_handle(struct UCSH_env *env, int err);

/****************************************************************************
 *  preset @commands
 ***************************************************************************/
extern __attribute__((nonnull, nothrow))
    int UCSH_reset(struct UCSH_env *env);


extern __attribute__((nonnull, nothrow))
    int UCSH_version(struct UCSH_env *env);

extern __attribute__((nonnull, nothrow))
    int UCSH_datetime(struct UCSH_env *env);

extern __attribute__((nonnull, nothrow))
    int UCSH_nvm(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_ls(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_cat(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_pwd(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_unlink(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_mkdir(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_chdir(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_rmdir(struct UCSH_env *env);
extern __attribute__((nonnull, nothrow))
    int UCSH_format(struct UCSH_env *env);


__END_DECLS
#endif
