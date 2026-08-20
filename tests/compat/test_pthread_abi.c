#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

/*
 * A glibc development host defaults several pthread symbols to GLIBC_2.34.
 * Bind this foreign-ABI fixture to the versions exported by the bridge.
 */
#if defined(__GLIBC__)
#define OLD_SYMBOL(local, symbol, version, declaration) \
	extern declaration; \
	__asm__(".symver " #local "," #symbol "@" version)

OLD_SYMBOL(compat_mutexattr_init, pthread_mutexattr_init, "GLIBC_2.2.5",
	   int compat_mutexattr_init(pthread_mutexattr_t *));
OLD_SYMBOL(compat_mutexattr_destroy, pthread_mutexattr_destroy, "GLIBC_2.2.5",
	   int compat_mutexattr_destroy(pthread_mutexattr_t *));
OLD_SYMBOL(compat_mutexattr_settype, pthread_mutexattr_settype, "GLIBC_2.2.5",
	   int compat_mutexattr_settype(pthread_mutexattr_t *, int));
OLD_SYMBOL(compat_mutexattr_setpshared, pthread_mutexattr_setpshared,
	   "GLIBC_2.2.5",
	   int compat_mutexattr_setpshared(pthread_mutexattr_t *, int));
OLD_SYMBOL(compat_mutexattr_setprotocol, pthread_mutexattr_setprotocol,
	   "GLIBC_2.4",
	   int compat_mutexattr_setprotocol(pthread_mutexattr_t *, int));
OLD_SYMBOL(compat_mutex_init, pthread_mutex_init, "GLIBC_2.2.5",
	   int compat_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *));
OLD_SYMBOL(compat_mutex_destroy, pthread_mutex_destroy, "GLIBC_2.2.5",
	   int compat_mutex_destroy(pthread_mutex_t *));
OLD_SYMBOL(compat_mutex_lock, pthread_mutex_lock, "GLIBC_2.2.5",
	   int compat_mutex_lock(pthread_mutex_t *));
OLD_SYMBOL(compat_mutex_trylock, pthread_mutex_trylock, "GLIBC_2.2.5",
	   int compat_mutex_trylock(pthread_mutex_t *));
OLD_SYMBOL(compat_mutex_unlock, pthread_mutex_unlock, "GLIBC_2.2.5",
	   int compat_mutex_unlock(pthread_mutex_t *));

OLD_SYMBOL(compat_condattr_init, pthread_condattr_init, "GLIBC_2.2.5",
	   int compat_condattr_init(pthread_condattr_t *));
OLD_SYMBOL(compat_condattr_destroy, pthread_condattr_destroy, "GLIBC_2.2.5",
	   int compat_condattr_destroy(pthread_condattr_t *));
OLD_SYMBOL(compat_condattr_setpshared, pthread_condattr_setpshared,
	   "GLIBC_2.2.5",
	   int compat_condattr_setpshared(pthread_condattr_t *, int));
OLD_SYMBOL(compat_cond_init, pthread_cond_init, "GLIBC_2.3.2",
	   int compat_cond_init(pthread_cond_t *, const pthread_condattr_t *));
OLD_SYMBOL(compat_cond_destroy, pthread_cond_destroy, "GLIBC_2.3.2",
	   int compat_cond_destroy(pthread_cond_t *));
OLD_SYMBOL(compat_cond_wait, pthread_cond_wait, "GLIBC_2.3.2",
	   int compat_cond_wait(pthread_cond_t *, pthread_mutex_t *));
OLD_SYMBOL(compat_cond_timedwait, pthread_cond_timedwait, "GLIBC_2.3.2",
	   int compat_cond_timedwait(pthread_cond_t *, pthread_mutex_t *,
				     const struct timespec *));
OLD_SYMBOL(compat_cond_signal, pthread_cond_signal, "GLIBC_2.3.2",
	   int compat_cond_signal(pthread_cond_t *));
OLD_SYMBOL(compat_cond_broadcast, pthread_cond_broadcast, "GLIBC_2.3.2",
	   int compat_cond_broadcast(pthread_cond_t *));

OLD_SYMBOL(compat_rwlockattr_init, pthread_rwlockattr_init, "GLIBC_2.2.5",
	   int compat_rwlockattr_init(pthread_rwlockattr_t *));
OLD_SYMBOL(compat_rwlockattr_destroy, pthread_rwlockattr_destroy,
	   "GLIBC_2.2.5",
	   int compat_rwlockattr_destroy(pthread_rwlockattr_t *));
