#include <sys/errno.h>
#include <sched.h>

int sched_get_priority_max(int policy)
{
    ARG_UNUSED(policy);
    return 0;
}

int sched_get_priority_min(int policy)
{
    ARG_UNUSED(policy);
    return 0;
}

int sched_getparam(pid_t pid, struct sched_param *param)
{
    ARG_UNUSED(pid, param);
    return ENOSYS;
}

int sched_getscheduler(pid_t pid)
{
    ARG_UNUSED(pid);
    return ENOSYS;
}

int sched_rr_get_interval(pid_t pid, struct timespec *tv)
{
    ARG_UNUSED(pid, tv);
    return ENOSYS;
}

int sched_setparam(pid_t pid, struct sched_param const *param)
{
    ARG_UNUSED(pid, param);
    return ENOSYS;
}

int sched_setscheduler(pid_t pid, int policy, struct sched_param const *param)
{
    ARG_UNUSED(pid, policy, param);
    return ENOSYS;
}

// move to _rtos_freetos_impl.c, to avoid include freertos/<etc.>
/*
int sched_yield(void)
{
    taskYIELD();
    return 0;
}
*/
