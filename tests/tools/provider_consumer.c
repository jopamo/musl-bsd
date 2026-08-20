extern int required_symbol(void);
extern int optional_symbol(void) __attribute__((weak));

int call_required_symbol(void) {
    return required_symbol();
}

int call_optional_symbol(void) {
    return optional_symbol == 0 ? 0 : optional_symbol();
}
