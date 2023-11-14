#include <stdlib.h>
#include <time.h>
#include <rtc.h>
#include <sys/errno.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_datetime(struct UCSH_env *env)
{
    time_t val;

    if (env->argc == 1)
    {
        val = time(NULL);
        struct tm tv;
        localtime_r(&val, &tv);

        UCSH_printf(env, "%d/%d/%d %02d:%02d:%02d\r\n",
            tv.tm_year + 1900, tv.tm_mon + 1, tv.tm_mday, tv.tm_hour, tv.tm_min, tv.tm_sec);
    }
    else if (env->argc == 2)
    {
        val = (time_t)atoi(CMD_paramvalue_byname("epoch", env->argc, env->argv));
        if (val == 0)
            goto dt_invalid_param;
        else
            stime(val);
    }
    else if (env->argc == 3)
    {
        struct tm tv;
        /// datetime with @format year/mon/day hh:nn:ss
        char *str = env->argv[1];
        tv.tm_year = strtol(str, &str, 10);

        str ++;
        tv.tm_mon  = strtol(str, &str, 10) - 1;

        str ++;
        tv.tm_mday = strtol(str, NULL, 10);

        str = env->argv[2];
        tv.tm_hour = strtol(str, &str, 10);
        str ++;
        tv.tm_min  = strtol(str, &str, 10);
        str ++;
        tv.tm_sec  = strtol(str, NULL, 10);

        if (// tv.tm_year < 2000 || tv.tm_year > 2050 ||
            tv.tm_mon < 0 || tv.tm_mon > 11 ||
            tv.tm_mday < 0 || tv.tm_mday > 31 ||
            tv.tm_hour < 0 || tv.tm_hour > 23 ||
            tv.tm_min < 0 || tv.tm_min > 59 ||
            tv.tm_sec < 0 || tv.tm_sec > 59)
        {
            goto dt_invalid_param;
        }

        tv.tm_year -= 1900;
        val = mktime((struct tm *)&tv);
        if (val == (time_t)(-1))
            goto dt_invalid_param;
        else
            stime(val);
    }
    else
    {
dt_invalid_param:
        return EINVAL;
    }

    return 0;
}
