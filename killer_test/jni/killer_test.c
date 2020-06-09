#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* # of syscalls */
#include "../../common/syscall_num.h"

struct mm_limit_user_struct {
    unsigned long mm_max;
    unsigned long time_allow_exceed_ms;
};

#define TEN_MB (10 << 20)

#define bugen_assert(case, lhs, op, rhs, format)                          \
    if (!((lhs)op(rhs))) {                                                \
        fprintf(stderr,                                                   \
                "%s:%d: Test case '" case "': ERROR: " #lhs " == " format \
                                          "\n",                           \
                __FILE__, __LINE__, (lhs));                               \
    }

int syscall_test(void) {
    int ret;
    struct mm_limit_user_struct buf;

    /* test set */
    ret = syscall(__NR_mm_limit, 10060, TEN_MB * 10);
    bugen_assert("set", ret, ==, 0, "%d");

    /* test get 1 */
    ret = syscall(__NR_get_mm_limit, 10060, &buf);
    bugen_assert("get 1", ret, ==, 0, "%d");
    bugen_assert("get 1", buf.mm_max, ==, TEN_MB * 10, "%lu");

    /* test set with time */
    ret = syscall(__NR_mm_limit_time, 10060, TEN_MB * 20, 1000);
    bugen_assert("set with time", ret, ==, 0, "%d");

    /* test get 2 */
    ret = syscall(__NR_get_mm_limit, 10060, &buf);
    bugen_assert("get 2", ret, ==, 0, "%d");
    bugen_assert("get 2", buf.mm_max, ==, TEN_MB * 20, "%lu");
    bugen_assert("get 2", buf.time_allow_exceed_ms, ==, 1000, "%lu");

    /* test removal */
    ret = syscall(__NR_mm_limit, 10060, ULONG_MAX);
    bugen_assert("removal", ret, ==, 0, "%d");

    /* test get after removal */
    ret = syscall(__NR_get_mm_limit, 10060, &buf);
    bugen_assert("get after removal", ret, <, 0, "%d");

    ret = syscall(__NR_get_mm_limit, 10060, NULL);
    bugen_assert("get after removal", ret, <, 0, "%d");

    return 0;
}

int main(int argc, char **argv) {
    int *p, *q, i, time = 0;

    syscall_test();

    /* get time_allow_exceed from command line */
    if (argc >= 2 && (time = atoi(argv[1])) >= 0) {
        syscall(__NR_mm_limit_time, 10060, TEN_MB * 3, time);
    } else {
        syscall(__NR_mm_limit, 10060, TEN_MB * 3);
    }
    printf("Syscalled with allowed time: %dms\n", time);

#define LIMIT_1 TEN_MB * 2
    p = malloc(LIMIT_1);
    for (i = 0; i < LIMIT_1 / 4; i++) { p[i] = i; }
    printf("I survived ONCE\n");

#define LIMIT_2 TEN_MB * 2
    q = malloc(LIMIT_2);
    for (i = 0; i < LIMIT_2 / 4; i++) { q[i] = i; }
    printf("I survived TWICE\n");

#define LIMIT_3 TEN_MB * 2
    q = malloc(LIMIT_3);
    for (i = 0; i < LIMIT_3 / 4; i++) { q[i] = i; }
    printf("I survived THRICE\n");

    printf("Now I will allocate memory in an infinite loop!\n");
    while (1) {
        p = malloc(4);
        *p = (int)p;
    }

    printf("Bye\n");
    exit(0);
}
