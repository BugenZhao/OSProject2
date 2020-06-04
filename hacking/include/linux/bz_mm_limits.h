#ifndef _LINUX_BZ_MM_LIMITS_H
#define _LINUX_BZ_MM_LIMITS_H

struct mm_limit_struct {
    const char *hello, *world;
};

#define INIT_MM_LIMIT(mm_limit) \
    { .hello = "Hello", .world = "world" }

extern struct mm_limit_struct init_mm_limit;

#endif
