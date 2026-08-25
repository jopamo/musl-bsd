#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int source_open64(const char* path, int flags) {
    return open64(path, flags);
}

FILE* source_fopen64(const char* path, const char* mode) {
    return fopen64(path, mode);
}

int source_fseeko64(FILE* stream, off64_t offset, int whence) {
    return fseeko64(stream, offset, whence);
}

off64_t source_ftello64(FILE* stream) {
    return ftello64(stream);
}

off64_t source_lseek64(int fd, off64_t offset, int whence) {
    return lseek64(fd, offset, whence);
}

void* source_mmap64(void* addr, size_t length, int prot, int flags, int fd, off64_t offset) {
    return mmap64(addr, length, prot, flags, fd, offset);
}

ssize_t source_pread64(int fd, void* buf, size_t count, off64_t offset) {
    return pread64(fd, buf, count, offset);
}

ssize_t source_pwrite64(int fd, const void* buf, size_t count, off64_t offset) {
    return pwrite64(fd, buf, count, offset);
}

int source_alphasort64(const struct dirent** a, const struct dirent** b) {
    return alphasort64(a, b);
}

int source_scandir64(const char* path, struct dirent*** namelist) {
    return scandir64(path, namelist, NULL, alphasort64);
}
