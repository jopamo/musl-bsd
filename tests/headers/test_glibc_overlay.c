#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <gnu/libc-version.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    Lmid_t ns = LM_ID_BASE;
    struct mallinfo info = mallinfo();
    struct stat64 st;
    struct dirent64* de = NULL;
    FILE* (*openf)(const char*, const char*) = fopen64;
    int (*openi)(const char*, int, ...) = open64;
    void* (*mapf)(void*, size_t, int, int, int, off64_t) = mmap64;
    off64_t (*seekf)(int, off64_t, int) = lseek64;
    ssize_t (*preadf)(int, void*, size_t, off64_t) = pread64;
    ssize_t (*pwritef)(int, const void*, size_t, off64_t) = pwrite64;
    int (*scanf)(const char*, struct dirent64***, int (*)(const struct dirent64*),
                 int (*)(const struct dirent64**, const struct dirent64**)) = scandir64;
    int (*sortf)(const struct dirent64**, const struct dirent64**) = alphasort64;

    (void)ns;
    (void)info;
    (void)st;
    (void)de;
    (void)openf;
    (void)openi;
    (void)mapf;
    (void)seekf;
    (void)preadf;
    (void)pwritef;
    (void)scanf;
    (void)sortf;
    (void)dlmopen;
    (void)dlvsym;
    (void)gnu_get_libc_release;
    (void)gnu_get_libc_version;
    (void)secure_getenv;

    return 0;
}
