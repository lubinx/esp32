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
#ifndef __SYS_STIME_H
#define __SYS_STIME_H                   1

#include <features.h>
#include "sys/_timespec.h"

__BEGIN_DECLS

    /**
     *  none-posix, direct setup time use timestamp
    */
extern __attribute__((nothrow))
    int stime(time_t const ts);

__END_DECLS
#endif
