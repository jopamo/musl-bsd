#include "glibc_pthread_abi.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define FUTEX_WAIT_PRIVATE 128
#define FUTEX_WAKE_PRIVATE 129

typedef int (*mutex_init_fn)(pthread_mutex_t *, const pthread_mutexattr_t *);
typedef int (*mutex_lock_fn)(pthread_mutex_t *);
typedef int (*mutex_unlock_fn)(pthread_mutex_t *);
typedef int (*mutex_trylock_fn)(pthread_mutex_t *);
typedef int (*mutex_destroy_fn)(pthread_mutex_t *);
typedef int (*mutexattr_init_fn)(pthread_mutexattr_t *);
typedef int (*mutexattr_destroy_fn)(pthread_mutexattr_t *);
typedef int (*mutexattr_setprotocol_fn)(pthread_mutexattr_t *, int);
typedef int (*mutexattr_setpshared_fn)(pthread_mutexattr_t *, int);
typedef int (*mutexattr_settype_fn)(pthread_mutexattr_t *, int);
typedef int (*cond_init_fn)(pthread_cond_t *, const pthread_condattr_t *);
typedef int (*cond_destroy_fn)(pthread_cond_t *);
typedef int (*cond_wait_fn)(pthread_cond_t *, pthread_mutex_t *);
typedef int (*cond_timedwait_fn)(pthread_cond_t *, pthread_mutex_t *,
				 const struct timespec *);
typedef int (*cond_signal_fn)(pthread_cond_t *);
typedef int (*cond_broadcast_fn)(pthread_cond_t *);
typedef int (*condattr_init_fn)(pthread_condattr_t *);
typedef int (*condattr_destroy_fn)(pthread_condattr_t *);
typedef int (*condattr_setclock_fn)(pthread_condattr_t *, clockid_t);
typedef int (*condattr_setpshared_fn)(pthread_condattr_t *, int);
typedef int (*rwlock_init_fn)(pthread_rwlock_t *,
			      const pthread_rwlockattr_t *);
typedef int (*rwlock_destroy_fn)(pthread_rwlock_t *);
typedef int (*rwlock_rdlock_fn)(pthread_rwlock_t *);
typedef int (*rwlock_wrlock_fn)(pthread_rwlock_t *);
typedef int (*rwlock_unlock_fn)(pthread_rwlock_t *);
typedef int (*rwlock_tryrdlock_fn)(pthread_rwlock_t *);
typedef int (*rwlock_trywrlock_fn)(pthread_rwlock_t *);
typedef int (*rwlockattr_init_fn)(pthread_rwlockattr_t *);
typedef int (*rwlockattr_destroy_fn)(pthread_rwlockattr_t *);
typedef int (*rwlockattr_setpshared_fn)(pthread_rwlockattr_t *, int);
typedef int (*sem_init_fn)(sem_t *, int, unsigned);
typedef int (*sem_destroy_fn)(sem_t *);
typedef int (*sem_post_fn)(sem_t *);
typedef int (*sem_wait_fn)(sem_t *);
typedef int (*sem_trywait_fn)(sem_t *);
typedef int (*sem_timedwait_fn)(sem_t *, const struct timespec *);
typedef int (*attr_init_fn)(pthread_attr_t *);
typedef int (*attr_destroy_fn)(pthread_attr_t *);
typedef int (*attr_setdetachstate_fn)(pthread_attr_t *, int);
typedef int (*attr_setinheritsched_fn)(pthread_attr_t *, int);
typedef int (*attr_setschedparam_fn)(pthread_attr_t *,
				     const struct sched_param *);
typedef int (*attr_setschedpolicy_fn)(pthread_attr_t *, int);
typedef int (*attr_setscope_fn)(pthread_attr_t *, int);
typedef int (*attr_setstack_fn)(pthread_attr_t *, void *, size_t);
typedef int (*attr_setguardsize_fn)(pthread_attr_t *, size_t);
typedef int (*attr_setstacksize_fn)(pthread_attr_t *, size_t);
typedef int (*create_fn)(pthread_t *, const pthread_attr_t *,
			 void *(*)(void *), void *);
typedef int (*join_fn)(pthread_t, void **);
typedef int (*detach_fn)(pthread_t);

typedef char dlsym_pointer_representation[
	(sizeof(void *) == sizeof(detach_fn)) ? 1 : -1];

