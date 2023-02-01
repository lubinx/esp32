#include <sys/errno.h>
#include <sys/reent.h>

#include <sys/time.h>
#include <sys/times.h>

int adjtime(const struct timeval *delta, struct timeval *outdelta)
{
    return __set_errno_neg(ENOSYS);
}

clock_t _times_r(struct _reent *r, struct tms *ptms)
{
    return 0;
}

int _gettimeofday_r(struct _reent *r, struct timeval *tv, void *tz)
{
    return __set_errno_neg(ENOSYS);
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    return __set_errno_neg(ENOSYS);
}

int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
    return __set_errno_neg(ENOSYS);
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    return __set_errno_neg(ENOSYS);
}

int clock_getres(clockid_t clock_id, struct timespec *res)
{
    return __set_errno_neg(ENOSYS);

}
