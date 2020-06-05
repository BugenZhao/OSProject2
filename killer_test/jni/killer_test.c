// Main code of problem 2

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TEN_MB (10 << 20)

#define __NR_mm_limit 356
#define __NR_mm_limit_time 357

#undef BUGEN_DEBUG

#define bugen_assert(no, lhs, op, rhs, format)                                 \
    if (!((lhs)op(rhs))) {                                                     \
        fprintf(stderr, "%s:%d: TEST " #no " ERROR: " #lhs " == " format "\n", \
                __FILE__, __LINE__, (lhs));                                    \
    }

int main(int argc, char **argv) {
    int *p, *q, i, time = 0;
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
        *p = p;
    }

    printf("Bye\n");
    exit(0);
}
