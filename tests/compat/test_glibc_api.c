#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <gnu/libc-version.h>
#include <locale.h>
#include <link.h>
#include <malloc.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

extern int __pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
extern cpu_set_t* __sched_cpualloc(size_t count);
extern void __sched_cpufree(cpu_set_t* set);
extern char* __realpath_chk(const char* path, char* resolved_path, size_t resolved_len);
extern char* __strdup(const char* string);
extern char* __strtok_r(char* s, const char* delim, char** save_ptr);
extern int dladdr1(const void* address, Dl_info* info, void** extra_info, int flags);
extern int backtrace(void** buffer, int size);
extern int __xstat64(int ver, const char* path, struct stat64* buf);
extern size_t __strftime_l(char* restrict s,
                           size_t n,
                           const char* restrict format,
                           const struct tm* restrict tm,
                           locale_t locale);

#ifndef RTLD_DL_LINKMAP
#define RTLD_DL_LINKMAP 2
#endif

#ifndef RTLD_DL_SYMENT
#define RTLD_DL_SYMENT 1
#endif

#if defined(__x86_64__) && __SIZEOF_POINTER__ == 8
typedef char glibc_dl_info_size[(sizeof(Dl_info) == 32) ? 1 : -1];
typedef char glibc_dl_info_align[(_Alignof(Dl_info) == 8) ? 1 : -1];
typedef char glibc_dl_info_fname_offset[(offsetof(Dl_info, dli_fname) == 0) ? 1 : -1];
typedef char glibc_dl_info_fbase_offset[(offsetof(Dl_info, dli_fbase) == 8) ? 1 : -1];
typedef char glibc_dl_info_sname_offset[(offsetof(Dl_info, dli_sname) == 16) ? 1 : -1];
typedef char glibc_dl_info_saddr_offset[(offsetof(Dl_info, dli_saddr) == 24) ? 1 : -1];
typedef char glibc_link_map_size[(sizeof(struct link_map) == 40) ? 1 : -1];
typedef char glibc_link_map_align[(_Alignof(struct link_map) == 8) ? 1 : -1];
typedef char glibc_link_map_addr_offset[(offsetof(struct link_map, l_addr) == 0) ? 1 : -1];
typedef char glibc_link_map_name_offset[(offsetof(struct link_map, l_name) == 8) ? 1 : -1];
typedef char glibc_lmid_size[(sizeof(Lmid_t) == 8) ? 1 : -1];
typedef char glibc_lmid_align[(_Alignof(Lmid_t) == 8) ? 1 : -1];
#endif

#define CHECK(cond)   \
    do {              \
        if (!(cond))  \
            return 1; \
    } while (0)

static int check_link_map(const void* address) {
    struct link_map* map;
    Dl_info info;
    void* extra = NULL;

    memset(&info, 0, sizeof(info));
    errno = EDOM;
    if (dladdr1(address, &info, &extra, RTLD_DL_LINKMAP) == 0 || errno != EDOM || info.dli_fname == NULL ||
        info.dli_fbase == NULL || extra == NULL)
        return 1;
    map = extra;
    if (map->l_addr != (ElfW(Addr))info.dli_fbase)
        return 1;
    return 0;
}

static int test_dladdr1(void) {
    Dl_info info;
    void* sentinel = (void*)(uintptr_t)1;
    void* extra;

    if (check_link_map((const void*)&test_dladdr1) != 0 || check_link_map((const void*)&malloc) != 0)
        return 1;

    memset(&info, 0, sizeof(info));
    extra = sentinel;
    errno = ERANGE;
    if (dladdr1((const void*)&malloc, &info, &extra, RTLD_DL_SYMENT) != 0 || errno != ERANGE || extra != sentinel)
        return 1;

    errno = EDOM;
    if (dladdr1((const void*)&malloc, &info, NULL, RTLD_DL_LINKMAP) != 0 || errno != EDOM)
        return 1;

    memset(&info, 0, sizeof(info));
    extra = sentinel;
    errno = ERANGE;
    if (dladdr1((const void*)(uintptr_t)1, &info, &extra, RTLD_DL_LINKMAP) != 0 || errno != ERANGE || extra != sentinel)
        return 1;
    return 0;
}

static int expect_dlvsym_failure(const char* symbol, const char* version) {
    const char* error;
    void* address;

    dlerror();
    errno = ERANGE;
    address = dlvsym(RTLD_DEFAULT, symbol, version);
    error = dlerror();
    if (address != NULL || error == NULL || errno != ERANGE || dlerror() != NULL)
        return 1;
    return 0;
}

static int test_dlvsym(void) {
    static const char* const accepted_versions[] = {
        "GLIBC_2.2.5", "GLIBC_2.3", "GLIBC_2.3.2", "GLIBC_2.3.3", "GLIBC_2.3.4", "GLIBC_2.4",  "GLIBC_2.6",
        "GLIBC_2.7",   "GLIBC_2.9", "GLIBC_2.10",  "GLIBC_2.12",  "GLIBC_2.14",  "GLIBC_2.16", "GLIBC_2.17",
    };

    for (size_t index = 0; index < sizeof(accepted_versions) / sizeof(accepted_versions[0]); ++index) {
        void* address;

        dlerror();
        errno = EDOM;
        address = dlvsym(RTLD_DEFAULT, "malloc", accepted_versions[index]);
        if (address != (void*)&malloc || dlerror() != NULL || errno != EDOM)
            return 1;
    }

    if (expect_dlvsym_failure("malloc", "GLIBC_2.999") != 0 || expect_dlvsym_failure("malloc", "GLIBC_PRIVATE") != 0 ||
        expect_dlvsym_failure("malloc", NULL) != 0 ||
        expect_dlvsym_failure("musl_bsd_missing_symbol", "GLIBC_2.2.5") != 0)
        return 1;
    return 0;
}

