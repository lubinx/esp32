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
#include <stdint.h>

    enum UART_parity_t
    {
        UART_PARITY_NONE,
        UART_PARITY_ODD,
        UART_PARITY_EVEN,
    };

    enum UART_stopbits_t
    {
        UART_STOP_BITS_HALF,
        UART_STOP_BITS_ONE,
        UART_STOP_BITS_ONE_HALF,
        UART_STOP_BITS_TWO,
    };

__BEGIN_DECLS
    /**
     *  create uart fd
     *
     *  @param nb
     *      0, 1... must select by PIN configuration, its depends on driver implementation
     */
extern __attribute__((nothrow))
    int UART_createfd(int nb, uint32_t bps, enum UART_parity_t parity, enum UART_stopbits_t stopbits);

__END_DECLS
#endif
