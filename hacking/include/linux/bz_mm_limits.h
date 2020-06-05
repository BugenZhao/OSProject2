#ifndef _LINUX_BZ_MM_LIMITS_H
#define _LINUX_BZ_MM_LIMITS_H

#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/types.h>

struct mm_limit_struct {
    uid_t uid;
    unsigned long mm_max;
    int waiting;
    struct timer_list* timer;
    unsigned long time_allow_exceed;
    struct list_head list;
};

#define INIT_MM_LIMIT(mm_limit) \
    { .list = LIST_HEAD_INIT(mm_limit.list) }

#define WAIT_FOR_KILLING_TIME HZ / 10

extern struct mm_limit_struct init_mm_limit;
extern rwlock_t mm_limit_rwlock;

extern unsigned long get_mm_limit(uid_t uid);
extern int set_mm_limit_waiting(uid_t uid, int v);
extern int get_mm_limit_waiting(uid_t uid);
extern long long bz_start_timer(uid_t uid, void (*function)(unsigned long),
                                unsigned long custom_time);

#endif
