// Main code of `set_mm_limit` syscall

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

#define DEFAULT_SYSCALL_TABLE 0xc000d8c4
static long *syscall_table = DEFAULT_SYSCALL_TABLE;

static int (*oldcall)(void);

static unsigned long **find_syscall_table() {
    unsigned long offset;
    unsigned long **sct;

    // sys call num maybe different
    // check in unistd.h
    //__NR_close will use 64bit version unistd.h by default when build LKM
    for (offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
        sct = (unsigned long **)offset;
        if (sct[__NR_close] == (unsigned long *)sys_close) {
            printk(KERN_INFO "Found syscall table: %p\n", sct);
            syscall_table = sct;
            return sct;
        }
    }

    printk(KERN_WARNING "Failed to find syscall table, use default value %p\n", DEFAULT_SYSCALL_TABLE);
    return DEFAULT_SYSCALL_TABLE;
}


static int set_mm_limit_syscall() {
    printk("HELLO!!!!!!!!\n");
}

// Initialization of module
static int mm_limit_init(void) {
    find_syscall_table();                  // Syscall table
    oldcall = (int (*)(void))(syscall_table[__NR_mm_limit]);      // Save the old one
    syscall_table[__NR_mm_limit] = (unsigned long)set_mm_limit_syscall;  // Set this one

    printk(KERN_INFO "*** mm_limit module loaded ***\n");
    return 0;
}

// Exit of module
static void mm_limit_exit(void) {
    syscall_table[__NR_mm_limit] = (unsigned long)oldcall;  // Restore the old one
    printk(KERN_INFO "*** mm_limit module exited ***\n");
}

module_init(mm_limit_init);
module_exit(mm_limit_exit);
