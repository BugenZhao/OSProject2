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

#define __NR_mm_limit 272

MODULE_LICENSE("GPL");

#define DEFAULT_SYSCALL_TABLE ((void *)0xc000d8c4)
unsigned long **syscall_table = DEFAULT_SYSCALL_TABLE;

static int (*oldcall)(void);

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

static int set_mm_limit_syscall(uid_t uid, unsigned long mm_max) {
    int ok = 0;
    int i = 0;
    struct mm_limit_struct *p;

    printk(KERN_INFO "*** Hello ***\n");
    write_lock(&mm_limit_rwlock);

    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            p->mm_max = mm_max;
            ok = 1;
            printk(KERN_INFO "Updated: uid=%u, mm_max=%lu\n", p->uid,
                   p->mm_max);
        }
    }

    if (!ok) {
        struct mm_limit_struct *tmp =
            kmalloc(sizeof(struct mm_limit_struct), 0);
        tmp->uid = uid;
        tmp->mm_max = mm_max;
        list_add(&tmp->list, &init_mm_limit.list);
        printk(KERN_INFO "Added: uid=%u, mm_max=%lu\n", uid, mm_max);
    }

    printk(KERN_INFO "Current list:\n");
    list_for_each_entry(p, &init_mm_limit.list, list) {
        printk(KERN_INFO "  %2d: uid=%u, limit=%lu\n", i++, p->uid, p->mm_max);
    }

    write_unlock(&mm_limit_rwlock);
    printk(KERN_INFO "*** Bye ***\n");
    return 0;
}

// Initialization of module
static int mm_limit_init(void) {
    syscall_table = find_syscall_table();                     // Syscall table
    oldcall = (int (*)(void))(syscall_table[__NR_mm_limit]);  // old
    syscall_table[__NR_mm_limit] =
        (unsigned long *)set_mm_limit_syscall;  // new

    printk(KERN_INFO "*** mm_limit module loaded ***\n");
    return 0;
}

// Exit of module
static void mm_limit_exit(void) {
    syscall_table[__NR_mm_limit] = (unsigned long *)oldcall;  // restore
    printk(KERN_INFO "*** mm_limit module exited ***\n");
}

module_init(mm_limit_init);
module_exit(mm_limit_exit);
