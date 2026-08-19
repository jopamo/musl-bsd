#include <dlfcn.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
 * The NVIDIA userspace libraries are built against glibc.  Their pthread
 * objects are opaque to the caller, but glibc and musl use different
 * representations for those objects.  Passing a glibc mutex directly to a
 * musl pthread function therefore turns fields such as glibc's mutex kind
 * into musl futex state.
 *
 * Keep a native musl object behind each glibc object address.  The map is
 * deliberately private to this DSO: these wrappers are only for glibc ABI
 * entry points resolved from preloaded DSOs, not for musl applications.
 */
struct pthread_node {
	struct pthread_node *next;
	const void *key;
	int kind;
	int initialized;
	union {
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		pthread_mutexattr_t mutex_attr;
		pthread_condattr_t cond_attr;
		pthread_rwlock_t rwlock;
		pthread_rwlockattr_t rwlock_attr;
		sem_t sem;
		pthread_attr_t attr;
	} u;
};

static struct pthread_node *nodes;
static volatile int nodes_lock;

static void lock_nodes(void)
{
	while (__sync_lock_test_and_set(&nodes_lock, 1))
		sched_yield();
}

static void unlock_nodes(void)
{
	__sync_lock_release(&nodes_lock);
}

static struct pthread_node *find_node(const void *key, int kind, int create)
{
	struct pthread_node *node;

	lock_nodes();
	for (node = nodes; node; node = node->next) {
		if (node->key == key && node->kind == kind) {
			unlock_nodes();
			return node;
		}
	}

	if (!create) {
		unlock_nodes();
		return NULL;
	}

	node = calloc(1, sizeof(*node));
	if (node) {
		node->key = key;
		node->kind = kind;
		node->next = nodes;
		nodes = node;
	}
	unlock_nodes();
	return node;
}

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
typedef int (*once_fn)(pthread_once_t *, void (*)(void));
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
static once_fn p_once;
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

static void resolve_native(void)
{
#define RESOLVE(member, name) \
	do { \
		if (!p_##member) \
			p_##member = (member##_fn)dlsym(RTLD_NEXT, name); \
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
	RESOLVE(once, "pthread_once");
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
}

static struct pthread_node *mutex_node(void *object)
{
	return find_node(object, 1, 1);
}

static struct pthread_node *cond_node(void *object)
{
	return find_node(object, 2, 1);
}

static struct pthread_node *rwlock_node(void *object)
{
	return find_node(object, 5, 1);
}

static struct pthread_node *sem_node(void *object)
{
	return find_node(object, 6, 1);
}

static struct pthread_node *attr_node(void *object)
{
	return find_node(object, 7, 1);
}

static struct pthread_node *rwlockattr_node(void *object)
{
	return find_node(object, 8, 1);
}

/*
 * A glibc static recursive mutex has kind == 1 at offset 16.  A zeroed
 * glibc mutex is a normal mutex and is also a valid zeroed musl mutex.  The
 * distinction matters because NVIDIA and its C++ runtime use recursive
 * static locks during compiler initialization.
 */
static void init_static_mutex(struct pthread_node *node, pthread_mutex_t *m)
{
	unsigned kind;
	pthread_mutexattr_t attr;
	int type;

	if (!node || node->initialized)
		return;

	kind = ((unsigned *)m)[4] & 3;
	if (kind == PTHREAD_MUTEX_NORMAL) {
		node->initialized = 1;
		return;
	}

	type = kind == 1 ? PTHREAD_MUTEX_RECURSIVE :
	       kind == 2 ? PTHREAD_MUTEX_ERRORCHECK : PTHREAD_MUTEX_NORMAL;
	if (p_mutexattr_init && p_mutexattr_settype && p_mutex_init) {
		p_mutexattr_init(&attr);
		p_mutexattr_settype(&attr, type);
		p_mutex_init(&node->u.mutex, &attr);
		p_mutexattr_destroy(&attr);
	}
	node->initialized = 1;
}

int pthread_mutexattr_init(pthread_mutexattr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 3, 1);
	return node ? p_mutexattr_init(&node->u.mutex_attr) : ENOMEM;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 3, 0);
	return node ? p_mutexattr_destroy(&node->u.mutex_attr) : 0;
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *a, int protocol)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 3, 1);
	return node ? p_mutexattr_setprotocol(&node->u.mutex_attr, protocol) : ENOMEM;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *a, int shared)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 3, 1);
	return node ? p_mutexattr_setpshared(&node->u.mutex_attr, shared) : ENOMEM;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 3, 1);
	return node ? p_mutexattr_settype(&node->u.mutex_attr, type) : ENOMEM;
}

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
	struct pthread_node *node, *attr_node_ptr = NULL;
	int result;

	resolve_native();
	node = mutex_node(m);
	if (!node)
		return ENOMEM;
	if (a)
		attr_node_ptr = find_node((void *)a, 3, 0);
	result = p_mutex_init(&node->u.mutex,
			      attr_node_ptr ? &attr_node_ptr->u.mutex_attr : NULL);
	if (!result)
		node->initialized = 1;
	return result;
}

