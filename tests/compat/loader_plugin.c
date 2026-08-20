extern int loader_dependency_middle_ready(void);
extern void loader_dependency_middle_trace(const char* event);

static int plugin_ready;

__attribute__((constructor)) static void plugin_constructor(void) {
    if (loader_dependency_middle_ready() != 1) {
        plugin_ready = -1;
        loader_dependency_middle_trace("plugin-invalid-constructor");
        return;
    }
    loader_dependency_middle_trace("plugin-constructor");
    plugin_ready = 1;
}

__attribute__((destructor)) static void plugin_destructor(void) {
    loader_dependency_middle_trace(
        plugin_ready == 1 && loader_dependency_middle_ready() == 1 ? "plugin-destructor" : "plugin-invalid-destructor");
    plugin_ready = 0;
}

int loader_plugin_entry(void) {
    if (plugin_ready != 1 || loader_dependency_middle_ready() != 1)
        return -1;
    loader_dependency_middle_trace("plugin-entry");
    return 73;
}
