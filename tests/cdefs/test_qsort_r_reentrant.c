#include "cdefs.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

struct sort_ctx {
    int tag;
    int nested_calls;
};

static int int_cmp(const void* ap, const void* bp) {
    int a = *(const int*)ap;
    int b = *(const int*)bp;
    return (a > b) - (a < b);
}

static int inner_cmp(const void* ap, const void* bp, void* arg) {
    struct sort_ctx* ctx = (struct sort_ctx*)arg;
    assert(ctx != NULL);
    assert(ctx->tag == 2);
    return int_cmp(ap, bp);
}

static int outer_cmp(const void* ap, const void* bp, void* arg) {
    struct sort_ctx* ctx = (struct sort_ctx*)arg;
    assert(ctx != NULL);
    assert(ctx->tag == 1);

    if (ctx->nested_calls == 0) {
        int inner_vals[] = {7, 1, 9, 3, 2};
        struct sort_ctx inner = {.tag = 2, .nested_calls = 0};
        size_t inner_n = sizeof(inner_vals) / sizeof(inner_vals[0]);

        qsort_r(inner_vals, inner_n, sizeof(inner_vals[0]), inner_cmp, &inner);

        for (size_t i = 1; i < inner_n; i++)
            assert(inner_vals[i - 1] <= inner_vals[i]);

        ctx->nested_calls = 1;
    }

    return int_cmp(ap, bp);
}

int main(void) {
    int vals[] = {8, 4, 6, 2, 7, 3, 1, 5};
    size_t n = sizeof(vals) / sizeof(vals[0]);
    struct sort_ctx outer = {.tag = 1, .nested_calls = 0};

    qsort_r(vals, n, sizeof(vals[0]), outer_cmp, &outer);

    assert(outer.nested_calls == 1);
    for (size_t i = 1; i < n; i++)
        assert(vals[i - 1] <= vals[i]);

    puts("test_qsort_r_reentrant ok");
    return 0;
}
