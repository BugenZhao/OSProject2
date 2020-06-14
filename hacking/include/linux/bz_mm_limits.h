/* definition of global vars & helper funcs */

#ifndef _LINUX_BZ_MM_LIMITS_H
#define _LINUX_BZ_MM_LIMITS_H

#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/types.h>

/* mm_limit info for user */
struct mm_limit_user_struct {
    unsigned long mm_max;
    unsigned long time_allow_exceed_ms;
};

#ifdef __KERNEL__
/* struct definition of mm_limit */
struct mm_limit_struct {
    uid_t uid;                       /* user id */
    unsigned long mm_max;            /* memory limit for this user in bytes */
    int waiting;                     /* is killer for this user waiting? */
    struct timer_list* timer;        /* killer timer for this user */
    unsigned long time_allow_exceed; /* time allowed to exceed the limit */
    unsigned long last_mm;
    unsigned long last_time;
    struct list_head list;           /* for linked list */
};

/* how many ticks will we waiting for a process to be killed? */
#define WAIT_FOR_KILLING_TIME HZ / 15

/* initialization for the head of mm_limit list */
#define INIT_MM_LIMIT(mm_limit) \
    { .list = LIST_HEAD_INIT(mm_limit.list) }

/* the head of mm_limit list */
extern struct mm_limit_struct init_mm_limit;

/* rwlock for mm_limit list */
extern rwlock_t mm_limit_rwlock;

/* declarations */
int set_mm_limit_waiting(uid_t uid, int v);
int get_mm_limit_waiting(uid_t uid);
struct mm_limit_struct *find_lock_mm_limit_struct(uid_t uid);
long long bz_start_timer(uid_t uid, void (*function)(unsigned long),
                                unsigned long custom_time);

#endif /* __KERNEL */
#endif /* _LINUX_BZ_MM_LIMITS_H */