static mutex_init_fn p_mutex_init;
static mutex_lock_fn p_mutex_lock;
static mutex_unlock_fn p_mutex_unlock;
static mutex_trylock_fn p_mutex_trylock;
static mutex_destroy_fn p_mutex_destroy;
static mutexattr_init_fn p_mutexattr_init;
static mutexattr_destroy_fn p_mutexattr_destroy;
static mutexattr_setprotocol_fn p_mutexattr_setprotocol;
static mutexattr_setpshared_fn p_mutexattr_setpshared;
static mutexattr_settype_fn p_mutexattr_settype;
static cond_init_fn p_cond_init;
static cond_destroy_fn p_cond_destroy;
static cond_wait_fn p_cond_wait;
static cond_timedwait_fn p_cond_timedwait;
static cond_signal_fn p_cond_signal;
static cond_broadcast_fn p_cond_broadcast;
static condattr_init_fn p_condattr_init;
static condattr_destroy_fn p_condattr_destroy;
static condattr_setclock_fn p_condattr_setclock;
static condattr_setpshared_fn p_condattr_setpshared;
static rwlock_init_fn p_rwlock_init;
static rwlock_destroy_fn p_rwlock_destroy;
static rwlock_rdlock_fn p_rwlock_rdlock;
static rwlock_wrlock_fn p_rwlock_wrlock;
static rwlock_unlock_fn p_rwlock_unlock;
static rwlock_tryrdlock_fn p_rwlock_tryrdlock;
static rwlock_trywrlock_fn p_rwlock_trywrlock;
static rwlockattr_init_fn p_rwlockattr_init;
static rwlockattr_destroy_fn p_rwlockattr_destroy;
static rwlockattr_setpshared_fn p_rwlockattr_setpshared;
static sem_init_fn p_sem_init;
static sem_destroy_fn p_sem_destroy;
static sem_post_fn p_sem_post;
static sem_wait_fn p_sem_wait;
static sem_trywait_fn p_sem_trywait;
static sem_timedwait_fn p_sem_timedwait;
static attr_init_fn p_attr_init;
static attr_destroy_fn p_attr_destroy;
static attr_setdetachstate_fn p_attr_setdetachstate;
static attr_setinheritsched_fn p_attr_setinheritsched;
static attr_setschedparam_fn p_attr_setschedparam;
static attr_setschedpolicy_fn p_attr_setschedpolicy;
static attr_setscope_fn p_attr_setscope;
static attr_setstack_fn p_attr_setstack;
static attr_setguardsize_fn p_attr_setguardsize;
static attr_setstacksize_fn p_attr_setstacksize;
static create_fn p_create;
static join_fn p_join;
static detach_fn p_detach;

enum resolver_state {
	RESOLVER_UNINITIALIZED,
	RESOLVER_RUNNING,
	RESOLVER_READY,
	RESOLVER_FAILED
};

static uint64_t resolver_status;
static __thread int resolver_active;

#define RESOLVER_STATUS(pid, state) \
	(((uint64_t)(uint32_t)(pid) << 32) | (uint32_t)(state))
#define RESOLVER_STATUS_STATE(status) ((uint32_t)(status))
#define RESOLVER_STATUS_PID(status) ((uint32_t)((status) >> 32))