int pthread_mutex_destroy(pthread_mutex_t *m)
{
	struct pthread_node *node;

	resolve_native();
	node = mutex_node(m);
	return node && node->initialized ? p_mutex_destroy(&node->u.mutex) : 0;
}

int pthread_mutex_lock(pthread_mutex_t *m)
{
	struct pthread_node *node;

	resolve_native();
	node = mutex_node(m);
	init_static_mutex(node, m);
	return node ? p_mutex_lock(&node->u.mutex) : ENOMEM;
}

int pthread_mutex_trylock(pthread_mutex_t *m)
{
	struct pthread_node *node;

	resolve_native();
	node = mutex_node(m);
	init_static_mutex(node, m);
	return node ? p_mutex_trylock(&node->u.mutex) : ENOMEM;
}

int pthread_mutex_unlock(pthread_mutex_t *m)
{
	struct pthread_node *node;

	resolve_native();
	node = mutex_node(m);
	init_static_mutex(node, m);
	return node ? p_mutex_unlock(&node->u.mutex) : 0;
}

int pthread_condattr_init(pthread_condattr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 4, 1);
	return node ? p_condattr_init(&node->u.cond_attr) : ENOMEM;
}

int pthread_condattr_destroy(pthread_condattr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 4, 0);
	return node ? p_condattr_destroy(&node->u.cond_attr) : 0;
}

int pthread_condattr_setclock(pthread_condattr_t *a, clockid_t clock)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 4, 1);
	return node ? p_condattr_setclock(&node->u.cond_attr, clock) : ENOMEM;
}

int pthread_condattr_setpshared(pthread_condattr_t *a, int shared)
{
	struct pthread_node *node;

	resolve_native();
	node = find_node(a, 4, 1);
	return node ? p_condattr_setpshared(&node->u.cond_attr, shared) : ENOMEM;
}

int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a)
{
	struct pthread_node *node, *attr_node_ptr = NULL;

	resolve_native();
	node = cond_node(c);
	if (!node)
		return ENOMEM;
	if (a)
		attr_node_ptr = find_node((void *)a, 4, 0);
	return p_cond_init(&node->u.cond,
			   attr_node_ptr ? &attr_node_ptr->u.cond_attr : NULL);
}

int pthread_cond_destroy(pthread_cond_t *c)
{
	struct pthread_node *node;

	resolve_native();
	node = cond_node(c);
	return node ? p_cond_destroy(&node->u.cond) : 0;
}

int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	struct pthread_node *cond, *mutex;

	resolve_native();
	cond = cond_node(c);
	mutex = mutex_node(m);
	init_static_mutex(mutex, m);
	return cond && mutex ? p_cond_wait(&cond->u.cond, &mutex->u.mutex) : ENOMEM;
}

int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m,
			   const struct timespec *at)
{
	struct pthread_node *cond, *mutex;

	resolve_native();
	cond = cond_node(c);
	mutex = mutex_node(m);
	init_static_mutex(mutex, m);
	return cond && mutex ?
		p_cond_timedwait(&cond->u.cond, &mutex->u.mutex, at) : ENOMEM;
}

int pthread_cond_signal(pthread_cond_t *c)
{
	struct pthread_node *node;

	resolve_native();
	node = cond_node(c);
	return node ? p_cond_signal(&node->u.cond) : 0;
}

int pthread_cond_broadcast(pthread_cond_t *c)
{
	struct pthread_node *node;

	resolve_native();
	node = cond_node(c);
	return node ? p_cond_broadcast(&node->u.cond) : 0;
}

int pthread_rwlock_init(pthread_rwlock_t *r, const pthread_rwlockattr_t *a)
{
	struct pthread_node *node, *attr_node_ptr = NULL;

	resolve_native();
	node = rwlock_node(r);
	if (a)
		attr_node_ptr = find_node((void *)a, 8, 0);
	return node ? p_rwlock_init(&node->u.rwlock,
				    attr_node_ptr ?
				    &attr_node_ptr->u.rwlock_attr : NULL) : ENOMEM;
}

