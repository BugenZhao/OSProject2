/* body of helper funcs */

#include <linux/bz_mm_limits.h>
#include <linux/export.h>
#include <linux/timer.h>

/* the head of mm_limit list */
struct mm_limit_struct init_mm_limit = INIT_MM_LIMIT(init_mm_limit);
/* rwlock for mm_limit list */
DEFINE_RWLOCK(mm_limit_rwlock);

EXPORT_SYMBOL(mm_limit_rwlock);
EXPORT_SYMBOL(init_mm_limit);

/* find the mm_limit_struct for the given uid and lock the list, if not found,
 * return NULL with list unlocked */
struct mm_limit_struct *find_lock_mm_limit_struct(uid_t uid) {
    struct mm_limit_struct *p;

    /* lock the list */
    write_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) { return p; }
    }
    /* uid not found */
    write_unlock_irq(&mm_limit_rwlock);
    return NULL;
}

/* set the killer's waiting state for the given uid to v */
int set_mm_limit_waiting(uid_t uid, int v) {
    struct mm_limit_struct *p;

    /* lock the list */
    write_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            p->waiting = v;
            write_unlock_irq(&mm_limit_rwlock);
            return 0;
        }
    }
    /* uid not found */
    write_unlock_irq(&mm_limit_rwlock);
    return -2;
}

/* get the killer's waiting state for the given uid */
int get_mm_limit_waiting(uid_t uid) {
    struct mm_limit_struct *p;

    /* lock the list */
    read_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            read_unlock_irq(&mm_limit_rwlock);
            return p->waiting;
        }
    }
    /* uid not found */
    read_unlock_irq(&mm_limit_rwlock);
    return -2;
}

/* start a killer timer for the given uid, with the callback function, if
 * custom_time is 0, the default time_allow_exceed will be used;
 * return the expires time, or negative value if failed */
long long bz_start_timer(uid_t uid, void (*function)(unsigned long),
                         unsigned long custom_time) {
    struct mm_limit_struct *p;

    /* lock the list */
    write_lock_irq(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            /* either custom_time, or time_allow_exceed ? */
            unsigned time = custom_time ? custom_time : p->time_allow_exceed;

            if (p->waiting) {
                /* the timer may have been started! */
                write_unlock_irq(&mm_limit_rwlock);
                return -3;
            }

            if (time == 0) {
                /* may not want to start the timer */
                write_unlock_irq(&mm_limit_rwlock);
                return -1;
            }

            p->timer->expires = jiffies + time; /* set expiration time */
            p->timer->data = uid;               /* argument to the callback */
            p->timer->function = function;      /* callback */
            p->waiting = 1;      /* killer should be waiting from now */
            add_timer(p->timer); /* add timer to kernel */

            write_unlock_irq(&mm_limit_rwlock);
            return time;
        }
    }
    /* uid not found */
    write_unlock_irq(&mm_limit_rwlock);
    return -2;
}