static int resolve_native(void)
{
	uint64_t status;
	uint64_t expected;
	uint64_t running;
	uint32_t state;
	uint32_t pid;
	int missing = 0;
	void *address;

	for (;;) {
		status = __atomic_load_n(&resolver_status, __ATOMIC_ACQUIRE);
		state = RESOLVER_STATUS_STATE(status);
		if (state == RESOLVER_READY)
			return 0;
		if (state == RESOLVER_FAILED)
			return ENOSYS;
		if (state == RESOLVER_RUNNING) {
			/* Do not deadlock if the dynamic linker re-enters us. */
			if (resolver_active)
				return ENOSYS;
			pid = (uint32_t)getpid();
			if (RESOLVER_STATUS_PID(status) != pid) {
				expected = status;
				__atomic_compare_exchange_n(&resolver_status,
							    &expected, 0, 0,
							    __ATOMIC_ACQ_REL,
							    __ATOMIC_ACQUIRE);
				continue;
			}
			sched_yield();
			continue;
		}

		pid = (uint32_t)getpid();
		running = RESOLVER_STATUS(pid, RESOLVER_RUNNING);
		expected = RESOLVER_UNINITIALIZED;
		if (__atomic_compare_exchange_n(&resolver_status, &expected,
						running, 0,
						__ATOMIC_ACQ_REL,
						__ATOMIC_ACQUIRE))
			break;
	}

	resolver_active = 1;

#define RESOLVE(member, name) \
	do { \
		address = dlsym(RTLD_NEXT, name); \
		memcpy(&p_##member, &address, sizeof(p_##member)); \
		missing |= address == NULL; \
	} while (0)

	RESOLVE(mutex_init, "pthread_mutex_init");
	RESOLVE(mutex_lock, "pthread_mutex_lock");
	RESOLVE(mutex_unlock, "pthread_mutex_unlock");
	RESOLVE(mutex_trylock, "pthread_mutex_trylock");
	RESOLVE(mutex_destroy, "pthread_mutex_destroy");
	RESOLVE(mutexattr_init, "pthread_mutexattr_init");
	RESOLVE(mutexattr_destroy, "pthread_mutexattr_destroy");
	RESOLVE(mutexattr_setprotocol, "pthread_mutexattr_setprotocol");
	RESOLVE(mutexattr_setpshared, "pthread_mutexattr_setpshared");
	RESOLVE(mutexattr_settype, "pthread_mutexattr_settype");
	RESOLVE(cond_init, "pthread_cond_init");
	RESOLVE(cond_destroy, "pthread_cond_destroy");
	RESOLVE(cond_wait, "pthread_cond_wait");
	RESOLVE(cond_timedwait, "pthread_cond_timedwait");
	RESOLVE(cond_signal, "pthread_cond_signal");
	RESOLVE(cond_broadcast, "pthread_cond_broadcast");
	RESOLVE(condattr_init, "pthread_condattr_init");
	RESOLVE(condattr_destroy, "pthread_condattr_destroy");
	RESOLVE(condattr_setclock, "pthread_condattr_setclock");
	RESOLVE(condattr_setpshared, "pthread_condattr_setpshared");
	RESOLVE(rwlock_init, "pthread_rwlock_init");
	RESOLVE(rwlock_destroy, "pthread_rwlock_destroy");
	RESOLVE(rwlock_rdlock, "pthread_rwlock_rdlock");
	RESOLVE(rwlock_wrlock, "pthread_rwlock_wrlock");
	RESOLVE(rwlock_unlock, "pthread_rwlock_unlock");
	RESOLVE(rwlock_tryrdlock, "pthread_rwlock_tryrdlock");
	RESOLVE(rwlock_trywrlock, "pthread_rwlock_trywrlock");
	RESOLVE(rwlockattr_init, "pthread_rwlockattr_init");
	RESOLVE(rwlockattr_destroy, "pthread_rwlockattr_destroy");
	RESOLVE(rwlockattr_setpshared, "pthread_rwlockattr_setpshared");
	RESOLVE(sem_init, "sem_init");
	RESOLVE(sem_destroy, "sem_destroy");
	RESOLVE(sem_post, "sem_post");
	RESOLVE(sem_wait, "sem_wait");
	RESOLVE(sem_trywait, "sem_trywait");
	RESOLVE(sem_timedwait, "sem_timedwait");
	RESOLVE(attr_init, "pthread_attr_init");
	RESOLVE(attr_destroy, "pthread_attr_destroy");
	RESOLVE(attr_setdetachstate, "pthread_attr_setdetachstate");
	RESOLVE(attr_setinheritsched, "pthread_attr_setinheritsched");
	RESOLVE(attr_setschedparam, "pthread_attr_setschedparam");
	RESOLVE(attr_setschedpolicy, "pthread_attr_setschedpolicy");
	RESOLVE(attr_setscope, "pthread_attr_setscope");
	RESOLVE(attr_setstack, "pthread_attr_setstack");
	RESOLVE(attr_setguardsize, "pthread_attr_setguardsize");
	RESOLVE(attr_setstacksize, "pthread_attr_setstacksize");
	RESOLVE(create, "pthread_create");
	RESOLVE(join, "pthread_join");
	RESOLVE(detach, "pthread_detach");

#undef RESOLVE

	resolver_active = 0;
	__atomic_store_n(&resolver_status,
			 RESOLVER_STATUS(pid, missing ? RESOLVER_FAILED :
					 RESOLVER_READY),
			 __ATOMIC_RELEASE);
	return missing ? ENOSYS : 0;
}

static int native_pthread_ready(void)
{
	return resolve_native();
}

static int native_sem_ready(void)
{
	int error = resolve_native();

	if (error)
		errno = error;
	return error;
}

#define FORWARD_PTHREAD(call) \
	do { \
		int error = native_pthread_ready(); \
		if (error) \
			return error; \
		return (call); \
	} while (0)

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	FORWARD_PTHREAD(p_mutexattr_init(attr));
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	FORWARD_PTHREAD(p_mutexattr_destroy(attr));
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
	FORWARD_PTHREAD(p_mutexattr_setprotocol(attr, protocol));
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int shared)
{
	FORWARD_PTHREAD(p_mutexattr_setpshared(attr, shared));
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	FORWARD_PTHREAD(p_mutexattr_settype(attr, type));
}