static int expect_dlmopen_failure(Lmid_t lmid) {
    const char* error;
    void* handle;

    dlerror();
    errno = ERANGE;
    handle = dlmopen(lmid, NULL, RTLD_NOW);
    error = dlerror();
    if (handle != NULL || error == NULL || errno != ERANGE || dlerror() != NULL)
        return 1;
    return 0;
}

static int test_dlmopen(void) {
    void* expected;
    void* handle;
    void* symbol;

    if (LM_ID_BASE != (Lmid_t)0 || LM_ID_NEWLM != (Lmid_t)-1)
        return 1;
    dlerror();
    errno = EDOM;
    handle = dlmopen(LM_ID_BASE, NULL, RTLD_NOW);
    if (handle == NULL || dlerror() != NULL || errno != EDOM)
        return 1;
    symbol = dlsym(handle, "malloc");
    expected = dlsym(RTLD_DEFAULT, "malloc");
    if (symbol == NULL || symbol != expected || dlclose(handle) != 0)
        return 1;

    if (expect_dlmopen_failure(LM_ID_NEWLM) != 0 || expect_dlmopen_failure((Lmid_t)1) != 0)
        return 1;
    return 0;
}

int main(void) {
    char buf[PATH_MAX];
    char tmpdir_template[] = "/tmp/musl-bsd-glibc-XXXXXX";
    char path_a[PATH_MAX];
    char path_b[PATH_MAX];
    char* tmpdir;
    char* dup;
    char* tok_state = NULL;
    char toks[] = "a:b";
    char datebuf[64];
    struct stat64 st;
    struct dirent** entries = NULL;
    struct mallinfo info;
    FILE* fp;
    int fd;
    int n;
    void* map;
    locale_t c_locale;
    void* frames[4];
    pthread_key_t key;

    CHECK(gnu_get_libc_release()[0] != '\0');
    CHECK(gnu_get_libc_version()[0] != '\0');
    CHECK(test_dladdr1() == 0);
    n = backtrace(frames, 4);
    CHECK(n > 0 && n <= 4);

    CHECK(test_dlmopen() == 0);
    CHECK(test_dlvsym() == 0);

    CHECK(__pthread_key_create(&key, NULL) == 0);
    CHECK(pthread_key_delete(key) == 0);

    info = mallinfo();
    CHECK(info.arena == 0);
    CHECK(malloc_trim(0) == 0);
    mtrace();
    muntrace();

    CHECK(setenv("MUSL_BSD_TEST_ENV", "ok", 1) == 0);
    CHECK(secure_getenv("MUSL_BSD_TEST_ENV") != NULL);

    CHECK(__realpath_chk(".", buf, sizeof(buf)) != NULL);

    dup = __strdup("alpha");
    CHECK(dup != NULL);
    CHECK(strcmp(dup, "alpha") == 0);
    free(dup);

    CHECK(strcmp(__strtok_r(toks, ":", &tok_state), "a") == 0);
    CHECK(strcmp(__strtok_r(NULL, ":", &tok_state), "b") == 0);

    c_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    memset(datebuf, 0, sizeof(datebuf));
    CHECK(__strftime_l(datebuf, sizeof(datebuf), "%Y", &(struct tm){.tm_year = 124}, c_locale) == 4);
    CHECK(strcmp(datebuf, "2024") == 0);
    freelocale(c_locale);

    tmpdir = mkdtemp(tmpdir_template);
    CHECK(tmpdir != NULL);

    CHECK(snprintf(path_a, sizeof(path_a), "%s/a", tmpdir) > 0);
    CHECK(snprintf(path_b, sizeof(path_b), "%s/b", tmpdir) > 0);

    fd = open64(path_a, O_CREAT | O_RDWR | O_TRUNC, 0600);
    CHECK(fd >= 0);
    CHECK(pwrite64(fd, "hello", 5, 0) == 5);
    CHECK(lseek64(fd, 0, SEEK_SET) == 0);
    memset(buf, 0, sizeof(buf));
    CHECK(pread64(fd, buf, 5, 0) == 5);
    CHECK(strcmp(buf, "hello") == 0);
    CHECK(close(fd) == 0);

    CHECK(__xstat64(0, path_a, &st) == 0);
    CHECK(S_ISREG(st.st_mode));
    CHECK(symlink(path_a, path_b) == 0);

    fp = fopen64(path_a, "r+");
    CHECK(fp != NULL);
    CHECK(fseeko64(fp, 2, SEEK_SET) == 0);
    CHECK(ftello64(fp) == 2);
    fclose(fp);

    map = mmap64(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(map != MAP_FAILED);
    memset(map, 0x5a, 4096);
    CHECK(munmap(map, 4096) == 0);

    fd = open64(path_b, O_CREAT | O_RDWR | O_TRUNC, 0600);
    CHECK(fd >= 0);
    CHECK(close(fd) == 0);

    n = scandir64(tmpdir, &entries, NULL, alphasort64);
    CHECK(n >= 2);
    for (int i = 0; i < n; ++i)
        free(entries[i]);
    free(entries);

    entries = NULL;

#ifdef CPU_ALLOC
    {
        cpu_set_t* set = __sched_cpualloc(256);
        CHECK(set != NULL);
        __sched_cpufree(set);
    }
#endif

    CHECK(unlink(path_a) == 0);
    CHECK(unlink(path_b) == 0);
    CHECK(rmdir(tmpdir) == 0);

    return 0;
}