OLD_SYMBOL(compat_rwlockattr_setpshared, pthread_rwlockattr_setpshared,
	   "GLIBC_2.2.5",
	   int compat_rwlockattr_setpshared(pthread_rwlockattr_t *, int));
OLD_SYMBOL(compat_rwlock_init, pthread_rwlock_init, "GLIBC_2.2.5",
	   int compat_rwlock_init(pthread_rwlock_t *,
				  const pthread_rwlockattr_t *));
OLD_SYMBOL(compat_rwlock_destroy, pthread_rwlock_destroy, "GLIBC_2.2.5",
	   int compat_rwlock_destroy(pthread_rwlock_t *));
OLD_SYMBOL(compat_rwlock_rdlock, pthread_rwlock_rdlock, "GLIBC_2.2.5",
	   int compat_rwlock_rdlock(pthread_rwlock_t *));
OLD_SYMBOL(compat_rwlock_wrlock, pthread_rwlock_wrlock, "GLIBC_2.2.5",
	   int compat_rwlock_wrlock(pthread_rwlock_t *));
OLD_SYMBOL(compat_rwlock_unlock, pthread_rwlock_unlock, "GLIBC_2.2.5",
	   int compat_rwlock_unlock(pthread_rwlock_t *));
OLD_SYMBOL(compat_rwlock_tryrdlock, pthread_rwlock_tryrdlock, "GLIBC_2.2.5",
	   int compat_rwlock_tryrdlock(pthread_rwlock_t *));
OLD_SYMBOL(compat_rwlock_trywrlock, pthread_rwlock_trywrlock, "GLIBC_2.2.5",
	   int compat_rwlock_trywrlock(pthread_rwlock_t *));

OLD_SYMBOL(compat_once, pthread_once, "GLIBC_2.2.5",
	   int compat_once(pthread_once_t *, void (*)(void)));

OLD_SYMBOL(compat_attr_init, pthread_attr_init, "GLIBC_2.2.5",
	   int compat_attr_init(pthread_attr_t *));
OLD_SYMBOL(compat_attr_destroy, pthread_attr_destroy, "GLIBC_2.2.5",
	   int compat_attr_destroy(pthread_attr_t *));
OLD_SYMBOL(compat_attr_setstack, pthread_attr_setstack, "GLIBC_2.2.5",
	   int compat_attr_setstack(pthread_attr_t *, void *, size_t));
OLD_SYMBOL(compat_attr_setstacksize, pthread_attr_setstacksize, "GLIBC_2.2.5",
	   int compat_attr_setstacksize(pthread_attr_t *, size_t));
OLD_SYMBOL(compat_create, pthread_create, "GLIBC_2.2.5",
	   int compat_create(pthread_t *, const pthread_attr_t *,
			     void *(*)(void *), void *));
OLD_SYMBOL(compat_join, pthread_join, "GLIBC_2.2.5",
	   int compat_join(pthread_t, void **));
OLD_SYMBOL(compat_detach, pthread_detach, "GLIBC_2.2.5",
	   int compat_detach(pthread_t));
OLD_SYMBOL(compat_internal_key_create, __pthread_key_create, "GLIBC_2.2.5",
	   int compat_internal_key_create(pthread_key_t *, void (*)(void *)));

OLD_SYMBOL(compat_sem_init, sem_init, "GLIBC_2.2.5",
	   int compat_sem_init(sem_t *, int, unsigned int));
OLD_SYMBOL(compat_sem_destroy, sem_destroy, "GLIBC_2.2.5",
	   int compat_sem_destroy(sem_t *));
OLD_SYMBOL(compat_sem_post, sem_post, "GLIBC_2.2.5",
	   int compat_sem_post(sem_t *));
OLD_SYMBOL(compat_sem_timedwait, sem_timedwait, "GLIBC_2.2.5",
	   int compat_sem_timedwait(sem_t *, const struct timespec *));
OLD_SYMBOL(compat_sem_trywait, sem_trywait, "GLIBC_2.2.5",
	   int compat_sem_trywait(sem_t *));
OLD_SYMBOL(compat_sem_wait, sem_wait, "GLIBC_2.2.5",
	   int compat_sem_wait(sem_t *));

