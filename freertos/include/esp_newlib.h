/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <features.h>
#include <sys/reent.h>
#include <string.h>

__BEGIN_DECLS

static inline
    void esp_reent_init(struct _reent *r)
    {
        // soo..stupid.. this will only need to tell newlib reent is not initialized
        r->__sdidinit = 0;

        /*
        //  REVIEW: SOO..hardcoded esp_reent_init() instead of __sinit()?
        memset(r, 0, sizeof(*r));

        r->_stdout = _GLOBAL_REENT->_stdout;
        r->_stderr = _GLOBAL_REENT->_stderr;
        r->_stdin  = _GLOBAL_REENT->_stdin;

        extern void _cleanup_r(struct _reent *r); // REVIEW: import from where?
        r->__cleanup = &_cleanup_r;
        r->__sdidinit = 1;
        */
    }

__END_DECLS
