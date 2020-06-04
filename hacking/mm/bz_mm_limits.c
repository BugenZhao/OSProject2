#include <linux/bz_mm_limits.h>
#include <linux/export.h>

struct mm_limit_struct init_mm_limit = INIT_MM_LIMIT(init_mm_limit);

DEFINE_RWLOCK(mm_limit_rwlock);

EXPORT_SYMBOL(mm_limit_rwlock);
EXPORT_SYMBOL(init_mm_limit);