#define pthread_mutexattr_init compat_mutexattr_init
#define pthread_mutexattr_destroy compat_mutexattr_destroy
#define pthread_mutexattr_settype compat_mutexattr_settype
#define pthread_mutexattr_setpshared compat_mutexattr_setpshared
#define pthread_mutexattr_setprotocol compat_mutexattr_setprotocol
#define pthread_mutex_init compat_mutex_init
#define pthread_mutex_destroy compat_mutex_destroy
#define pthread_mutex_lock compat_mutex_lock
#define pthread_mutex_trylock compat_mutex_trylock
#define pthread_mutex_unlock compat_mutex_unlock
#define pthread_condattr_init compat_condattr_init
#define pthread_condattr_destroy compat_condattr_destroy
#define pthread_condattr_setpshared compat_condattr_setpshared
#define pthread_cond_init compat_cond_init
#define pthread_cond_destroy compat_cond_destroy
#define pthread_cond_wait compat_cond_wait
#define pthread_cond_timedwait compat_cond_timedwait
#define pthread_cond_signal compat_cond_signal
#define pthread_cond_broadcast compat_cond_broadcast
#define pthread_rwlockattr_init compat_rwlockattr_init
#define pthread_rwlockattr_destroy compat_rwlockattr_destroy
#define pthread_rwlockattr_setpshared compat_rwlockattr_setpshared
#define pthread_rwlock_init compat_rwlock_init
#define pthread_rwlock_destroy compat_rwlock_destroy
#define pthread_rwlock_rdlock compat_rwlock_rdlock
#define pthread_rwlock_wrlock compat_rwlock_wrlock
#define pthread_rwlock_unlock compat_rwlock_unlock
#define pthread_rwlock_tryrdlock compat_rwlock_tryrdlock
#define pthread_rwlock_trywrlock compat_rwlock_trywrlock
#define pthread_once compat_once
#define pthread_attr_init compat_attr_init
#define pthread_attr_destroy compat_attr_destroy
#define pthread_attr_setstack compat_attr_setstack
#define pthread_attr_setstacksize compat_attr_setstacksize
#define pthread_create compat_create
#define pthread_join compat_join
#define pthread_detach compat_detach
#define __pthread_key_create compat_internal_key_create
#define sem_init compat_sem_init
#define sem_destroy compat_sem_destroy
#define sem_post compat_sem_post
#define sem_timedwait compat_sem_timedwait
#define sem_trywait compat_sem_trywait
#define sem_wait compat_sem_wait
#endif

#define THREAD_COUNT 24

#if !defined(__GLIBC__)
extern int __pthread_key_create(pthread_key_t *, void (*)(void *))
	__attribute__((weak));
#endif

union glibc_mutex_storage {
	uint64_t align;
	unsigned int words[10];
};

static pthread_mutex_t resolver_mutexes[THREAD_COUNT];
static int resolver_go;
static union glibc_mutex_storage concurrent_static_mutex;
static int concurrent_static_go;

static int resolver_worker(void *argument)
{
	uintptr_t index = (uintptr_t)argument;

	while (!__atomic_load_n(&resolver_go, __ATOMIC_ACQUIRE))
		thrd_yield();
	if (pthread_mutex_lock(&resolver_mutexes[index]) != 0)
		return 1;
	if (pthread_mutex_unlock(&resolver_mutexes[index]) != 0)
		return 2;
	return 0;
}

static int test_concurrent_resolver(void)
{
	thrd_t threads[THREAD_COUNT];
	int result;
	int i;

	for (i = 0; i < THREAD_COUNT; i++)
		if (thrd_create(&threads[i], resolver_worker,
				(void *)(uintptr_t)i) != thrd_success)
			return __LINE__;
	__atomic_store_n(&resolver_go, 1, __ATOMIC_RELEASE);
	for (i = 0; i < THREAD_COUNT; i++) {
		if (thrd_join(threads[i], &result) != thrd_success || result)
			return __LINE__;
	}
	return 0;
}

static int static_mutex_worker(void *argument)
{
	pthread_mutex_t *mutex =
		(pthread_mutex_t *)&concurrent_static_mutex;

	(void)argument;
	while (!__atomic_load_n(&concurrent_static_go, __ATOMIC_ACQUIRE))
		thrd_yield();
	if (pthread_mutex_lock(mutex) != 0)
		return 1;
	if (pthread_mutex_unlock(mutex) != 0)
		return 2;
	return 0;
}

