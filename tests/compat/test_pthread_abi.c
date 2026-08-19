#include <pthread.h>
#include <string.h>

/*
 * This is the x86_64 glibc layout for a statically initialized recursive
 * mutex. The object is intentionally opaque to the test; the compatibility
 * DSO must translate it before passing it to musl.
 */
struct glibc_mutex_storage {
	unsigned int words[10];
};

int main(void)
{
	struct glibc_mutex_storage storage;
	pthread_mutex_t *mutex = (pthread_mutex_t *)&storage;

	memset(&storage, 0, sizeof(storage));
	storage.words[4] = 1; /* PTHREAD_MUTEX_RECURSIVE_NP */

	if (pthread_mutex_lock(mutex) != 0)
		return 1;
	if (pthread_mutex_lock(mutex) != 0)
		return 2;
	if (pthread_mutex_unlock(mutex) != 0)
		return 3;
	if (pthread_mutex_unlock(mutex) != 0)
		return 4;
	if (pthread_mutex_destroy(mutex) != 0)
		return 5;

	return 0;
}
