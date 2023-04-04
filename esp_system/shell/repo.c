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
#include <string.h>

#include <ultracore/kernel.h>
#include "sh/repo.h"

struct SHELL_repo
{
    pthread_mutex_t lock;

    struct SHELL_REPO_reg const *static_tbl;
    ssize_t static_tbl_count;
    glist_t list;
};

struct SHELL_repo_item
{
    struct SHELL_repo_item *glist_next;
    struct SHELL_REPO_reg reg;
};

int SHELL_REPO_init(void *repo, struct SHELL_REPO_reg const *static_reg_tbl, size_t reg_tbl_count)
{
    ASSERT(sizeof(SHELL_REPO_t) == sizeof(struct SHELL_repo));
    struct SHELL_repo *repository = repo;

    repository->static_tbl = static_reg_tbl;
    repository->static_tbl_count = (ssize_t)reg_tbl_count;

    glist_initialize(&repository->list);
    return pthread_mutex_init(&repository->lock, NULL);
}

int SHELL_REPO_destroy(void *repo)
{
    struct SHELL_repo *repository = repo;

    int retval = pthread_mutex_lock(&repository->lock);
    if (0 == retval)
    {
        struct SHELL_repo_item **iter = glist_iter_begin(&repository->list);
        while (iter != glist_iter_end(&repository->list))
        {
            struct SHELL_repo_item *tmp = *iter;
            glist_iter_extract(&repository->list, iter);

            free(tmp);
        }
        pthread_mutex_unlock(&repository->lock);
    }
    return retval;
}

int SHELL_REPO_lock(void *repo)
{
    return pthread_mutex_lock(&((struct SHELL_repo *)repo)->lock);
}

int SHELL_REPO_unlock(void *repo)
{
    return pthread_mutex_unlock(&((struct SHELL_repo *)repo)->lock);
}

int SHELL_REPO_register(void *repo, char const *cmd, void *callback)
{
    struct SHELL_repo_item *item = malloc(sizeof(struct SHELL_repo_item));
    if (NULL == item)
        return ENOMEM;

    item->reg.cmd = cmd;
    item->reg.callback = (void *)callback;

    struct SHELL_repo *repository = repo;

    int retval = pthread_mutex_lock(&repository->lock);
    if (0 == retval)
    {
        glist_push_back(&repository->list, item);
        return pthread_mutex_unlock(&repository->lock);
    }
    else
        free(item);

    return retval;
}

struct SHELL_REPO_reg const *SHELL_REPO_get(void *repo, char const *cmd)
{
    struct SHELL_repo *repository = repo;
    struct SHELL_REPO_reg *reg = NULL;

    if (0 == pthread_mutex_lock(&repository->lock))
    {
        for (struct SHELL_repo_item **iter = glist_iter_begin(&repository->list);
            iter != glist_iter_end(&repository->list);
            iter = glist_iter_next(&repository->list, iter))
        {
            if (0 == strcmp(cmd, (*iter)->reg.cmd))
            {
                reg = &(*iter)->reg;
                break;
            }
        }
        pthread_mutex_unlock(&repository->lock);
    }

    if (NULL == reg)
    {
        for (int i = 0; i < repository->static_tbl_count; i ++)
        {
            if (0 == strcmp(cmd, repository->static_tbl[i].cmd))
                return &repository->static_tbl[i];
        }
    }

    return reg;
}

struct SHELL_REPO_reg const *SHELL_REPO_iterator_begin(void *repo)
{
    struct SHELL_repo *repository = repo;

    if (0 == repository->static_tbl_count)
    {
        if (repository->list.entry)
            return &((struct SHELL_repo_item *)repository->list.entry)->reg;
        else
            return NULL;
    }
    else
        return &repository->static_tbl[0];
}

struct SHELL_REPO_reg const *SHELL_REPO_iterator_end(void *repo)
{
    ARG_UNUSED(repo);
    return NULL;
}

struct SHELL_REPO_reg const *SHELL_REPO_iterator_next(void *repo, struct SHELL_REPO_reg const *iter)
{
    struct SHELL_repo *repository = repo;

    int diff = iter - repository->static_tbl;
    if (diff == repository->static_tbl_count - 1)
    {
        if (repository->list.entry)
            return &((struct SHELL_repo_item *)repository->list.entry)->reg;
        else
            return NULL;
    }
    else if (diff < repository->static_tbl_count)
        return ++ iter;

    struct SHELL_repo_item *gitem = (struct SHELL_repo_item *)(uintptr_t)(uintptr_t)((uint8_t const *)iter - sizeof(void *));
    if (gitem->glist_next)
        return &gitem->glist_next->reg;
    else
        return NULL;
}
