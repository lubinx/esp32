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

/****************************************************************************
* @def & @internal
****************************************************************************/
#define DAYS_PER_YEAR                   (365U)
#define DAYS_4YEAR                      (4 * DAYS_PER_YEAR + 1)
#define DAYS_100YEAR                    (25 * DAYS_4YEAR - 1)
#define DAYS_400YEAR                    (4 * DAYS_100YEAR + 1)

static uint32_t leap_years_passed(uint32_t days)
{
    uint32_t quad_centuries = days / DAYS_400YEAR;
    days -= quad_centuries;

    uint32_t centuries = days / DAYS_100YEAR;
    days += centuries;

    return (days / DAYS_4YEAR) - centuries + quad_centuries;
}

static uint32_t leap_days_passed(uint32_t days)
{
    return leap_years_passed(days + DAYS_PER_YEAR - 59 + 1);
}

/****************************************************************************
* @implements: localtime() / localtime_r() / gmtime() / gmtime_r()
****************************************************************************/
struct tm *localtime(time_t const *timep)
{
    // assume no timezone, localtime = gmtime
    return gmtime(timep);
}

struct tm *localtime_r(time_t const *timep, struct tm *r)
{
    return gmtime_r(timep, r);
}

struct tm *gmtime(time_t const *timep)
{
    static struct tm r;
    return gmtime_r(timep, &r);
}

struct tm *gmtime_r(time_t const *timep, struct tm *r)
{
    // time of day
    {
        int time = (int)(*timep % 86400);
        r->tm_hour = time / 3600;

        time %= 3600;
        r->tm_min = time / 60;
        r->tm_sec = time % 60;

        r->tm_isdst = 0;    // assume DST always false
    }

    // days shift from 1601/1/1
    uint32_t days = (uint32_t)(*timep / 86400 + (3 * DAYS_100YEAR + 17 * DAYS_4YEAR + 1 * DAYS_PER_YEAR));
    r->tm_wday = (int)((days + 1) % 7);

    uint32_t leap_days = leap_days_passed(days);
    uint32_t leap_years = leap_years_passed(days);

    // years & year day
    {
        uint32_t years = (days - leap_days) / 365;

        r->tm_year = (int)(years - 299);
        r->tm_yday = (int)(days - years * 365 - leap_years);
    }

    // month & month day
    {
        static uint16_t const normal_monthdays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
        static uint16_t const leap_year_monthdays[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

        uint8_t month;
        uint16_t const *monthdays;

        // are more leap days passed than leap years?
        if (leap_days > leap_years)
            monthdays = leap_year_monthdays;
        else
            monthdays = normal_monthdays;

        // dayinyear < 366 => terminates with i <= 11
        for (month = 0; r->tm_yday  >= monthdays[month + 1]; month ++)
            ;

        r->tm_mon = month;
        r->tm_mday = (int)(1 + r->tm_yday  - monthdays[month]);
    }

    return r;
}
