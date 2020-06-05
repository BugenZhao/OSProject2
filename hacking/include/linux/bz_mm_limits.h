#ifndef _LINUX_BZ_MM_LIMITS_H
#define _LINUX_BZ_MM_LIMITS_H

#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>

struct mm_limit_struct {
    uid_t uid;
    unsigned long mm_max;
    int waiting;
    struct list_head list;
};

#define INIT_MM_LIMIT(mm_limit) \
    { .uid = 0, .mm_max = 0, .waiting = 0, .list = LIST_HEAD_INIT(mm_limit.list) }

extern struct mm_limit_struct init_mm_limit;
extern rwlock_t mm_limit_rwlock;

extern unsigned long get_mm_limit(uid_t uid);
extern int set_mm_limit_waiting(uid_t uid, int v);
extern int get_mm_limit_waiting(uid_t uid);

#endif
