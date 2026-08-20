extern int loader_versioned_symbol(void);

int loader_versioned_value(void) {
    return loader_versioned_symbol();
}