int pthread_mutex_init(pthread_mutex_t *mutex,
		       const pthread_mutexattr_t *attr)
{
	FORWARD_PTHREAD(p_mutex_init(mutex, attr));
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	int error = native_pthread_ready();

	if (!error)
		error = glibc_prepare_static_mutex(mutex);
	return error ? error : p_mutex_destroy(mutex);
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	int error = native_pthread_ready();

	if (!error)
		error = glibc_prepare_static_mutex(mutex);
	return error ? error : p_mutex_lock(mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	int error = native_pthread_ready();

	if (!error)
		error = glibc_prepare_static_mutex(mutex);
	return error ? error : p_mutex_trylock(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	int error = native_pthread_ready();

	if (!error)
		error = glibc_prepare_static_mutex(mutex);
	return error ? error : p_mutex_unlock(mutex);
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
	FORWARD_PTHREAD(p_condattr_init(attr));
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
	FORWARD_PTHREAD(p_condattr_destroy(attr));
}

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock)
{
	FORWARD_PTHREAD(p_condattr_setclock(attr, clock));
}

int pthread_condattr_setpshared(pthread_condattr_t *attr, int shared)
{
	FORWARD_PTHREAD(p_condattr_setpshared(attr, shared));
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	FORWARD_PTHREAD(p_cond_init(cond, attr));
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	FORWARD_PTHREAD(p_cond_destroy(cond));
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	int error = native_pthread_ready();

	if (!error)
		error = glibc_prepare_static_mutex(mutex);
	return error ? error : p_cond_wait(cond, mutex);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			   const struct timespec *time)
{
	int error = native_pthread_ready();

	if (!error)
		error = glibc_prepare_static_mutex(mutex);
	return error ? error : p_cond_timedwait(cond, mutex, time);
}

int pthread_cond_signal(pthread_cond_t *cond)
{
	FORWARD_PTHREAD(p_cond_signal(cond));
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
	FORWARD_PTHREAD(p_cond_broadcast(cond));
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock,
			const pthread_rwlockattr_t *attr)
{
	FORWARD_PTHREAD(p_rwlock_init(rwlock, attr));
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	FORWARD_PTHREAD(p_rwlock_destroy(rwlock));
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	FORWARD_PTHREAD(p_rwlock_rdlock(rwlock));
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	FORWARD_PTHREAD(p_rwlock_wrlock(rwlock));
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	FORWARD_PTHREAD(p_rwlock_unlock(rwlock));
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	FORWARD_PTHREAD(p_rwlock_tryrdlock(rwlock));
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	FORWARD_PTHREAD(p_rwlock_trywrlock(rwlock));
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
	FORWARD_PTHREAD(p_rwlockattr_init(attr));
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
	FORWARD_PTHREAD(p_rwlockattr_destroy(attr));
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int shared)
{
	FORWARD_PTHREAD(p_rwlockattr_setpshared(attr, shared));
}

#define ONCE_INPROGRESS 1u
#define ONCE_DONE       2u
#define ONCE_STATE_MASK 3u
#define ONCE_GEN_INCR   4u
#define ONCE_GEN_MASK   (~ONCE_STATE_MASK)

static int once_pid;
static unsigned int once_generation;

static void once_futex_wait(volatile unsigned int *control,
			    unsigned int value)
{
	syscall(SYS_futex, control, FUTEX_WAIT_PRIVATE, value, NULL);
}

static void once_futex_wake(volatile unsigned int *control)
{
	syscall(SYS_futex, control, FUTEX_WAKE_PRIVATE, INT32_MAX);
}

static unsigned int process_once_generation(void)
{
	int pid = (int)getpid();
	int seen;
	int expected;

	for (;;) {
		seen = __atomic_load_n(&once_pid, __ATOMIC_ACQUIRE);
		if (seen == pid)
			return __atomic_load_n(&once_generation,
					       __ATOMIC_ACQUIRE);
		if (seen == -pid) {
			sched_yield();
			continue;
		}

		expected = seen;
		if (!__atomic_compare_exchange_n(&once_pid, &expected, -pid, 0,
						 __ATOMIC_ACQ_REL,
						 __ATOMIC_ACQUIRE))
			continue;

		seen = __atomic_add_fetch(&once_generation, ONCE_GEN_INCR,
					  __ATOMIC_RELAXED);
		__atomic_store_n(&once_pid, pid, __ATOMIC_RELEASE);
		return (unsigned int)seen & ONCE_GEN_MASK;
	}
}

static void once_cancel(void *argument)
{
	volatile unsigned int *control = argument;

	__atomic_store_n(control, 0, __ATOMIC_RELEASE);
	once_futex_wake(control);
}

int pthread_once(pthread_once_t *once, void (*init)(void))
{
	volatile unsigned int *control = (volatile unsigned int *)once;
	unsigned int generation;
	unsigned int current;
	unsigned int desired;

	current = __atomic_load_n(control, __ATOMIC_ACQUIRE);
	if ((current & ONCE_STATE_MASK) == ONCE_DONE)
		return 0;

	for (;;) {
		generation = process_once_generation();
		desired = generation | ONCE_INPROGRESS;
		current = __atomic_load_n(control, __ATOMIC_ACQUIRE);
		if ((current & ONCE_STATE_MASK) == ONCE_DONE)
			return 0;

		if (!(current & ONCE_INPROGRESS) ||
		    (current & ONCE_GEN_MASK) != generation) {
			if (!__atomic_compare_exchange_n(control, &current,
							 desired, 0,
							 __ATOMIC_ACQ_REL,
							 __ATOMIC_ACQUIRE))
				continue;

			pthread_cleanup_push(once_cancel, (void *)control);
			init();
			pthread_cleanup_pop(0);

			__atomic_store_n(control, ONCE_DONE, __ATOMIC_RELEASE);
			once_futex_wake(control);
			return 0;
		}

		once_futex_wait(control, current);
	}
}

int pthread_attr_init(pthread_attr_t *attr)
{
	FORWARD_PTHREAD(p_attr_init(attr));
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
	FORWARD_PTHREAD(p_attr_destroy(attr));
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
	FORWARD_PTHREAD(p_attr_setstacksize(attr, size));
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
	FORWARD_PTHREAD(p_attr_setdetachstate(attr, state));
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit)
{
	FORWARD_PTHREAD(p_attr_setinheritsched(attr, inherit));
}

int pthread_attr_setschedparam(pthread_attr_t *attr,
			       const struct sched_param *param)
{
	FORWARD_PTHREAD(p_attr_setschedparam(attr, param));
}

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
	FORWARD_PTHREAD(p_attr_setschedpolicy(attr, policy));
}

