void test_obstack_begin() {
    struct obstack obs;
    int result = obstack_init(&obs);
    assert(result == 1);
    assert(obs.chunk != NULL);
    obstack_free(&obs, NULL);
    printf("obstack_begin passed\n");
}

void test_obstack_newchunk() {
    struct obstack obs;
    obstack_init(&obs);

    void* initial_base = obs.object_base;
    _obstack_newchunk(&obs, 1024);
    assert(obs.object_base != initial_base);
    obstack_free(&obs, NULL);
    printf("_obstack_newchunk passed\n");
}

void test_obstack_free() {
    struct obstack obs;
    obstack_init(&obs);

    int* data = (int*)obstack_alloc(&obs, sizeof(int));
    *data = 42;

    obstack_free(&obs, data);
    assert(obs.chunk != NULL);
    printf("obstack_free passed\n");
}

void test_obstack_printf() {
    struct obstack obs;
    obstack_init(&obs);

    int result = obstack_printf(&obs, "Hello, %s!", "world");
    assert(result > 0);

    size_t size = obstack_calculate_object_size(&obs);
    assert(size > 0);
    obstack_free(&obs, NULL);
    printf("obstack_printf passed\n");
}

void test_obstack_memory_used() {
    struct obstack obs;
    obstack_init(&obs);

    int* data1 = (int*)obstack_alloc(&obs, 128);
    *data1 = 1;
    int* data2 = (int*)obstack_alloc(&obs, 256);
    *data2 = 2;

    _OBSTACK_SIZE_T memory_used = obstack_memory_used(&obs);
    assert(memory_used >= 128 + 256);
    obstack_free(&obs, NULL);
    printf("obstack_memory_used passed\n");
}

int main() {
    test_obstack_begin();
    test_obstack_newchunk();
    test_obstack_free();
    test_obstack_printf();
    test_obstack_memory_used();

    printf("All tests passed!\n");
    return 0;
}
