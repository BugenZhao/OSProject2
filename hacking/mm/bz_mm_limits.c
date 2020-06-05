#include <linux/bz_mm_limits.h>
#include <linux/export.h>
#include <linux/timer.h>

/* the head of mm_limit list */
struct mm_limit_struct init_mm_limit = INIT_MM_LIMIT(init_mm_limit);
/* rwlock for mm_limit list */
DEFINE_RWLOCK(mm_limit_rwlock);

EXPORT_SYMBOL(mm_limit_rwlock);
EXPORT_SYMBOL(init_mm_limit);

/* get the memory limit for the given uid */
unsigned long get_mm_limit(uid_t uid) {
    struct mm_limit_struct *p;

    read_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            read_unlock_irq(&mm_limit_rwlock);
            return p->mm_max;
        }
    }
    read_unlock_irq(&mm_limit_rwlock);
    return ULONG_MAX;
}

/* set the killer's waiting state for the given uid to v */
int set_mm_limit_waiting(uid_t uid, int v) {
    struct mm_limit_struct *p;

    write_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            p->waiting = v;
            write_unlock_irq(&mm_limit_rwlock);
            return 0;
        }
    }
    write_unlock_irq(&mm_limit_rwlock);
    return -1;
}

/* get the killer's waiting state for the given uid */
int get_mm_limit_waiting(uid_t uid) {
    struct mm_limit_struct *p;

    read_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            read_unlock_irq(&mm_limit_rwlock);
            return p->waiting;
        }
    }
    read_unlock_irq(&mm_limit_rwlock);
    return -1;
}

/* start a killer timer for the given uid, with the callback function, if
 * custom_time is 0, the default time_allow_exceed will be used */
long long bz_start_timer(uid_t uid, void (*function)(unsigned long),
                         unsigned long custom_time) {
    struct mm_limit_struct *p;

    write_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            unsigned time = custom_time ? custom_time : p->time_allow_exceed;

            if (time == 0) {
                write_unlock_irq(&mm_limit_rwlock);
                return -1;
            }

            init_timer(p->timer);
            p->timer->expires = jiffies + time;
            p->timer->data = uid;
            p->timer->function = function;
            p->waiting = 1;
            add_timer(p->timer);

            write_unlock_irq(&mm_limit_rwlock);
            return time;
        }
    }
    write_unlock_irq(&mm_limit_rwlock);
    return -2;
}

EXPORT_SYMBOL(get_mm_limit);
