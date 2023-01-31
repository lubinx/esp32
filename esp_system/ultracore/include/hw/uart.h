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
#ifndef __UART_H
#define __UART_H

#include <features.h>

__BEGIN_DECLS
    typedef enum {paNone, paOdd, paEven} parity_t;

    /**
     *  create uart fd
     *
     *  @param nb
     *      0, 1... must select by PIN configuration, its depends on driver implementation
     */
extern __attribute__((nothrow))
    int UART_createfd(int nb, uint32_t bps, parity_t parity, uint8_t stop_bits);

__END_DECLS
#endif
