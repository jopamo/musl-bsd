#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int* __errno_location(void);

enum {
    THREAD_COUNT = 8,
    MAIN_ERRNO = EOWNERDEAD,
};

struct worker {
    pthread_barrier_t* ready;
    pthread_barrier_t* release;
    int* location;
    int expected;
    int failed;
};

#define CHECK(condition)                                                            \
    do {                                                                            \
        if (!(condition)) {                                                         \
            fprintf(stderr, "__errno_location test failed at line %d\n", __LINE__); \
            return 1;                                                               \
        }                                                                           \
    } while (0)

static int wait_at(pthread_barrier_t* barrier) {
    int result = pthread_barrier_wait(barrier);

    return result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD ? 0 : -1;
}

static void* run_worker(void* argument) {
    struct worker* worker = argument;

    worker->location = __errno_location();
    if (worker->location == NULL)
        worker->failed = 1;
    else
        *worker->location = worker->expected;
    if (wait_at(worker->ready) != 0 || wait_at(worker->release) != 0) {
        worker->failed = 1;
        return NULL;
    }
    if (worker->failed)
        return NULL;
    if (__errno_location() != worker->location || errno != worker->expected) {
        worker->failed = 1;
        return NULL;
    }
    errno = 0;
    if (close(-1) != -1 || *worker->location != EBADF)
        worker->failed = 1;
    return NULL;
}

int main(void) {
    int* (*errno_location)(void) = __errno_location;
    struct worker workers[THREAD_COUNT];
    pthread_t threads[THREAD_COUNT];
    pthread_barrier_t ready;
    pthread_barrier_t release;
    int* main_location;
    Dl_info info;
    size_t index;
    size_t previous;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)errno_location, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__errno_location") == (void*)errno_location);

    errno = EDOM;
    main_location = errno_location();
    CHECK(main_location != NULL);
    CHECK(main_location == &errno);
    CHECK(*main_location == EDOM);
    CHECK(errno_location() == main_location);

    *main_location = ERANGE;
    CHECK(errno == ERANGE);
    errno = EILSEQ;
    CHECK(*main_location == EILSEQ);
    errno = 0;
    CHECK(close(-1) == -1);
    CHECK(*main_location == EBADF);

    CHECK(pthread_barrier_init(&ready, NULL, THREAD_COUNT + 1) == 0);
    CHECK(pthread_barrier_init(&release, NULL, THREAD_COUNT + 1) == 0);
    for (index = 0; index < THREAD_COUNT; ++index) {
        workers[index] = (struct worker){
            .ready = &ready,
            .release = &release,
            .expected = 1000 + (int)index,
        };
        CHECK(pthread_create(&threads[index], NULL, run_worker, &workers[index]) == 0);
    }

    *main_location = MAIN_ERRNO;
    CHECK(wait_at(&ready) == 0);
    CHECK(errno_location() == main_location);
    CHECK(*main_location == MAIN_ERRNO);
    for (index = 0; index < THREAD_COUNT; ++index) {
        CHECK(workers[index].location != NULL);
        CHECK(workers[index].location != main_location);
        for (previous = 0; previous < index; ++previous)
            CHECK(workers[index].location != workers[previous].location);
    }
    CHECK(wait_at(&release) == 0);

    for (index = 0; index < THREAD_COUNT; ++index) {
        CHECK(pthread_join(threads[index], NULL) == 0);
        CHECK(!workers[index].failed);
    }
    CHECK(*main_location == MAIN_ERRNO);
    CHECK(pthread_barrier_destroy(&release) == 0);
    CHECK(pthread_barrier_destroy(&ready) == 0);
    return 0;
}
