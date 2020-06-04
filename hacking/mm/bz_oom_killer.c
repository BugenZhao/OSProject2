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

int bz_oom_worker(uid_t uid, int order) {
    struct task_struct *task;
    struct task_struct *selected = NULL;
    unsigned long max_rss = 0;
    unsigned long sum_rss = 0;
    unsigned long rss = 0;
    unsigned long mm_limit;

    if ((mm_limit = get_mm_limit(uid)) == ULONG_MAX) { return 0; }

    read_lock_irq(&tasklist_lock);

    for_each_process(task) {
        if (task->cred->uid == uid) {
            struct task_struct *p = find_lock_task_mm(task);
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

    if ((sum_rss << 12) <= mm_limit) {
        read_unlock_irq(&tasklist_lock);
        return 0;
    }

    if (selected) {
        printk(KERN_INFO
               "*** Selected '%s' (%d): mm=%lu;"
               "    User %u: mm_sum=%lu, mm_limit=%lu ***\n",
               selected->comm, selected->pid, max_rss * PAGE_SIZE, uid, sum_rss * PAGE_SIZE,
               mm_limit);
        task_lock(selected);
        send_sig(SIGKILL, selected, 0);
        task_unlock(selected);

        printk(KERN_INFO
               "*** uid=%u, uRSS=%lu, mm_max=%lu, pid=%u, pRSS=%d ***\n",
               uid, sum_rss, mm_limit, selected->pid, max_rss);

        read_unlock_irq(&tasklist_lock);
        return 1;
    }

    read_unlock_irq(&tasklist_lock);
    return 0;
}
