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
#ifndef __SCHED_H
#define __SCHED_H

#include <features.h>

#include <sys/types.h>
#include <sys/sched.h>

/*
    struct sched_param
    {
        int sched_ss_low_priority;      // Low scheduling priority for  sporadic server.
        struct timespec sched_ss_repl_period;   // Replenishment period for  sporadic server.
        struct timespec sched_ss_init_budget;   // Initial budget for sporadic server.
        int sched_ss_max_repl;          // Maximum pending replenishments for sporadic server.
    };

    enum sched_policy_t
    {
        SCHED_FIFO,                     // First in-first out (FIFO) scheduling policy.
        SCHED_RR,                       // Round robin scheduling policy.
        SCHED_SPORADIC,                 // Sporadic server scheduling policy.
        SCHED_OTHER                     // Another scheduling policy.
    };
*/

__BEGIN_DECLS

extern __attribute__((nothrow))
    int sched_get_priority_max(int policy);
extern __attribute__((nothrow))
    int sched_get_priority_min(int policy);

extern __attribute__((nonnull, nothrow))
    int sched_getparam(pid_t pid, struct sched_param *param);
extern __attribute__((nothrow))
    int sched_getscheduler(pid_t pid);

extern __attribute__((nonnull, nothrow))
    int sched_rr_get_interval(pid_t pid, struct timespec *intv);

extern __attribute__((nonnull, nothrow))
    int sched_setparam(pid_t pid, struct sched_param const *param);
extern __attribute__((nonnull, nothrow))
    int sched_setscheduler(pid_t pid, int policy, struct sched_param const *param);

extern __attribute__((nothrow))
    int sched_yield(void);

__END_DECLS
#endif