int pthread_attr_setscope(pthread_attr_t *attr, int scope)
{
	FORWARD_PTHREAD(p_attr_setscope(attr, scope));
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stack, size_t size)
{
	FORWARD_PTHREAD(p_attr_setstack(attr, stack, size));
}

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t size)
{
	FORWARD_PTHREAD(p_attr_setguardsize(attr, size));
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start)(void *), void *argument)
{
	FORWARD_PTHREAD(p_create(thread, attr, start, argument));
}

int pthread_join(pthread_t thread, void **result)
{
	FORWARD_PTHREAD(p_join(thread, result));
}

int pthread_detach(pthread_t thread)
{
	FORWARD_PTHREAD(p_detach(thread));
}

int sem_init(sem_t *sem, int shared, unsigned int value)
{
	if (native_sem_ready())
		return -1;
	return p_sem_init(sem, shared, value);
}

int sem_destroy(sem_t *sem)
{
	if (native_sem_ready())
		return -1;
	return p_sem_destroy(sem);
}

int sem_post(sem_t *sem)
{
	if (native_sem_ready())
		return -1;
	return p_sem_post(sem);
}

int sem_wait(sem_t *sem)
{
	if (native_sem_ready())
		return -1;
	return p_sem_wait(sem);
}

int sem_trywait(sem_t *sem)
{
	if (native_sem_ready())
		return -1;
	return p_sem_trywait(sem);
}

int sem_timedwait(sem_t *sem, const struct timespec *time)
{
	if (native_sem_ready())
		return -1;
	return p_sem_timedwait(sem, time);
}
