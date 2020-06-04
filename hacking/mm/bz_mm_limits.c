#include <linux/bz_mm_limits.h>
#include <linux/export.h>

struct mm_limit_struct init_mm_limit = INIT_MM_LIMIT(init_mm_limit);
DEFINE_RWLOCK(mm_limit_rwlock);

EXPORT_SYMBOL(mm_limit_rwlock);
EXPORT_SYMBOL(init_mm_limit);

unsigned long get_mm_limit(uid_t uid) {
    struct mm_limit_struct *p;
    list_for_each_entry(p, &init_mm_limit.list, list) {
        if (p->uid == uid) {
            return p->mm_max;
        }
    }
    return ULONG_MAX;
}

EXPORT_SYMBOL(get_mm_limit);
