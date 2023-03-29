#include <sys/errno.h>
#include <sys/reent.h>

#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

/****************************************************************************
* @implements
****************************************************************************/
int adjtime(const struct timeval *delta, struct timeval *outdelta)
{
    ARG_UNUSED(delta, outdelta);
    return __set_errno_neg(ENOSYS);
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    ARG_UNUSED(tv, tz);
    return __set_errno_neg(ENOSYS);
}

int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
    ARG_UNUSED(clock_id, tp);
    return __set_errno_neg(ENOSYS);
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    ARG_UNUSED(clock_id, tp);
    return __set_errno_neg(ENOSYS);
}

int clock_getres(clockid_t clock_id, struct timespec *res)
{
    ARG_UNUSED(clock_id, res);
    return __set_errno_neg(ENOSYS);
}
