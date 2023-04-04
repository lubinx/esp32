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
#ifndef __SH_REPOSITORY_H
#define __SH_REPOSITORY_H               1

#include <features.h>
#include <pthread.h>

#include <stdint.h>
#include <ultracore/glist.h>

__BEGIN_DECLS

    struct __SHELL_repo
    {
        uintptr_t pad[10];
    };
    typedef struct __SHELL_repo     SHELL_REPO_t;

    struct SHELL_REPO_reg
    {
        char const *cmd;
        void *callback;
    };
    typedef struct SHELL_REPO_reg   SHELL_REPO_reg_t;

    /**
     *  SHELL_REPO_init()
     *      init the repository. inside the repository, it contains: a dynamic SHELL_REPO_reg_s
     *  list mutex lock; a static SHELL_REPO_reg_s array setup by this function
     *
     *  @PARAMS
     *      repo: was a struct lead with SHELL_REPO_header_s
     *      table: static initialized register item
     *
     *  @RETURN values see pthread_mutex_init();
     */
extern __attribute__((nonnull(1), nothrow))
    int SHELL_REPO_init(void *repo, struct SHELL_REPO_reg const *static_tbl, size_t tbl_count);

    /**
     *  SHELL_REPO_destroy()
     */
extern __attribute__((nonnull, nothrow))
    int SHELL_REPO_destroy(void *repo);

    /**
     *  SHELL_REPO_lock(): lock the repository
     *  @RETURN values see pthread_mutex_lock();
     */
extern __attribute__((nonnull, nothrow))
    int SHELL_REPO_lock(void *repo);

    /**
     *  SHELL_REPO_unlock(): unlock the repository
     *  @RETURN values see pthread_mutex_unlock();
     */
extern __attribute__((nonnull, nothrow))
    int SHELL_REPO_unlock(void *repo);

    /**
     *  SHELL_REPO_register()
     *  @RETURN values
     *      see pthread_mutex_lock() / pthread_mutex_unlock();
     *  @ERRORS
     *      ENOMEM
     */
extern __attribute__((nonnull, nothrow))
    int SHELL_REPO_register(void *rep, char const *cmd, void *callback);

    /**
     *  SHELL_REPO_get()
     */
extern __attribute__((nonnull, pure, nothrow))
    struct SHELL_REPO_reg const *SHELL_REPO_get(void *repo, char const *cmd);

    /**
     *  SHELL_REPO iterator
     */
extern __attribute__((nonnull, pure, nothrow))
    struct SHELL_REPO_reg const *SHELL_REPO_iterator_begin(void *repo);
extern __attribute__((nonnull, pure, nothrow))
    struct SHELL_REPO_reg const *SHELL_REPO_iterator_end(void *repo);
extern __attribute__((nonnull, nothrow))
    struct SHELL_REPO_reg const *SHELL_REPO_iterator_next(void *repo, struct SHELL_REPO_reg const *iter);

__END_DECLS
#endif
