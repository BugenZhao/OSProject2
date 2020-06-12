#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* the # of syscalls */
#include "../../common/syscall_num.h"

/* mm_limit info for user */
struct mm_limit_user_struct {
    unsigned long mm_max;
    unsigned long time_allow_exceed_ms;
};

#define TEN_MB (10 << 20)

/* for performance test */
#define PERF_TIMES 10
#define PERF_SIZE (1 << 28)

/* test assertion utility */
#define bugen_assert(case, lhs, op, rhs, format)                          \
    if (!((lhs)op(rhs))) {                                                \
        fprintf(stderr,                                                   \
                "%s:%d: Test case '" case "': ERROR: " #lhs " == " format \
                                          "\n",                           \
                __FILE__, __LINE__, (lhs));                               \
        exit(-1);                                                         \
    }

/* test behaviors of system calls */
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

    /* test get to NULL */
    ret = syscall(__NR_get_mm_limit, 10060, NULL);
    bugen_assert("get after removal", ret, <, 0, "%d");

    /* test removal */
    ret = syscall(__NR_mm_limit, 10060, ULONG_MAX);
    bugen_assert("removal", ret, ==, 0, "%d");

    /* test get after removal */
    ret = syscall(__NR_get_mm_limit, 10060, &buf);
    bugen_assert("get after removal", ret, <, 0, "%d");

    printf("Syscall test: all tests passed\n");
    return 0;
}

/* test performance */
int performance_test(void) {
    clock_t start, end;
    char *p;
    double time, time_sum;
    int times;

    /* do two malloc's to avoid unstable results */
    printf("Performance test: preparing...\n");
    p = malloc(PERF_SIZE);
    if (p != NULL) memset(p, 0x88, PERF_SIZE);
    free(p);
    p = malloc(PERF_SIZE);
    if (p != NULL) memset(p, 0x88, PERF_SIZE);
    free(p);

    /* test with mm_limit */
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
        printf("Performance test: allocated %u MB in %.2lf cpu secs\n",
               (PERF_SIZE >> 20), time);
        free(p);
    }
    printf("Performance test: average time: %.2lf cpu secs\n",
           time_sum / PERF_TIMES);

    /* test without mm_limit */
    time_sum = .0;
    syscall(__NR_mm_limit, getuid(), ULONG_MAX);
    printf("Performance test: running WITHOUT mm_limit\n");
    times = PERF_TIMES;
    while (times--) {
        start = clock();
        p = malloc(PERF_SIZE);
        if (p != NULL) memset(p, 0x88, PERF_SIZE);
        end = clock();
        time_sum += (time = (end - start) / (double)CLOCKS_PER_SEC);
        printf("Performance test: allocated %u MB in %.2lf cpu secs\n",
               (PERF_SIZE >> 20), time);
        free(p);
    }
    printf("Performance test: average time: %.2lf cpu secs\n",
           time_sum / PERF_TIMES);

    return 0;
}

#define LIMIT (TEN_MB * 5)
char *p;

void race_timer_handler(int sig) {
    syscall(__NR_mm_limit, getuid(), ULONG_MAX);
}

int race_test(void) {
    syscall(__NR_mm_limit_time, getuid(), LIMIT, 2000);
    p = malloc(LIMIT);
    memset(p, 0x88, LIMIT);

    signal(SIGALRM, &race_timer_handler);
    alarm(1);
    sleep(5);
    printf("Race test: passed\n");
    return 0;
}

void timer_handler(int sig) {
    static int count = 0;
    printf("%d ms\n", ++count * 200);
    if (count == 3) {
        free(p);
        printf("Freed: 20%% of %dMB\n", LIMIT >> 20);
    }
    if (count == 6) {
        p = malloc(LIMIT / 5);
        memset(p, 0x88, LIMIT / 5);
        printf("Allocated again: 20%% of %dMB\n", LIMIT >> 20);
    }
}

int main(int argc, char **argv) {
    int i, time = 0;
    struct itimerval timer;
    int count = 0;

    /* test mode */
    if (argc >= 2) {
        printf("Test mode (uid: %u)\n", getuid());
        if (strcmp(argv[1], "syscall") == 0) {
            syscall_test();
            exit(0);
        } else if (strcmp(argv[1], "performance") == 0) {
            performance_test();
            exit(0);
        } else if (strcmp(argv[1], "race") == 0) {
            race_test();
            exit(0);
        }
        printf("Nothing to test\n");
    }

    /* get time_allow_exceed from command line */
    if (argc >= 2 && (time = atoi(argv[1])) >= 0) {
        syscall(__NR_mm_limit_time, getuid(), LIMIT, time);
    } else {
        syscall(__NR_mm_limit, getuid(), LIMIT);
    }
    printf("Syscalled with: limit=%dMB, time=%dms\n", LIMIT >> 20, time);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 200000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 200000;
    signal(SIGALRM, &timer_handler);

    /* start testing */
    printf("Allocated: ");
    for (count = 1; count <= 5; count++) {
        p = malloc(LIMIT / 5);
        memset(p, 0x88, LIMIT / 5);
        printf("%d%% ", count * 20);
    }
    printf("\n");
    setitimer(ITIMER_REAL, &timer, NULL);

    while (1)
        ;

    printf("SHOULD NEVER REACH HERE: Bye\n");
    exit(0);
}