static int test_concurrent_static_mutex(void)
{
	pthread_mutex_t *mutex =
		(pthread_mutex_t *)&concurrent_static_mutex;
	thrd_t threads[THREAD_COUNT];
	int result;
	int i;

	memset(&concurrent_static_mutex, 0, sizeof(concurrent_static_mutex));
	concurrent_static_mutex.words[4] = 1;
	for (i = 0; i < THREAD_COUNT; i++)
		if (thrd_create(&threads[i], static_mutex_worker, NULL) !=
		    thrd_success)
			return __LINE__;
	__atomic_store_n(&concurrent_static_go, 1, __ATOMIC_RELEASE);
	for (i = 0; i < THREAD_COUNT; i++)
		if (thrd_join(threads[i], &result) != thrd_success || result)
			return __LINE__;
	return pthread_mutex_destroy(mutex) == 0 ? 0 : __LINE__;
}

static int test_static_mutexes(void)
{
	union glibc_mutex_storage storage;
	pthread_mutex_t *mutex = (pthread_mutex_t *)&storage;
	int result;

	memset(&storage, 0, sizeof(storage));
	if (pthread_mutex_lock(mutex) != 0)
		return __LINE__;
	if (pthread_mutex_trylock(mutex) != EBUSY)
		return __LINE__;
	if (pthread_mutex_unlock(mutex) != 0 ||
	    pthread_mutex_destroy(mutex) != 0)
		return __LINE__;

	memset(&storage, 0, sizeof(storage));
	storage.words[4] = 1;
	if (pthread_mutex_lock(mutex) != 0 ||
	    pthread_mutex_lock(mutex) != 0 ||
	    pthread_mutex_unlock(mutex) != 0 ||
	    pthread_mutex_unlock(mutex) != 0 ||
	    pthread_mutex_destroy(mutex) != 0)
		return __LINE__;

	/* Destroyed recursive storage reused at the same address as normal. */
	memset(&storage, 0, sizeof(storage));
	if (pthread_mutex_trylock(mutex) != 0)
		return __LINE__;
	if (pthread_mutex_trylock(mutex) != EBUSY)
		return __LINE__;
	if (pthread_mutex_unlock(mutex) != 0 ||
	    pthread_mutex_destroy(mutex) != 0)
		return __LINE__;

	memset(&storage, 0, sizeof(storage));
	storage.words[4] = 2;
	if (pthread_mutex_lock(mutex) != 0)
		return __LINE__;
	result = pthread_mutex_lock(mutex);
	if (result != EDEADLK)
		return __LINE__;
	if (pthread_mutex_unlock(mutex) != 0 ||
	    pthread_mutex_destroy(mutex) != 0)
		return __LINE__;
	return 0;
}

static int test_explicit_mutex_attributes(void)
{
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;

	if (pthread_mutexattr_init(&attr) != 0 ||
	    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||
	    pthread_mutex_init(&mutex, &attr) != 0 ||
	    pthread_mutexattr_destroy(&attr) != 0)
		return __LINE__;
	if (pthread_mutex_lock(&mutex) != 0 ||
	    pthread_mutex_lock(&mutex) != 0 ||
	    pthread_mutex_unlock(&mutex) != 0 ||
	    pthread_mutex_unlock(&mutex) != 0 ||
	    pthread_mutex_destroy(&mutex) != 0)
		return __LINE__;
	return 0;
}

static int wait_for_child(pid_t child)
{
	struct timespec delay = { 0, 10000000 };
	int status;
	int i;

	for (i = 0; i < 300; i++) {
		if (waitpid(child, &status, WNOHANG) == child)
			return WIFEXITED(status) && WEXITSTATUS(status) == 0
				? 0 : -1;
		nanosleep(&delay, NULL);
	}
	kill(child, SIGKILL);
	waitpid(child, &status, 0);
	return -1;
}

static int wait_until(volatile int *value)
{
	struct timespec delay = { 0, 1000000 };
	int i;

	for (i = 0; i < 3000; i++) {
		if (__atomic_load_n(value, __ATOMIC_ACQUIRE))
			return 0;
		nanosleep(&delay, NULL);
	}
	return -1;
}

static int wait_until_at_least(volatile int *value, int expected)
{
	struct timespec delay = { 0, 1000000 };
	int i;

	for (i = 0; i < 3000; i++) {
		if (__atomic_load_n(value, __ATOMIC_ACQUIRE) >= expected)
			return 0;
		nanosleep(&delay, NULL);
	}
	return -1;
}

static void *return_argument(void *argument)
{
	return argument;
}

static volatile int detached_done;

static void *detached_worker(void *argument)
{
	(void)argument;
	__atomic_store_n(&detached_done, 1, __ATOMIC_RELEASE);
	return NULL;
}

