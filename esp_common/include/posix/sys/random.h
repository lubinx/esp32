/*
 * SPDX-FileCopyrightText: 2018-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SYS_RANDOM__
#define __SYS_RANDOM__

#include <features.h>
#include <sys/types.h>

__BEGIN_DECLS

extern __attribute__((nothrow))
    ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);

__END_DECLS
#endif
