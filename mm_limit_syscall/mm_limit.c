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

/* # of two syscalls */
#define __NR_mm_limit 356
#define __NR_mm_limit_time 357

MODULE_LICENSE("GPL");

/* we will find the exact table address later */
#define DEFAULT_SYSCALL_TABLE ((void *)0xc000d8c4)
unsigned long **syscall_table = DEFAULT_SYSCALL_TABLE;

/* old syscalls */
static int (*oldcall_first)(void);
static int (*oldcall_second)(void);

/* find the syscall table address */
static unsigned long **find_syscall_table(void) {
    unsigned long offset;
    unsigned long **sct;

    // sys call num maybe different
    // check in unistd.h
    //__NR_close will use 64bit version unistd.h by default when build LKM
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
    unsigned long time_allow_exceed = time_allow_exceed_ms * HZ / 1000;

    if (uid < 10000) {
        printk(KERN_ERR "Attempted to limit user with uid < 10000. Aborted.\n");
        return -1;
    }

    write_lock(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            p->mm_max = mm_max;
            p->time_allow_exceed = time_allow_exceed;
            ok = 1;
            printk(KERN_INFO
                   "Updated: uid=%u, mm_max=%lu, time_allow_exceed=%lu\n",
                   p->uid, p->mm_max, p->time_allow_exceed);
            break;
        }
    }
    write_unlock(&mm_limit_rwlock);

    if (!ok) {
        struct mm_limit_struct *tmp =
            kmalloc(sizeof(struct mm_limit_struct), GFP_KERNEL);

        tmp->uid = uid;
        tmp->mm_max = mm_max;
        tmp->waiting = 0;
        tmp->timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
        tmp->time_allow_exceed = time_allow_exceed;

        write_lock(&mm_limit_rwlock);
        list_add(&tmp->list, &init_mm_limit.list);
        write_unlock(&mm_limit_rwlock);

        printk(KERN_INFO "Added: uid=%u, mm_max=%lu, time_allow_exceed=%lu\n",
               uid, mm_max, time_allow_exceed);
    }

    printk(KERN_INFO "Current list:\n");

    read_lock(&mm_limit_rwlock);
    list_for_each_entry(p, &init_mm_limit.list, list) {
        printk(KERN_INFO "  %2d: uid=%u, mm_max=%lu, time_allow_exceed=%lu\n",
               i++, p->uid, p->mm_max, p->time_allow_exceed);
    }
    read_unlock(&mm_limit_rwlock);

    return 0;
}

/* convenience: set_mm_limit system call, without time_allow_exceed */
static int set_mm_limit_syscall(uid_t uid, unsigned long mm_max) {
    return set_mm_limit_time_syscall(uid, mm_max, 0);
}

/* initialization of module */
static int mm_limit_init(void) {
    syscall_table = find_syscall_table(); 
    oldcall_first = (int (*)(void))(syscall_table[__NR_mm_limit]);
    oldcall_second = (int (*)(void))(syscall_table[__NR_mm_limit_time]);

    syscall_table[__NR_mm_limit] = (unsigned long *)set_mm_limit_syscall;
    syscall_table[__NR_mm_limit_time] =
        (unsigned long *)set_mm_limit_time_syscall;

    printk(KERN_INFO "*** mm_limit module loaded ***\n");
    return 0;
}

/* exit of module */
static void mm_limit_exit(void) {
    syscall_table[__NR_mm_limit] = (unsigned long *)oldcall_first;
    syscall_table[__NR_mm_limit_time] = (unsigned long *)oldcall_second;
    printk(KERN_INFO "*** mm_limit module exited ***\n");
}

module_init(mm_limit_init);
module_exit(mm_limit_exit);
