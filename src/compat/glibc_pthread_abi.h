#ifndef MUSL_BSD_GLIBC_PTHREAD_ABI_H
#define MUSL_BSD_GLIBC_PTHREAD_ABI_H

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

/*
 * This bridge is enabled only for ABIs verified here.  These constants are
 * the x86_64 glibc opaque-object ABI; the native musl objects must fit in the
 * caller-provided storage and require no stronger alignment.
 */
#if !defined(__x86_64__) || !defined(__SIZEOF_POINTER__) || \
	__SIZEOF_POINTER__ != 8
#error "glibc pthread ABI bridge is not verified for this architecture"
#endif

#define GLIBC_PTHREAD_ATTR_SIZE          56
#define GLIBC_PTHREAD_MUTEX_SIZE         40
#define GLIBC_PTHREAD_MUTEXATTR_SIZE      4
#define GLIBC_PTHREAD_COND_SIZE          48
#define GLIBC_PTHREAD_CONDATTR_SIZE       4
#define GLIBC_PTHREAD_RWLOCK_SIZE        56
#define GLIBC_PTHREAD_RWLOCKATTR_SIZE     8
#define GLIBC_PTHREAD_ONCE_SIZE           4
#define GLIBC_SEM_SIZE                   32

#define GLIBC_PTHREAD_ATTR_ALIGN          8
#define GLIBC_PTHREAD_MUTEX_ALIGN         8
#define GLIBC_PTHREAD_MUTEXATTR_ALIGN     4
#define GLIBC_PTHREAD_COND_ALIGN          8
#define GLIBC_PTHREAD_CONDATTR_ALIGN      4
#define GLIBC_PTHREAD_RWLOCK_ALIGN        8
#define GLIBC_PTHREAD_RWLOCKATTR_ALIGN    8
#define GLIBC_PTHREAD_ONCE_ALIGN          4
#define GLIBC_SEM_ALIGN                   8

#define ABI_FITS(name, type, size, align) \
	typedef char name##_size_fits[(sizeof(type) <= (size)) ? 1 : -1]; \
	typedef char name##_alignment_fits[(__alignof__(type) <= (align)) ? 1 : -1]

ABI_FITS(pthread_attr, pthread_attr_t, GLIBC_PTHREAD_ATTR_SIZE,
	 GLIBC_PTHREAD_ATTR_ALIGN);
ABI_FITS(pthread_mutex, pthread_mutex_t, GLIBC_PTHREAD_MUTEX_SIZE,
	 GLIBC_PTHREAD_MUTEX_ALIGN);
ABI_FITS(pthread_mutexattr, pthread_mutexattr_t, GLIBC_PTHREAD_MUTEXATTR_SIZE,
	 GLIBC_PTHREAD_MUTEXATTR_ALIGN);
ABI_FITS(pthread_cond, pthread_cond_t, GLIBC_PTHREAD_COND_SIZE,
	 GLIBC_PTHREAD_COND_ALIGN);
ABI_FITS(pthread_condattr, pthread_condattr_t, GLIBC_PTHREAD_CONDATTR_SIZE,
	 GLIBC_PTHREAD_CONDATTR_ALIGN);
ABI_FITS(pthread_rwlock, pthread_rwlock_t, GLIBC_PTHREAD_RWLOCK_SIZE,
	 GLIBC_PTHREAD_RWLOCK_ALIGN);
ABI_FITS(pthread_rwlockattr, pthread_rwlockattr_t,
	 GLIBC_PTHREAD_RWLOCKATTR_SIZE,
	 GLIBC_PTHREAD_RWLOCKATTR_ALIGN);
ABI_FITS(pthread_once, pthread_once_t, GLIBC_PTHREAD_ONCE_SIZE,
	 GLIBC_PTHREAD_ONCE_ALIGN);
ABI_FITS(sem, sem_t, GLIBC_SEM_SIZE, GLIBC_SEM_ALIGN);

#undef ABI_FITS

/*
 * An untouched x86_64 glibc static mutex stores its kind in word 4.  Musl
 * stores _m_type in word 0 and does not use word 4.  A compare-exchange
 * publishes the native type exactly once; clearing the now-unused foreign
 * kind word can safely follow it.
 */
#define GLIBC_MUTEX_KIND_WORD             4
#define GLIBC_MUTEX_RECURSIVE_KIND        1u
#define GLIBC_MUTEX_ERRORCHECK_KIND       2u
static inline int glibc_prepare_static_mutex(pthread_mutex_t *mutex)
{
#if defined(__GLIBC__)
	/*
	 * Native glibc already consumes its own static initializer.  Keeping this
	 * build path makes the ABI fixtures runnable on glibc development hosts.
	 */
	(void)mutex;
	return 0;
#else
	volatile unsigned int *words = (volatile unsigned int *)mutex;
	unsigned int type;
	unsigned int kind;

	for (;;) {
		type = __atomic_load_n(&words[0], __ATOMIC_ACQUIRE);
		/* Nonzero word 0 is already a native musl mutex type. */
		if (type != 0)
			return 0;

		kind = __atomic_load_n(&words[GLIBC_MUTEX_KIND_WORD],
				       __ATOMIC_ACQUIRE);
		if (kind == 0)
			return 0;
		if (kind != GLIBC_MUTEX_RECURSIVE_KIND &&
		    kind != GLIBC_MUTEX_ERRORCHECK_KIND)
			return ENOTSUP;

		type = 0;
		if (!__atomic_compare_exchange_n(&words[0], &type,
						 kind, 0,
						 __ATOMIC_ACQ_REL,
						 __ATOMIC_ACQUIRE))
			continue;

		__atomic_store_n(&words[GLIBC_MUTEX_KIND_WORD], 0,
				 __ATOMIC_RELAXED);
		return 0;
	}
#endif
}

#endif