static int test_thread_attributes_and_lifecycle(void)
{
	long page_size = sysconf(_SC_PAGESIZE);
	size_t stack_size;
	pthread_attr_t attr;
	pthread_t thread;
	void *stack;
	void *result;
	int token;
	int error;

	if (page_size <= 0)
		return __LINE__;
	stack_size = (size_t)PTHREAD_STACK_MIN * 2;
	stack_size = (stack_size + (size_t)page_size - 1) /
		     (size_t)page_size * (size_t)page_size;
	stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED)
		return __LINE__;

	if (pthread_attr_init(&attr) != 0) {
		munmap(stack, stack_size);
		return __LINE__;
	}
	errno = EDOM;
	error = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN - 1);
	if (error != EINVAL || errno != EDOM ||
	    pthread_attr_setstacksize(&attr, stack_size) != 0 ||
	    pthread_attr_setstack(&attr, stack, stack_size) != 0 ||
	    pthread_create(&thread, &attr, return_argument, &token) != 0 ||
	    pthread_join(thread, &result) != 0 || result != &token ||
	    pthread_attr_destroy(&attr) != 0) {
		munmap(stack, stack_size);
		return __LINE__;
	}
	if (munmap(stack, stack_size) != 0)
		return __LINE__;

	__atomic_store_n(&detached_done, 0, __ATOMIC_RELEASE);
	if (pthread_create(&thread, NULL, detached_worker, NULL) != 0 ||
	    pthread_detach(thread) != 0 ||
	    wait_until(&detached_done) != 0)
		return __LINE__;
	return 0;
}

struct cond_group {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	volatile int ready;
	int release;
};

static void *cond_waiter(void *argument)
{
	struct cond_group *group = argument;

	if (pthread_mutex_lock(&group->mutex) != 0)
		return (void *)(uintptr_t)1;
	__atomic_add_fetch(&group->ready, 1, __ATOMIC_RELEASE);
	while (!group->release)
		if (pthread_cond_wait(&group->cond, &group->mutex) != 0)
			return (void *)(uintptr_t)2;
	if (pthread_mutex_unlock(&group->mutex) != 0)
		return (void *)(uintptr_t)3;
	return NULL;
}

static int test_condition_broadcast_and_timeout(void)
{
	struct cond_group group;
	struct timespec deadline;
	pthread_t threads[2];
	void *result;
	int error;
	int i;

	memset(&group, 0, sizeof(group));
	if (pthread_mutex_init(&group.mutex, NULL) != 0 ||
	    pthread_cond_init(&group.cond, NULL) != 0)
		return __LINE__;
	for (i = 0; i < 2; i++)
		if (pthread_create(&threads[i], NULL, cond_waiter, &group) != 0)
			return __LINE__;
	if (wait_until_at_least(&group.ready, 2) != 0 ||
	    pthread_mutex_lock(&group.mutex) != 0)
		return __LINE__;
	group.release = 1;
	if (pthread_cond_broadcast(&group.cond) != 0 ||
	    pthread_mutex_unlock(&group.mutex) != 0)
		return __LINE__;
	for (i = 0; i < 2; i++)
		if (pthread_join(threads[i], &result) != 0 || result != NULL)
			return __LINE__;

	if (pthread_mutex_lock(&group.mutex) != 0 ||
	    clock_gettime(CLOCK_REALTIME, &deadline) != 0)
		return __LINE__;
	deadline.tv_nsec += 20000000;
	if (deadline.tv_nsec >= 1000000000) {
		deadline.tv_sec++;
		deadline.tv_nsec -= 1000000000;
	}
	errno = EDOM;
	do {
		error = pthread_cond_timedwait(&group.cond, &group.mutex,
					       &deadline);
	} while (error == 0);
	if (error != ETIMEDOUT || errno != EDOM ||
	    pthread_mutex_unlock(&group.mutex) != 0 ||
	    pthread_cond_destroy(&group.cond) != 0 ||
	    pthread_mutex_destroy(&group.mutex) != 0)
		return __LINE__;
	return 0;
}

