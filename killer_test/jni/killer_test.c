// Main code of problem 2

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_PRO_CNT 1 << 10

#define __NR_mm_limit 272

#undef BUGEN_DEBUG

#define bugen_assert(no, lhs, op, rhs, format)                                 \
    if (!((lhs)op(rhs))) {                                                     \
        fprintf(stderr, "%s:%d: TEST " #no " ERROR: " #lhs " == " format "\n", \
                __FILE__, __LINE__, (lhs));                                    \
    }

int main(int argc, char **argv) {
    // Do some TEST first
#ifdef BUGEN_DEBUG
    bugens_test();
#endif
    srand(time(NULL));
    unsigned int i;
    for (i = 0; i < 10; i++) { syscall(__NR_mm_limit, i, rand()); }
    exit(0);
}
