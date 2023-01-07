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
#ifndef __C_GENERIC_LIST_H
#define __C_GENERIC_LIST_H              1

#include <features.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/****************************************************************************
 *  @declaration
 ***************************************************************************/
    /**
     *  @glist
     *      glist is a @forward only generic list for language c,
     *      aim to provide a small memory for kernel purpuse usage:
     *
     *  .as @queue usage
     *      glist_push_back(...);
     *      ...
     *      glist_pop(...);
     *
     *  .as @stack usage
     *      glist_push_front(...);
     *      ...
     *      glist_pop(...);
     */
    struct glist_t
    {
#ifdef __cplusplus
        glist_t() :
            entry(nullptr), extry(&entry)
        {
        }
#endif
        struct glist_hdr_t *entry;
        struct glist_hdr_t **extry;
    };
    typedef struct glist_t          glist_t;

    /**
     *  glist @hdr
     *      glist can collect any structure that
     *          .contains struct glist_hdr_t
     *          .or contains sizeof(intptr_t) padding at first element
     */
    struct glist_hdr_t
    {
#ifdef __cplusplus
        glist_hdr_t():
            link_next(nullptr)
        {
        }
#endif
        struct glist_hdr_t *link_next;
    };

#ifdef __cplusplus
    /**
     *  C++ polymorphism
     *      inherit from glist_t / glist_hdr_t, this will allow GNU c++
     *  static_cast<> / dynamic_cast<> to work properly when multi-inherit
     */
    typedef struct glist_hdr_t *    glist_node_t;
    typedef struct glist_hdr_t **   glist_iter_t;
#else
    /**
     *  glist @item
     *      declare void * for any pointer type to accept the return value
     *
     *  a valid glist item must contain a padding element sizeof(intptr_t) at
     *      structure's first element, this element can be used to any other
     *      purpuse when its not in the list
     *
     *  examples:
     *      struct some_list_item_t
     *      {
     *          intptr_t perserved;     // padding for glist_hdr_t
     *          // ... @payload
     *      };
     */
    typedef void *                  glist_node_t;

    /**
     *  glist @iterator
     *      struct glist_hdr_t **
     *      declare void * for any pointer type to accept the return value
     */
    typedef void *                  glist_iter_t;
#endif

/****************************************************************************
 *  @initializer
 ***************************************************************************/
 /// C99 style initializer
#define GLIST_INITIALIZER(list)         {.entry = NULL, .extry = &list.entry}

__BEGIN_DECLS
/****************************************************************************
 *  generic list @basic
 ****************************************************************************/
static inline __attribute__((nonnull, nothrow))
    void glist_initialize(glist_t *list)
    {
        list->entry = NULL;
        list->extry = &list->entry;
    }

static inline __attribute__((nonnull, nothrow))
    bool glist_is_initialized(glist_t *list)
    {
        return NULL != list->extry;
    }

static inline __attribute__((nonnull, nothrow))
    bool glist_is_empty(glist_t *list)
    {
        return NULL == list->entry;
    }

static inline __attribute__((nonnull, nothrow))
    glist_node_t glist_peek(glist_t *list)
    {
        return list->entry;
    }

static inline __attribute__((nonnull, nothrow))
    glist_node_t glist_peek_last(glist_t *list)
    {
        return *list->extry;
    }

extern __attribute__((nonnull, nothrow))
    void glist_push_front(glist_t *list, glist_node_t node);

extern __attribute__((nonnull, nothrow))
    void glist_push_back(glist_t *list, glist_node_t node);

extern __attribute__((nonnull, nothrow))
    glist_node_t glist_pop(glist_t *list);

/****************************************************************************
 *  generic list @iterator
 ****************************************************************************/
static inline __attribute__((nonnull, nothrow))
    glist_iter_t glist_iter_begin(glist_t *list)
    {
        return &list->entry;
    }

static inline __attribute__((nonnull, nothrow))
    glist_iter_t glist_iter_end(glist_t *list)
    {
        return list->extry;
    }

extern __attribute__((nonnull, nothrow))
    glist_iter_t glist_iter_next(glist_t *list, glist_iter_t iter);

extern __attribute__((nonnull, nothrow))
    void glist_iter_insert(glist_t *list, glist_iter_t iter, glist_node_t node);

extern __attribute__((nonnull, nothrow))
    void glist_iter_insert_after(glist_t *list, glist_iter_t iter, glist_node_t node);

extern __attribute__((nonnull, nothrow))
    glist_node_t glist_iter_extract(glist_t *list, glist_iter_t iter);

/****************************************************************************
 *  generic list @algorithm
 *      todo: find / sort
 ****************************************************************************/
extern __attribute__((nonnull, nothrow))
    glist_iter_t glist_find(glist_t *list, glist_node_t node);

__END_DECLS
#endif
