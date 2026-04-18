#include "obstack_test_common.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static char* setup_two_chunk_obstack(struct obstack* ob, struct _obstack_chunk** first_chunk_out) {
    int ok = _obstack_begin(ob, 64, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    char* first = obstack_build_string(ob, 40, 'a');
    struct _obstack_chunk* first_chunk = ob->chunk;
    assert(first_chunk != NULL);
    assert((char*)first > (char*)first_chunk);
    assert((char*)first < first_chunk->limit);

    (void)obstack_build_string(ob, 300, 'b');
    assert(ob->chunk != first_chunk);

    if (first_chunk_out)
        *first_chunk_out = first_chunk;
    return first;
}

static void test_mid_object_free_rewinds_across_chunks(void) {
    struct obstack ob;
    char* first = setup_two_chunk_obstack(&ob, NULL);

    char* rewind = first + 13;
    _obstack_free(&ob, rewind);
    assert(ob.object_base == rewind);
    assert(ob.next_free == rewind);

    obstack_grow(&ob, "MID", 4);
    char* out = (char*)obstack_finish(&ob);
    assert(strcmp(out, "MID") == 0);

    _obstack_free(&ob, NULL);
}

static void test_object_boundary_free_rewinds_across_chunks(void) {
    struct obstack ob;
    char* first = setup_two_chunk_obstack(&ob, NULL);

    _obstack_free(&ob, first);
    assert(ob.object_base == first);
    assert(ob.next_free == first);

    char* out = obstack_build_string(&ob, 5, 'c');
    assert(strcmp(out, "ccccc") == 0);

    _obstack_free(&ob, NULL);
}

static void test_null_free_resets_and_empty_free_is_noop(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    (void)obstack_build_string(&ob, 16, 'r');
    ob.maybe_empty_object = 0;
    ob.alloc_failed = 1;

    _obstack_free(&ob, NULL);
    assert(ob.chunk == NULL);
    assert(ob.chunk_limit == NULL);
    assert(ob.object_base == NULL);
    assert(ob.next_free == NULL);
    assert(ob.maybe_empty_object == 1);
    assert(ob.alloc_failed == 0);

    _obstack_free(&ob, NULL);
    assert(ob.chunk == NULL);
    assert(ob.object_base == NULL);
    assert(ob.next_free == NULL);
}

static void test_outside_pointer_aborts(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);
    (void)obstack_build_string(&ob, 32, 'z');

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        int stack_var = 0;
        _obstack_free(&ob, &stack_var);
        _exit(0);
    }

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGABRT);

    _obstack_free(&ob, NULL);
}

int main(void) {
    test_mid_object_free_rewinds_across_chunks();
    test_object_boundary_free_rewinds_across_chunks();
    test_null_free_resets_and_empty_free_is_noop();
    test_outside_pointer_aborts();
    puts("test_free_edges ok");
    return 0;
}