static int test_required_lock_and_semaphore_errors(void)
{
	pthread_mutexattr_t mutex_attr;
	pthread_rwlock_t rwlock;
	sem_t sem;
	int error;

	if (pthread_mutexattr_init(&mutex_attr) != 0 ||
	    pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_NONE) != 0)
		return __LINE__;
	errno = EDOM;
	error = pthread_mutexattr_setprotocol(&mutex_attr, -1);
	if (error != EINVAL || errno != EDOM ||
	    pthread_mutexattr_destroy(&mutex_attr) != 0)
		return __LINE__;

	if (pthread_rwlock_init(&rwlock, NULL) != 0 ||
	    pthread_rwlock_rdlock(&rwlock) != 0 ||
	    pthread_rwlock_trywrlock(&rwlock) != EBUSY ||
	    pthread_rwlock_unlock(&rwlock) != 0 ||
	    pthread_rwlock_wrlock(&rwlock) != 0 ||
	    pthread_rwlock_tryrdlock(&rwlock) != EBUSY ||
	    pthread_rwlock_unlock(&rwlock) != 0 ||
	    pthread_rwlock_destroy(&rwlock) != 0)
		return __LINE__;

	if (sem_init(&sem, 0, 0) != 0)
		return __LINE__;
	errno = 0;
	if (sem_trywait(&sem) != -1 || errno != EAGAIN ||
	    sem_post(&sem) != 0 || sem_wait(&sem) != 0 ||
	    sem_destroy(&sem) != 0)
		return __LINE__;
	return 0;
}

static int test_direct_pthread_abi(void)
{
	cpu_set_t affinity;
	sigset_t block;
	sigset_t old;
	pthread_spinlock_t spin;
	pthread_key_t key;
	pthread_t self;
	int value = 0;
	int error;

	self = pthread_self();
	if (self != pthread_self())
		return __LINE__;
	errno = EDOM;
	if (pthread_key_create(&key, NULL) != 0 ||
	    pthread_setspecific(key, &value) != 0 ||
	    pthread_getspecific(key) != &value ||
	    pthread_key_delete(key) != 0 || errno != EDOM)
		return __LINE__;
	if (__pthread_key_create == NULL ||
	    __pthread_key_create(&key, NULL) != 0 ||
	    pthread_key_delete(key) != 0)
		return __LINE__;

	if (pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE) != 0 ||
	    pthread_spin_lock(&spin) != 0 ||
	    pthread_spin_trylock(&spin) != EBUSY ||
	    pthread_spin_unlock(&spin) != 0 ||
	    pthread_spin_destroy(&spin) != 0)
		return __LINE__;

	sigemptyset(&block);
	sigaddset(&block, SIGUSR1);
	if (pthread_sigmask(SIG_BLOCK, &block, &old) != 0 ||
	    pthread_kill(self, 0) != 0 ||
	    pthread_sigmask(SIG_SETMASK, &old, NULL) != 0)
		return __LINE__;

	CPU_ZERO(&affinity);
	error = pthread_getaffinity_np(self, sizeof(affinity), &affinity);
	if (error != 0 ||
	    pthread_setaffinity_np(self, sizeof(affinity), &affinity) != 0)
		return __LINE__;
	return 0;
}

struct shared_mutex {
	pthread_mutex_t mutex;
	volatile int ready;
	volatile int value;
};

static int test_pshared_mutex(void)
{
	struct shared_mutex *shared;
	pthread_mutexattr_t attr;
	pid_t child;
	int failed = 0;

	shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE,
		      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared == MAP_FAILED)
		return __LINE__;
	if (pthread_mutexattr_init(&attr) != 0 ||
	    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0 ||
	    pthread_mutex_init(&shared->mutex, &attr) != 0 ||
	    pthread_mutexattr_destroy(&attr) != 0 ||
	    pthread_mutex_lock(&shared->mutex) != 0)
		failed = 1;

	child = failed ? -1 : fork();
	if (child == 0) {
		__atomic_store_n(&shared->ready, 1, __ATOMIC_RELEASE);
		if (pthread_mutex_lock(&shared->mutex) != 0)
			_exit(1);
		shared->value = 1;
		if (pthread_mutex_unlock(&shared->mutex) != 0)
			_exit(2);
		_exit(0);
	}
	if (child < 0)
		failed = 1;
	if (!failed && wait_until(&shared->ready) != 0)
		failed = 1;
	if (pthread_mutex_unlock(&shared->mutex) != 0)
		failed = 1;
	if (child > 0 && wait_for_child(child) != 0)
		failed = 1;
	if (shared->value != 1)
		failed = 1;
	if (pthread_mutex_destroy(&shared->mutex) != 0)
		failed = 1;
	munmap(shared, sizeof(*shared));
	return failed ? __LINE__ : 0;
}

struct shared_cond {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	volatile int ready;
	int proceed;
	int observed;
};

