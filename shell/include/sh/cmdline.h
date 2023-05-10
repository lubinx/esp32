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
#ifndef __CMDLINE_H
#define __CMDLINE_H                     1

#include <features.h>

#include <stdbool.h>
#include <unistd.h>

__BEGIN_DECLS

extern __attribute__((nonnull, nothrow))
    int CMD_parse(char *cmdline, int lengthof_argv, char *argv[]);

extern __attribute__((nonnull))
    char *CMD_paramvalue_byname(char const *name, int argc, char *const argv[]);

extern __attribute__((nonnull))
    char *CMD_paramvalue(char *param);

extern __attribute__((nonnull))
    bool CMD_param_isoptional(char const *param);

__END_DECLS
#endif
