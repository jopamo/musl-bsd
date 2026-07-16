#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <gnu/libc-version.h>
#include <locale.h>
#include <malloc.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <limits.h>

extern int __register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void* dso_handle);
extern cpu_set_t* __sched_cpualloc(size_t count);
extern void __sched_cpufree(cpu_set_t* set);
extern char* __realpath_chk(const char* path, char* resolved_path, size_t resolved_len);
extern char* __secure_getenv(const char* name);
extern char* __strdup(const char* string);
extern char* __strtok_r(char* s, const char* delim, char** save_ptr);
extern int __xstat64(int ver, const char* path, struct stat64* buf);
extern int __lxstat64(int ver, const char* path, struct stat64* buf);
extern int __fxstat64(int ver, int fd, struct stat64* buf);
extern size_t __strftime_l(char* restrict s,
                           size_t n,
                           const char* restrict format,
                           const struct tm* restrict tm,
                           locale_t locale);

#define CHECK(cond)   \
    do {              \
        if (!(cond))  \
            return 1; \
    } while (0)

static void noop(void) {}

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
    void* sym;
    locale_t c_locale;

    CHECK(gnu_get_libc_release()[0] != '\0');
    CHECK(gnu_get_libc_version()[0] != '\0');

    sym = dlmopen(LM_ID_BASE, NULL, RTLD_NOW);
    CHECK(sym != NULL);
    sym = dlvsym(RTLD_DEFAULT, "malloc", "GLIBC_2.2.5");
    CHECK(sym != NULL);

    CHECK(__register_atfork(noop, noop, noop, NULL) == 0);

    info = mallinfo();
    CHECK(info.arena == 0);
    CHECK(malloc_trim(0) == 0);
    mtrace();
    muntrace();

    CHECK(setenv("MUSL_BSD_TEST_ENV", "ok", 1) == 0);
    CHECK(secure_getenv("MUSL_BSD_TEST_ENV") != NULL);
    CHECK(__secure_getenv("MUSL_BSD_TEST_ENV") != NULL);

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
    CHECK(__fxstat64(0, fd, &st) == 0);
    CHECK(S_ISREG(st.st_mode));
    CHECK(close(fd) == 0);

    CHECK(__xstat64(0, path_a, &st) == 0);
    CHECK(S_ISREG(st.st_mode));
    CHECK(symlink(path_a, path_b) == 0);
    CHECK(__lxstat64(0, path_b, &st) == 0);
    CHECK(S_ISLNK(st.st_mode));

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
