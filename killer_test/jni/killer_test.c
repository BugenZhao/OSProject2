// Main code of problem 2

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TEN_MB (10 << 20)

#define __NR_mm_limit 272

#undef BUGEN_DEBUG

#define bugen_assert(no, lhs, op, rhs, format)                                 \
    if (!((lhs)op(rhs))) {                                                     \
        fprintf(stderr, "%s:%d: TEST " #no " ERROR: " #lhs " == " format "\n", \
                __FILE__, __LINE__, (lhs));                                    \
    }

int main(int argc, char **argv) {
    int *p, *q, i;

    syscall(__NR_mm_limit, 10060, TEN_MB * 3);
    printf("Syscalled\n");

#define LIMIT_1 TEN_MB * 2
    p = malloc(LIMIT_1);
    for (i = 0; i < LIMIT_1 / 4; i++) { p[i] = i; }
    printf("I survived ONCE\n");

#define LIMIT_2 TEN_MB * 2
    q = malloc(LIMIT_2);
    for (i = 0; i < LIMIT_2 / 4; i++) { q[i] = p[i]; }
    printf("I survived TWICE\n");

    printf("Bye\n");
    exit(0);
}
