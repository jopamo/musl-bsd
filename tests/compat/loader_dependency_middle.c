extern int loader_dependency_leaf_ready(void);
extern void loader_dependency_trace(const char* event);

static int middle_ready;

void loader_dependency_middle_trace(const char* event) {
    loader_dependency_trace(event);
}

int loader_dependency_middle_ready(void) {
    return middle_ready;
}

__attribute__((constructor)) static void middle_constructor(void) {
    if (loader_dependency_leaf_ready() != 1) {
        middle_ready = -1;
        loader_dependency_trace("dependency-middle-invalid-constructor");
        return;
    }
    loader_dependency_trace("dependency-middle-constructor");
    middle_ready = 1;
}

__attribute__((destructor)) static void middle_destructor(void) {
    loader_dependency_trace(middle_ready == 1 && loader_dependency_leaf_ready() == 1
                                ? "dependency-middle-destructor"
                                : "dependency-middle-invalid-destructor");
    middle_ready = 0;
}
