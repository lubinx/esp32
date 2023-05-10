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
#include <unistd.h>
#include <sys/errno.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sh/cmdline.h"

/*****************************************************************************/
/** @internal
*****************************************************************************/
static char *CMD_iterate(char *p, char **retval);

/*****************************************************************************/
/** @export
*****************************************************************************/
int CMD_parse(char *cmdline, int lengthof_argv, char *argv[])
{
    int argc = 0;

    while (true)
    {
        char *param;
        cmdline = CMD_iterate(cmdline, &param);

        if (param)
        {
            argv[argc] = param;
            argc ++;

            if (argc > lengthof_argv)
                return __set_errno_neg(E2BIG);
        }
        else
            return argc;
    }
}

char *CMD_paramvalue(char *param)
{
    while (*param && '=' != *param)
        param ++;

    if (*param)
    {
        param ++;

        char *p = param;
        if ('"' == *p)
        {
            param ++;

            while (*p && '"' != *p) p ++;

            if ('"' == *p)
                *p = '\0';
        }
        return param;
    }
    else
        return NULL;
}

char *CMD_paramvalue_byname(char const *name, int argc, char *const argv[])
{
    char *retval = NULL;
    if (argc < 1)
        return retval;

    size_t namelen = strlen(name);

    for (int I = 0; I < argc; I ++)
    {
        char *param = argv[I];
        if (0 == strncmp(name, param, namelen) && '=' == param[namelen])
        {
            retval = &param[namelen + 1];
            break;
        }
    }
    return retval;
}

bool CMD_param_isoptional(char const *param)
{
    return  '-' == *(param -1);
}

/*****************************************************************************/
/** @internal
*****************************************************************************/
static inline  __attribute__((always_inline)) char *CMD_iterate(char *p, char **retval)
{
    *retval = NULL;
    if (! p)
        return p;

    // skip ctrl char & space
    while (*p && ' ' > *p)
        p ++;
    if (! *p)
        return p;

    // is a quoted str
    if ('"' == *p)
    {
        p ++;
        *retval = p;

        while (*p && '"' != *p)
            p ++;
    }
    else
    {
        if ('-' == *p)
            p++;
        *retval = p;

        while (*p && ' ' != *p)
            p ++;
    }

    // ** modify the original string and return
    if (*p)
    {
        *p = '\0';
        p ++;
    }

    // skip ctrl char & space
    while (*p && ' ' > *p)
        p ++;
    return p;
}
