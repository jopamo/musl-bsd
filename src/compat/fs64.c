#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdarg.h>

#ifdef stat64
#undef stat64
#endif

#ifdef fstat64
#undef fstat64
#endif

#ifdef lstat64
#undef lstat64
#endif

#ifdef fstatat64
#undef fstatat64
#endif

int open64(const char* path, int oflag, ...) {
    if (oflag & O_CREAT) {
        va_list ap;
        mode_t mode;

        va_start(ap, oflag);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);

        return open(path, oflag, mode);
    }

    return open(path, oflag);
}

FILE* fopen64(const char* path, const char* mode) {
    return fopen(path, mode);
}

off64_t lseek64(int fd, off64_t offset, int whence) {
    return lseek(fd, offset, whence);
}

void* mmap64(void* addr, size_t length, int prot, int flags, int fd, off64_t offset) {
    return mmap(addr, length, prot, flags, fd, offset);
}

ssize_t pread64(int fd, void* buf, size_t count, off64_t offset) {
    return pread(fd, buf, count, offset);
}

ssize_t pwrite64(int fd, const void* buf, size_t count, off64_t offset) {
    return pwrite(fd, buf, count, offset);
}

int alphasort64(const struct dirent** a, const struct dirent** b) {
    return alphasort(a, b);
}

int scandir64(const char* path,
              struct dirent*** namelist,
              int (*filter)(const struct dirent*),
              int (*compar)(const struct dirent**, const struct dirent**)) {
    return scandir(path, namelist, filter, compar);
}

int fseeko64(FILE* stream, off64_t offset, int whence) {
    return fseeko(stream, offset, whence);
}

off64_t ftello64(FILE* stream) {
    return ftello(stream);
}

int stat64(const char* path, struct stat* buf) {
    return stat(path, buf);
}

int lstat64(const char* path, struct stat* buf) {
    return lstat(path, buf);
}

int fstat64(int fd, struct stat* buf) {
    return fstat(fd, buf);
}

int fstatat64(int dfd, const char* path, struct stat* buf, int flags) {
    return fstatat(dfd, path, buf, flags);
}

int __xstat64(int ver, const char* path, struct stat* buf) {
    (void)ver;
    return stat(path, buf);
}

int __lxstat64(int ver, const char* path, struct stat* buf) {
    (void)ver;
    return lstat(path, buf);
}

int __fxstat64(int ver, int fd, struct stat* buf) {
    (void)ver;
    return fstat(fd, buf);
}