int pthread_rwlock_destroy(pthread_rwlock_t *r)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlock_node(r);
	return node ? p_rwlock_destroy(&node->u.rwlock) : 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *r)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlock_node(r);
	return node ? p_rwlock_rdlock(&node->u.rwlock) : ENOMEM;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *r)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlock_node(r);
	return node ? p_rwlock_wrlock(&node->u.rwlock) : ENOMEM;
}

int pthread_rwlock_unlock(pthread_rwlock_t *r)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlock_node(r);
	return node ? p_rwlock_unlock(&node->u.rwlock) : 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *r)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlock_node(r);
	return node ? p_rwlock_tryrdlock(&node->u.rwlock) : ENOMEM;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *r)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlock_node(r);
	return node ? p_rwlock_trywrlock(&node->u.rwlock) : ENOMEM;
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlockattr_node(a);
	return node ? p_rwlockattr_init(&node->u.rwlock_attr) : ENOMEM;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlockattr_node(a);
	return node ? p_rwlockattr_destroy(&node->u.rwlock_attr) : 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *a, int shared)
{
	struct pthread_node *node;

	resolve_native();
	node = rwlockattr_node(a);
	return node ? p_rwlockattr_setpshared(&node->u.rwlock_attr, shared) : ENOMEM;
}

int pthread_once(pthread_once_t *once, void (*init)(void))
{
	resolve_native();
	return p_once(once, init);
}

int pthread_attr_init(pthread_attr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_init(&node->u.attr) : ENOMEM;
}

int pthread_attr_destroy(pthread_attr_t *a)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_destroy(&node->u.attr) : 0;
}

int pthread_attr_setstacksize(pthread_attr_t *a, size_t size)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setstacksize(&node->u.attr, size) : ENOMEM;
}

int pthread_attr_setdetachstate(pthread_attr_t *a, int state)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setdetachstate(&node->u.attr, state) : ENOMEM;
}

int pthread_attr_setinheritsched(pthread_attr_t *a, int inherit)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setinheritsched(&node->u.attr, inherit) : ENOMEM;
}

int pthread_attr_setschedparam(pthread_attr_t *a,
			       const struct sched_param *param)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setschedparam(&node->u.attr, param) : ENOMEM;
}

int pthread_attr_setschedpolicy(pthread_attr_t *a, int policy)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setschedpolicy(&node->u.attr, policy) : ENOMEM;
}

int pthread_attr_setscope(pthread_attr_t *a, int scope)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setscope(&node->u.attr, scope) : ENOMEM;
}

int pthread_attr_setstack(pthread_attr_t *a, void *stack, size_t size)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setstack(&node->u.attr, stack, size) : ENOMEM;
}

int pthread_attr_setguardsize(pthread_attr_t *a, size_t size)
{
	struct pthread_node *node;

	resolve_native();
	node = attr_node(a);
	return node ? p_attr_setguardsize(&node->u.attr, size) : ENOMEM;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *a,
		   void *(*start)(void *), void *arg)
{
	struct pthread_node *node = NULL;

	resolve_native();
	if (a)
		node = find_node((void *)a, 7, 0);
	return p_create(thread, node ? &node->u.attr : NULL, start, arg);
}

int pthread_join(pthread_t thread, void **result)
{
	resolve_native();
	return p_join(thread, result);
}

int pthread_detach(pthread_t thread)
{
	resolve_native();
	return p_detach(thread);
}

int sem_init(sem_t *s, int shared, unsigned value)
{
	struct pthread_node *node;
	int result;

	resolve_native();
	node = sem_node(s);
	if (!node)
		return ENOMEM;
	result = p_sem_init(&node->u.sem, shared, value);
	if (!result)
		node->initialized = 1;
	return result;
}

int sem_destroy(sem_t *s)
{
	struct pthread_node *node;

	resolve_native();
	node = sem_node(s);
	return node && node->initialized ? p_sem_destroy(&node->u.sem) : 0;
}

int sem_post(sem_t *s)
{
	struct pthread_node *node;

	resolve_native();
	node = sem_node(s);
	return node && node->initialized ? p_sem_post(&node->u.sem) : -1;
}

int sem_wait(sem_t *s)
{
	struct pthread_node *node;

	resolve_native();
	node = sem_node(s);
	return node && node->initialized ? p_sem_wait(&node->u.sem) : -1;
}

int sem_trywait(sem_t *s)
{
	struct pthread_node *node;

	resolve_native();
	node = sem_node(s);
	return node && node->initialized ? p_sem_trywait(&node->u.sem) : -1;
}

int sem_timedwait(sem_t *s, const struct timespec *at)
{
	struct pthread_node *node;

	resolve_native();
	node = sem_node(s);
	return node && node->initialized ? p_sem_timedwait(&node->u.sem, at) : -1;
}
