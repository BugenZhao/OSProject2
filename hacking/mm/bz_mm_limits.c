#include <linux/bz_mm_limits.h>
#include <linux/init.h>
#include <linux/export.h>

struct mm_limit_struct init_mm_limit = INIT_MM_LIMIT(init_mm_limit);

EXPORT_SYMBOL(init_mm_limit);
