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

#define PERF_TIMES 3
#define PERF_SIZE (1 << 28)

#define bugen_assert(case, lhs, op, rhs, format)                          \
    if (!((lhs)op(rhs))) {                                                \
        fprintf(stderr,                                                   \
                "%s:%d: Test case '" case "': ERROR: " #lhs " == " format \
                                          "\n",                           \
                __FILE__, __LINE__, (lhs));                               \
        exit(-1);                                                         \
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

    printf("Syscall test: all tests passed\n");
    return 0;
}

static int performance_test(void) {
    clock_t start, end;
    char *p;
    double time, time_sum = .0;
    int times;

    printf("Performance test: running WITHOUT mm_limit\n");
    times = PERF_TIMES;
    while (times--) {
        start = clock();
        p = malloc(PERF_SIZE);
        if (p != NULL) memset(p, 0x88, PERF_SIZE);
        end = clock();
        time_sum += (time = (end - start) / (double)CLOCKS_PER_SEC);
        printf("Performance test: allocated %u MB in %.2lf secs\n",
               (PERF_SIZE >> 20), time);
        free(p);
    }
    printf("Performance test: average time: %.2lf secs\n",
           time_sum / PERF_TIMES);

    time_sum = .0;
    syscall(__NR_mm_limit, getuid(), ULONG_MAX - 1);
    printf("Performance test: running WITH mm_limit\n");
    times = PERF_TIMES;
    while (times--) {
        start = clock();
        p = malloc(PERF_SIZE);
        if (p != NULL) memset(p, 0x88, PERF_SIZE);
        end = clock();
        time_sum += (time = (end - start) / (double)CLOCKS_PER_SEC);
        printf("Performance test: allocated %u MB in %.2lf secs\n",
               (PERF_SIZE >> 20), time);
        free(p);
    }
    printf("Performance test: average time: %.2lf secs\n",
           time_sum / PERF_TIMES);
    return 0;
}

int main(int argc, char **argv) {
    int *p, *q, i, time = 0;

    if (argc >= 2 && strcmp(argv[1], "test") == 0) {
        printf("Test mode (uid: %u)\n", getuid());
        syscall_test();
        performance_test();
        exit(0);
    }

    /* get time_allow_exceed from command line */
    if (argc >= 2 && (time = atoi(argv[1])) >= 0) {
        syscall(__NR_mm_limit_time, getuid(), TEN_MB * 3, time);
    } else {
        syscall(__NR_mm_limit, getuid(), TEN_MB * 3);
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

    printf("SHOULD NEVER REACH HERE: Bye\n");
    exit(0);
}
