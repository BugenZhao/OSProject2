#include <linux/bz_mm_limits.h>
#include <linux/cpuset.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/freezer.h>
#include <linux/ftrace.h>
#include <linux/gfp.h>
#include <linux/jiffies.h>
#include <linux/memcontrol.h>
#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/notifier.h>
#include <linux/oom.h>
#include <linux/ptrace.h>
#include <linux/ratelimit.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <linux/swap.h>
#include <linux/timex.h>

int bz_oom_worker(uid_t uid, int order, int strict);

void bz_oom_time_expires(unsigned long _uid) {
    uid_t uid = (uid_t)_uid;
    printk(KERN_INFO "Time's up for user %u\n", uid);
    set_mm_limit_waiting(uid, 0);
    bz_oom_worker(uid, 0, 1);
}

int bz_oom_worker(uid_t uid, int order, int strict) {
    struct task_struct *task;
    struct task_struct *selected = NULL;
    struct mm_struct *mm = NULL;

    unsigned long max_rss = 0;
    unsigned long sum_rss = 0;
    unsigned long rss = 0;
    unsigned long mm_limit;

    int count = 0;

    if ((mm_limit = get_mm_limit(uid)) == ULONG_MAX) { return 0; }

    read_lock_irq(&tasklist_lock);
    for_each_process(task) {
        if (task->cred->uid == uid) {
            struct task_struct *p = find_lock_task_mm(task); /* safe mm */
            if (!p) continue;
            rss = get_mm_rss(p->mm);
            if (p->pid == current->pid) { rss += 1 << order; }
            sum_rss += rss;
            if (rss > max_rss) {
                selected = p;
                max_rss = rss;
            }
            task_unlock(p);
        }
    }
    read_unlock_irq(&tasklist_lock);

    if ((sum_rss << PAGE_SHIFT) <= mm_limit) { return 0; }
    if (!selected) { return 0; }
    if (selected->flags & PF_KTHREAD) { return 0; }
    if (selected->flags & PF_EXITING) {
        set_tsk_thread_flag(selected, TIF_MEMDIE);
        return 0;
    }
    if (get_mm_limit_waiting(uid)) { return 0; }

    if (!strict) {
        struct timer_list timer;
        printk("*** 4 ***\n");

        init_timer(&timer);
        timer.expires = jiffies + HZ;
        timer.data = uid;
        timer.function = bz_oom_time_expires;

        printk(KERN_INFO
               "Selected '%s' (%d) of user %u, but we are not strict. "
               "Start a timer now!\n",
               selected->comm, selected->pid, uid);
        printk("*** 5 ***\n");
        add_timer(&timer);
        set_mm_limit_waiting(uid, 1);
        printk("*** 6 ***\n");
        return 0;
    }

    /* KILL NOW ! */
    selected = find_lock_task_mm(selected);
    if (!selected) { return 0; }

    mm = selected->mm;
    printk(KERN_ERR
           "*** Selected '%s' (%d): mm=%lu;"
           "    User %u: mm_sum=%lu, mm_limit=%lu ***\n",
           selected->comm, selected->pid, max_rss << PAGE_SHIFT, uid,
           sum_rss * PAGE_SHIFT, mm_limit);
    task_unlock(selected);

    read_lock_irq(&tasklist_lock);
    for_each_process(task) {
        if (task->mm == mm && !same_thread_group(task, selected) &&
            !(task->flags & PF_KTHREAD)) {
            task_lock(task);
            printk(KERN_ERR
                   "*** Killed '%s' (%d) since it is sharing same memory ***\n",
                   task->comm, task->pid);
            send_sig(SIGKILL, task, 0);
            task_unlock(task);
            count++;
        }
    }
    read_unlock_irq(&tasklist_lock);

    task_lock(selected);
    printk(KERN_ERR "*** Killed '%s' (%d) since it is selected ***\n",
           selected->comm, selected->pid);
    printk(KERN_ERR "*** uid=%u, uRSS=%lu, mm_max=%lu, pid=%u, pRSS=%lu ***\n",
           uid, sum_rss, mm_limit, selected->pid, max_rss);
    set_tsk_thread_flag(selected, TIF_MEMDIE);
    send_sig(SIGKILL, selected, 0);
    task_unlock(selected);

    count++;
    return count;
}
