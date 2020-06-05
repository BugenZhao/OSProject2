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

/* timer callback: time_allow_exceed expires */
void bz_oom_time_expires(unsigned long _uid) {
    uid_t uid = (uid_t)_uid;
    printk(KERN_INFO "*** Time for checking user %u again. ***\n", uid);
    set_mm_limit_waiting(uid, 0);
    bz_oom_worker(uid, 0, 1);
}

/* timer callback: time to check if exactly killed */
void bz_oom_kill_expires(unsigned long _uid) {
    uid_t uid = (uid_t)_uid;
    printk(KERN_INFO
           "*** Time for checking if process of user %u is killed. ***\n",
           uid);
    set_mm_limit_waiting(uid, 0);
    bz_oom_worker(uid, 0, 2);
}

/* heart of bugen's oom killer */
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
    if (get_mm_limit_waiting(uid)) { return 0; }

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

    if ((sum_rss << PAGE_SHIFT) <= mm_limit || !selected) {
        if (strict) {
            printk(KERN_INFO "*** No process to select for user %u now. ***\n",
                   uid);
        }
        return 0;
    }
    if (selected->flags & PF_KTHREAD) { return 0; }
    if (selected->flags & PF_EXITING) {
        set_tsk_thread_flag(selected, TIF_MEMDIE);
        return 0;
    }

    if (!strict) {
        long long timer_time = bz_start_timer(uid, &bz_oom_time_expires, 0);

        if (timer_time > 0) {
            printk(KERN_INFO
                   "*** Selected '%s' (%d) of user %u for the the first time. "
                   "Start a timer of %lld ticks now! ***\n",
                   selected->comm, selected->pid, uid, timer_time);
            return 0;
        }
    }

    /* KILL NOW ! */
    selected = find_lock_task_mm(selected);
    if (unlikely(!selected)) { return 0; }

    mm = selected->mm;
    printk(KERN_ERR
           "*** Selected '%s' (%d) of user %u. "
           "Stats: mm=%lu, mm_sum=%lu, mm_limit=%lu ***\n",
           selected->comm, selected->pid, uid, max_rss << PAGE_SHIFT,
           sum_rss << PAGE_SHIFT, mm_limit);
    task_unlock(selected);

    read_lock_irq(&tasklist_lock);
    for_each_process(task) {
        if (task->mm == mm && !same_thread_group(task, selected) &&
            !(task->flags & PF_KTHREAD)) {
            task_lock(task);
            printk(KERN_ERR
                   "*** Killing '%s' (%d) of user %u since it is sharing same "
                   "memory. ***\n",
                   task->comm, task->pid, task->cred->uid);
            send_sig(SIGKILL, task, 0);
            task_unlock(task);
            count++;
        }
    }
    read_unlock_irq(&tasklist_lock);

    task_lock(selected);
    if (strict <= 1) {
        printk(KERN_ERR
               "*** Killing '%s' (%d) of user %u since it is selected. ***\n",
               selected->comm, selected->pid, uid);
        printk(KERN_ERR
               "*** uid=%u, uRSS=%lupages=%lubytes, mm_max=%lubytes; pid=%u, "
               "pRSS=%lupages=%lubytes ***\n",
               uid, sum_rss, sum_rss << PAGE_SHIFT, mm_limit, selected->pid,
               max_rss, max_rss << PAGE_SHIFT);
        set_tsk_thread_flag(selected, TIF_MEMDIE);
        send_sig(SIGKILL, selected, 0);
        bz_start_timer(uid, bz_oom_kill_expires, WAIT_FOR_KILLING_TIME);
    } else {
        printk(KERN_ERR "*** Force-killing '%s' (%d) of user %u. ***\n",
               selected->comm, selected->pid, uid);
        force_sig(SIGKILL, selected);
        bz_start_timer(uid, bz_oom_kill_expires, WAIT_FOR_KILLING_TIME);
    }
    task_unlock(selected);

    count++;
    return count;
}