static int test_pshared_cond(void)
{
	struct shared_cond *shared;
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	pid_t child;
	int failed = 0;

	shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE,
		      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared == MAP_FAILED)
		return __LINE__;
	if (pthread_mutexattr_init(&mutex_attr) != 0 ||
	    pthread_mutexattr_setpshared(&mutex_attr,
					 PTHREAD_PROCESS_SHARED) != 0 ||
	    pthread_condattr_init(&cond_attr) != 0 ||
	    pthread_condattr_setpshared(&cond_attr,
					PTHREAD_PROCESS_SHARED) != 0 ||
	    pthread_mutex_init(&shared->mutex, &mutex_attr) != 0 ||
	    pthread_cond_init(&shared->cond, &cond_attr) != 0 ||
	    pthread_mutexattr_destroy(&mutex_attr) != 0 ||
	    pthread_condattr_destroy(&cond_attr) != 0)
		failed = 1;

	child = failed ? -1 : fork();
	if (child == 0) {
		if (pthread_mutex_lock(&shared->mutex) != 0)
			_exit(1);
		__atomic_store_n(&shared->ready, 1, __ATOMIC_RELEASE);
		while (!shared->proceed)
			if (pthread_cond_wait(&shared->cond,
					      &shared->mutex) != 0)
				_exit(2);
		shared->observed = 1;
		if (pthread_mutex_unlock(&shared->mutex) != 0)
			_exit(3);
		_exit(0);
	}
	if (child < 0)
		failed = 1;
	if (!failed && wait_until(&shared->ready) != 0)
		failed = 1;
	if (!failed && pthread_mutex_lock(&shared->mutex) != 0)
		failed = 1;
	shared->proceed = 1;
	if (!failed && pthread_cond_signal(&shared->cond) != 0)
		failed = 1;
	if (!failed && pthread_mutex_unlock(&shared->mutex) != 0)
		failed = 1;
	if (child > 0 && wait_for_child(child) != 0)
		failed = 1;
	if (shared->observed != 1)
		failed = 1;
	if (pthread_cond_destroy(&shared->cond) != 0 ||
	    pthread_mutex_destroy(&shared->mutex) != 0)
		failed = 1;
	munmap(shared, sizeof(*shared));
	return failed ? __LINE__ : 0;
}

struct shared_rwlock {
	pthread_rwlock_t rwlock;
	volatile int ready;
	volatile int value;
};

static int test_pshared_rwlock(void)
{
	struct shared_rwlock *shared;
	pthread_rwlockattr_t attr;
	pid_t child;
	int failed = 0;

	shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE,
		      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared == MAP_FAILED)
		return __LINE__;
	if (pthread_rwlockattr_init(&attr) != 0 ||
	    pthread_rwlockattr_setpshared(&attr,
					  PTHREAD_PROCESS_SHARED) != 0 ||
	    pthread_rwlock_init(&shared->rwlock, &attr) != 0 ||
	    pthread_rwlockattr_destroy(&attr) != 0 ||
	    pthread_rwlock_wrlock(&shared->rwlock) != 0)
		failed = 1;

	child = failed ? -1 : fork();
	if (child == 0) {
		__atomic_store_n(&shared->ready, 1, __ATOMIC_RELEASE);
		if (pthread_rwlock_wrlock(&shared->rwlock) != 0)
			_exit(1);
		shared->value = 1;
		if (pthread_rwlock_unlock(&shared->rwlock) != 0)
			_exit(2);
		_exit(0);
	}
	if (child < 0)
		failed = 1;
	if (!failed && wait_until(&shared->ready) != 0)
		failed = 1;
	if (pthread_rwlock_unlock(&shared->rwlock) != 0)
		failed = 1;
	if (child > 0 && wait_for_child(child) != 0)
		failed = 1;
	if (shared->value != 1)
		failed = 1;
	if (pthread_rwlock_destroy(&shared->rwlock) != 0)
		failed = 1;
	munmap(shared, sizeof(*shared));
	return failed ? __LINE__ : 0;
}

