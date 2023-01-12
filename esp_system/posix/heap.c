/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <sys/reent.h>
#include <errno.h>
#include <malloc.h>
#include "esp_heap_caps.h"

/*
 These contain the business logic for the malloc() and realloc() implementation. Because of heap tracing
 wrapping reasons, we do not want these to be a public api, however, so they're not defined publicly.
*/
extern void *heap_caps_malloc_default( size_t size );
extern void *heap_caps_realloc_default( void *ptr, size_t size );

void *malloc(size_t size)
{
    return heap_caps_malloc_default(size);
}

void *calloc(size_t n, size_t size)
{
    return _calloc_r(_REENT, n, size);
}

void *realloc(void *ptr, size_t size)
{
    return heap_caps_realloc_default(ptr, size);
}

void free(void *ptr)
{
    heap_caps_free(ptr);
}

void *_malloc_r(struct _reent *r, size_t size)
{
    return heap_caps_malloc_default(size);
}

void _free_r(struct _reent *r, void *ptr)
{
    heap_caps_free(ptr);
}

void *_realloc_r(struct _reent *r, void *ptr, size_t size)
{
    return heap_caps_realloc_default( ptr, size );
}

void *_calloc_r(struct _reent *r, size_t nmemb, size_t size)
{
    void *result;
    size_t size_bytes;
    if (__builtin_mul_overflow(nmemb, size, &size_bytes)) {
        return NULL;
    }

    result = heap_caps_malloc_default(size_bytes);
    if (result != NULL) {
        bzero(result, size_bytes);
    }
    return result;
}

void *memalign(size_t alignment, size_t n)
{
    return heap_caps_aligned_alloc(alignment, n, MALLOC_CAP_DEFAULT);
}

int posix_memalign(void **out_ptr, size_t alignment, size_t size)
{
    if (size == 0) {
        /* returning NULL for zero size is allowed, don't treat this as an error */
        *out_ptr = NULL;
        return 0;
    }
    void *result = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_DEFAULT);
    if (result != NULL) {
        /* Modify output pointer only on success */
        *out_ptr = result;
        return 0;
    }
    /* Note: error returned, not set via errno! */
    return ENOMEM;
}
