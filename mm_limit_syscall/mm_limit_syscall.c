// Main code of `set_mm_limit` syscall

#include <linux/bz_mm_limits.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>

/* # of syscalls */
#include "../common/syscall_num.h"

MODULE_LICENSE("GPL");

/* we will find the exact table address later */
#define DEFAULT_SYSCALL_TABLE ((void *)0xc000d8c4)
unsigned long **syscall_table = DEFAULT_SYSCALL_TABLE;

/* old syscalls */
static int (*oldcall_first)(void);
static int (*oldcall_second)(void);
static int (*oldcall_third)(void);

/* find the syscall table address */
static unsigned long **find_syscall_table(void) {
    unsigned long offset;
    unsigned long **sct;

    /* find syscall table by __NR_close */
    for (offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
        sct = (unsigned long **)offset;
        if (sct[__NR_close] == (unsigned long *)sys_close) {
            printk(KERN_INFO "Found syscall table: %p\n", sct);
            return sct;
        }
    }

    printk(KERN_WARNING "Failed to find syscall table, use default value %p\n",
           DEFAULT_SYSCALL_TABLE);
    return (unsigned long **)DEFAULT_SYSCALL_TABLE;
}

/* set_mm_limit_time system call, with time_allow_exceed in ms */
static int set_mm_limit_time_syscall(uid_t uid, unsigned long mm_max,
                                     unsigned int time_allow_exceed_ms) {
    int ok = 0;
    int i = 0;
    struct mm_limit_struct *p;
    /* convert ms to kernel ticks */
    unsigned long time_allow_exceed = time_allow_exceed_ms * HZ / 1000;

    /* avoid illegal calling */
    if (uid < 10000) {
        printk(KERN_ERR
               "*** Attempted to limit user with uid < 10000. Aborted. ***\n");
        return -EACCES;
    }

    /* check if the limit is already in the list */
    write_lock(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            if (mm_max == ULONG_MAX) {
                /* remove */
                list_del(&p->list);
                kfree(p->timer);
                kfree(p);
                printk(KERN_INFO "*** Removed: uid=%u ***\n", p->uid);
            } else {
                /* update mm_max and time_allow_exceed */
                p->mm_max = mm_max;
                p->time_allow_exceed = time_allow_exceed;
                /* reset last record for heuristic */
                p->last_time = jiffies;
                p->last_mm = 0;
                printk(KERN_INFO
                       "*** Updated: uid=%u, mm_max=%lu, time_allow_exceed=%lu "
                       "***\n",
                       p->uid, p->mm_max, p->time_allow_exceed);
            }
            ok = 1;
            break;
        }
    }
    write_unlock(&mm_limit_rwlock);

    /* limit not found in list, add it */
    if (!ok && mm_max != ULONG_MAX) {
        /* allocate a new mm_limit_struct */
        struct mm_limit_struct *tmp =
            kmalloc(sizeof(struct mm_limit_struct), GFP_KERNEL);

        tmp->uid = uid;       /* user id */
        tmp->mm_max = mm_max; /* memory limit */
        tmp->waiting = 0;     /* killer waiting flag */

        /* reset the last record for heuristic  */
        tmp->last_mm = 0;
        tmp->last_time = jiffies; /* NOW */

        /* just allocate memory for timer but keep it uninitialized */
        tmp->timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
        tmp->time_allow_exceed = time_allow_exceed;

        /* add it to list */
        write_lock(&mm_limit_rwlock);
        list_add(&tmp->list, &init_mm_limit.list);
        write_unlock(&mm_limit_rwlock);

        printk(KERN_INFO
               "*** Added: uid=%u, mm_max=%lu, time_allow_exceed=%lu ***\n",
               uid, mm_max, time_allow_exceed);
        ok = 1;
    }

    /* print the whole list for debugging*/
    if (ok) {
        printk(KERN_INFO "*** Current list: <begin>\n");
        read_lock(&mm_limit_rwlock);
        list_for_each_entry(p, &init_mm_limit.list, list) {
            printk(KERN_INFO
                   "  %2d: uid=%u, mm_max=%lu, time_allow_exceed=%lu\n",
                   i++, p->uid, p->mm_max, p->time_allow_exceed);
        }
        read_unlock(&mm_limit_rwlock);
        printk(KERN_INFO "                  <end> ***\n");
    }

    return ok ? 0 : -ENODATA;
}

/* convenience: set_mm_limit system call, without time_allow_exceed */
static int set_mm_limit_syscall(uid_t uid, unsigned long mm_max) {
    return set_mm_limit_time_syscall(uid, mm_max, 0);
}

/* get_mm_limit system call, will store the info in *ufound */
static int get_mm_limit_syscall(uid_t uid,
                                struct mm_limit_user_struct *ufound) {
    struct mm_limit_struct *p;
    struct mm_limit_user_struct kfound;
    int ok = 0;

    /* check if the limit is in the list, then copy to kfound */
    read_lock(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            kfound.mm_max = p->mm_max;
            kfound.time_allow_exceed_ms = p->time_allow_exceed * 1000 / HZ;
            ok = 1;
            break;
        }
    }
    read_unlock(&mm_limit_rwlock);

    /* copy to user */
    if (ok) {
        if (copy_to_user(ufound, &kfound,
                         sizeof(struct mm_limit_user_struct)) != 0) {
            return -ENOBUFS;
        }
    }
    return ok ? 0 : -ENODATA;
}

/* initialization of module */
static int mm_limit_init(void) {
    /* find the syscall table */
    syscall_table = find_syscall_table();

    /* save and replace 3 calls */
    oldcall_first = (int (*)(void))(syscall_table[__NR_mm_limit]);
    oldcall_second = (int (*)(void))(syscall_table[__NR_mm_limit_time]);
    oldcall_third = (int (*)(void))(syscall_table[__NR_get_mm_limit]);
    syscall_table[__NR_mm_limit] = (unsigned long *)set_mm_limit_syscall;
    syscall_table[__NR_mm_limit_time] =
        (unsigned long *)set_mm_limit_time_syscall;
    syscall_table[__NR_get_mm_limit] = (unsigned long *)get_mm_limit_syscall;

    printk(KERN_INFO "*** mm_limit module loaded ***\n");
    return 0;
}

/* exit of module */
static void mm_limit_exit(void) {
    /* restore 3 calls */
    syscall_table[__NR_mm_limit] = (unsigned long *)oldcall_first;
    syscall_table[__NR_mm_limit_time] = (unsigned long *)oldcall_second;
    syscall_table[__NR_get_mm_limit] = (unsigned long *)oldcall_third;
    printk(KERN_INFO "*** mm_limit module exited ***\n");
}

module_init(mm_limit_init);
module_exit(mm_limit_exit);
