#include "../../src/compat/glibc_pthread_abi.h"

#include <stddef.h>

#define ABI_EQUALS(name, type, size, align) \
	typedef char name##_size_matches[(sizeof(type) == (size)) ? 1 : -1]; \
	typedef char name##_alignment_matches[ \
		(__alignof__(type) == (align)) ? 1 : -1]

ABI_EQUALS(pthread_attr, pthread_attr_t, GLIBC_PTHREAD_ATTR_SIZE,
	   GLIBC_PTHREAD_ATTR_ALIGN);
ABI_EQUALS(pthread_mutex, pthread_mutex_t, GLIBC_PTHREAD_MUTEX_SIZE,
	   GLIBC_PTHREAD_MUTEX_ALIGN);
ABI_EQUALS(pthread_mutexattr, pthread_mutexattr_t,
	   GLIBC_PTHREAD_MUTEXATTR_SIZE,
	   GLIBC_PTHREAD_MUTEXATTR_ALIGN);
ABI_EQUALS(pthread_cond, pthread_cond_t, GLIBC_PTHREAD_COND_SIZE,
	   GLIBC_PTHREAD_COND_ALIGN);
ABI_EQUALS(pthread_condattr, pthread_condattr_t, GLIBC_PTHREAD_CONDATTR_SIZE,
	   GLIBC_PTHREAD_CONDATTR_ALIGN);
ABI_EQUALS(pthread_rwlock, pthread_rwlock_t, GLIBC_PTHREAD_RWLOCK_SIZE,
	   GLIBC_PTHREAD_RWLOCK_ALIGN);
ABI_EQUALS(pthread_rwlockattr, pthread_rwlockattr_t,
	   GLIBC_PTHREAD_RWLOCKATTR_SIZE,
	   GLIBC_PTHREAD_RWLOCKATTR_ALIGN);
ABI_EQUALS(pthread_once, pthread_once_t, GLIBC_PTHREAD_ONCE_SIZE,
	   GLIBC_PTHREAD_ONCE_ALIGN);
ABI_EQUALS(sem, sem_t, GLIBC_SEM_SIZE, GLIBC_SEM_ALIGN);

typedef char glibc_mutex_kind_offset_matches[
	(offsetof(pthread_mutex_t, __data.__kind) ==
	 GLIBC_MUTEX_KIND_WORD * sizeof(unsigned int)) ? 1 : -1];
typedef char glibc_recursive_mutex_kind_matches[
	(PTHREAD_MUTEX_RECURSIVE_NP == GLIBC_MUTEX_RECURSIVE_KIND) ? 1 : -1];
typedef char glibc_errorcheck_mutex_kind_matches[
	(PTHREAD_MUTEX_ERRORCHECK_NP == GLIBC_MUTEX_ERRORCHECK_KIND) ? 1 : -1];

int main(void)
{
	pthread_mutex_t recursive = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	pthread_mutex_t errorcheck = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

	if ((unsigned int)recursive.__data.__kind !=
	    GLIBC_MUTEX_RECURSIVE_KIND)
		return 1;
	if ((unsigned int)errorcheck.__data.__kind !=
	    GLIBC_MUTEX_ERRORCHECK_KIND)
		return 2;
	return 0;
}
