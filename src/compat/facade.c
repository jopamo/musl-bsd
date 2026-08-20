/*
 * Real glibc-named facade DSO.
 *
 * The facade intentionally contains no compatibility implementation.  Its
 * dependency on the project-owned core makes the implementation available
 * without making the core claim a glibc SONAME.  Facade-owned entry points can
 * be added here as their ABI contracts are qualified.
 */
extern unsigned long musl_bsd_core_abi(void);

__attribute__((visibility("hidden"))) unsigned long musl_bsd_facade_core_abi(void) {
    return musl_bsd_core_abi();
}

#ifdef MUSL_BSD_PROBE_FACADE
unsigned long musl_bsd_glibc_probe(void) {
    return musl_bsd_facade_core_abi();
}
#endif
