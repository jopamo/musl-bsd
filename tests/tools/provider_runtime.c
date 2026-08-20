extern int optional_symbol(void) __attribute__((weak));

int provided_symbol(void) {
    return 7;
}

int call_runtime_optional(void) {
    return optional_symbol == 0 ? 0 : optional_symbol();
}
