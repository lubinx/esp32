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
#ifndef __ULTRACORE_ATOMIC_H
#define __ULTRACORE_ATOMIC_H            1

#include <features.h>
#include <stdint.h>

#ifdef __clang__
    #pragma GCC diagnostic ignored "-Watomic-implicit-seq-cst"
#endif

__BEGIN_DECLS

    /**
     *  ATOMIC_enter() / ATOMIC_leave()
     *      __disable_irq() / __enable_irq() with internal refcount
     */
extern __attribute__((nothrow))
    void ATOMIC_enter(void);
extern __attribute__((nothrow))
    void ATOMIC_leave(void);

    /**
     *  ATOMIC_inc32() / ATOMIC_add32() / ATOMIC_dec32() / ATOMIC_sub32()
     *      32bits int is only ultracore needed
     */
static inline __attribute__((nothrow))
    uint32_t ATOMIC_inc32(uint32_t volatile *ptr)
    {
    #ifdef __GNUC__
        return __sync_add_and_fetch(ptr, 1);
    #else
        return ++ (*ptr);
    #endif
    }

static inline __attribute__((nothrow))
    uint32_t ATOMIC_add32(uint32_t volatile *ptr, uint32_t val)
    {
    #ifdef __GNUC__
        return __sync_add_and_fetch(ptr, val);
    #else
        (*ptr) += val;
        return *ptr;
    #endif
    }

static inline __attribute__((nothrow))
    uint32_t ATOMIC_dec32(uint32_t volatile *ptr)
    {
    #ifdef __GNUC__
        return __sync_sub_and_fetch(ptr, 1);
    #else
        return -- (*ptr);
    #endif
    }

static inline __attribute__((nothrow))
    uint32_t ATOMIC_sub32(uint32_t volatile *ptr, uint32_t val)
    {
    #ifdef __GNUC__
        return __sync_sub_and_fetch(ptr, val);
    #else
        *ptr -= val;
        return *ptr;
    #endif
    }

__END_DECLS
#endif