static int test_pshared_semaphore(void)
{
	sem_t *sem;
	struct timespec deadline;
	pid_t child;
	int status;

	sem = mmap(NULL, sizeof(*sem), PROT_READ | PROT_WRITE,
		   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (sem == MAP_FAILED)
		return __LINE__;

	errno = 0;
	if (sem_init(sem, 0, (unsigned int)SEM_VALUE_MAX + 1u) != -1 ||
	    errno != EINVAL) {
		munmap(sem, sizeof(*sem));
		return __LINE__;
	}
	if (sem_init(sem, 1, 0) != 0) {
		munmap(sem, sizeof(*sem));
		return __LINE__;
	}
	child = fork();
	if (child == 0)
		_exit(sem_post(sem) == 0 ? 0 : 1);
	if (child < 0) {
		sem_destroy(sem);
		munmap(sem, sizeof(*sem));
		return __LINE__;
	}
	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += 3;
	if (sem_timedwait(sem, &deadline) != 0 ||
	    waitpid(child, &status, 0) != child ||
	    !WIFEXITED(status) || WEXITSTATUS(status) != 0 ||
	    sem_destroy(sem) != 0) {
		kill(child, SIGKILL);
		waitpid(child, NULL, 0);
		munmap(sem, sizeof(*sem));
		return __LINE__;
	}
	munmap(sem, sizeof(*sem));
	return 0;
}

static pthread_once_t competing_once = PTHREAD_ONCE_INIT;
static int competing_once_calls;

static void competing_once_init(void)
{
	struct timespec delay = { 0, 20000000 };

	__atomic_add_fetch(&competing_once_calls, 1, __ATOMIC_RELAXED);
	nanosleep(&delay, NULL);
}

static int once_worker(void *argument)
{
	(void)argument;
	return pthread_once(&competing_once, competing_once_init);
}

static int test_competing_once(void)
{
	thrd_t threads[THREAD_COUNT];
	int result;
	int i;

	for (i = 0; i < THREAD_COUNT; i++)
		if (thrd_create(&threads[i], once_worker, NULL) != thrd_success)
			return __LINE__;
	for (i = 0; i < THREAD_COUNT; i++)
		if (thrd_join(threads[i], &result) != thrd_success || result)
			return __LINE__;
	return competing_once_calls == 1 ? 0 : __LINE__;
}

static pthread_once_t fork_once = PTHREAD_ONCE_INIT;
static volatile int fork_once_started;
static volatile int fork_once_release;
static pid_t fork_once_parent;
static int fork_once_child_calls;

static void fork_once_init(void)
{
	if (getpid() != fork_once_parent) {
		fork_once_child_calls++;
		return;
	}
	__atomic_store_n(&fork_once_started, 1, __ATOMIC_RELEASE);
	while (!__atomic_load_n(&fork_once_release, __ATOMIC_ACQUIRE))
		sched_yield();
}

static int fork_once_worker(void *argument)
{
	(void)argument;
	return pthread_once(&fork_once, fork_once_init);
}

static int test_fork_during_once(void)
{
	thrd_t thread;
	pid_t child;
	int result;
	int failed = 0;

	fork_once_parent = getpid();
	if (thrd_create(&thread, fork_once_worker, NULL) != thrd_success)
		return __LINE__;
	if (wait_until(&fork_once_started) != 0)
		failed = 1;
	child = failed ? -1 : fork();
	if (child == 0) {
		result = pthread_once(&fork_once, fork_once_init);
		_exit(result == 0 && fork_once_child_calls == 1 ? 0 : 1);
	}
	if (child < 0)
		failed = 1;
	if (child > 0 && wait_for_child(child) != 0)
		failed = 1;
	__atomic_store_n(&fork_once_release, 1, __ATOMIC_RELEASE);
	if (thrd_join(thread, &result) != thrd_success || result)
		failed = 1;
	return failed ? __LINE__ : 0;
}

struct test_case {
	const char *name;
	int (*run)(void);
};

int main(void)
{
	static const struct test_case tests[] = {
		{ "concurrent resolver", test_concurrent_resolver },
		{ "concurrent static mutex", test_concurrent_static_mutex },
		{ "static mutexes", test_static_mutexes },
		{ "explicit mutex attributes", test_explicit_mutex_attributes },
		{ "thread attributes and lifecycle",
		  test_thread_attributes_and_lifecycle },
		{ "condition broadcast and timeout",
		  test_condition_broadcast_and_timeout },
		{ "required lock and semaphore errors",
		  test_required_lock_and_semaphore_errors },
		{ "direct pthread ABI", test_direct_pthread_abi },
		{ "pshared mutex", test_pshared_mutex },
		{ "pshared condition variable", test_pshared_cond },
		{ "pshared rwlock", test_pshared_rwlock },
		{ "pshared semaphore", test_pshared_semaphore },
		{ "competing pthread_once", test_competing_once },
		{ "fork during pthread_once", test_fork_during_once },
	};
	size_t i;
	int line;

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		line = tests[i].run();
		if (line) {
			fprintf(stderr, "%s failed at line %d\n",
				tests[i].name, line);
			return 1;
		}
	}
	return 0;
}
