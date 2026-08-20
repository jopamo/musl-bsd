extern int loader_weak_provider(void) __attribute__((weak));

int loader_weak_value(void) {
    return loader_weak_provider == 0 ? 41 : loader_weak_provider();
}
