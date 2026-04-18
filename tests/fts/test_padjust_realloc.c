#include <fts.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEPTH 90
#define SEGMENT_LEN 100

void* __real_realloc(void* ptr, size_t size);

/* Force successful realloc(ptr, size) to return a moved address so that the
 * fts_palloc -> fts_padjust path is deterministic in this test binary. */
void* __wrap_realloc(void* ptr, size_t size) {
    void* r = __real_realloc(ptr, size);
    if (!ptr || !r || size == 0)
        return r;

    void* moved = malloc(size);
    if (!moved)
        return r;

    memcpy(moved, r, size);
    free(r);
    return moved;
}

static void make_segment_name(int i, char out[SEGMENT_LEN + 1]) {
    snprintf(out, SEGMENT_LEN + 1, "d%03d_", i);
    size_t used = strlen(out);
    memset(out + used, 'a' + (i % 26), SEGMENT_LEN - used);
    out[SEGMENT_LEN] = '\0';
}

int main(void) {
    char template[] = "/tmp/fts-padjust-XXXXXX";
    char* root = mkdtemp(template);
    assert(root != NULL);

    char names[DEPTH][SEGMENT_LEN + 1];

    int saved_cwd = open(".", O_RDONLY | O_DIRECTORY);
    assert(saved_cwd >= 0);

    assert(chdir(root) == 0);
    for (int i = 0; i < DEPTH; ++i) {
        make_segment_name(i, names[i]);
        assert(mkdir(names[i], 0700) == 0);
        assert(chdir(names[i]) == 0);
    }
    assert(fchdir(saved_cwd) == 0);

    char* roots[] = {root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL, NULL);
    assert(f != NULL);

    int saw_deep = 0;
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        if (e->fts_level >= DEPTH)
            saw_deep = 1;
    }
    assert(saw_deep);
    assert(fts_close(f) == 0);

    assert(chdir(root) == 0);
    for (int i = 0; i < DEPTH; ++i)
        assert(chdir(names[i]) == 0);

    for (int i = DEPTH - 1; i >= 0; --i) {
        assert(chdir("..") == 0);
        assert(rmdir(names[i]) == 0);
    }

    assert(fchdir(saved_cwd) == 0);
    close(saved_cwd);

    assert(rmdir(root) == 0);
    return 0;
}
