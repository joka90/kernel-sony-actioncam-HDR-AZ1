/* 2013-08-20: File added by Sony Corporation */
#ifndef _LINUX_RWSEM_RT_H
#define _LINUX_RWSEM_RT_H

#ifndef _LINUX_RWSEM_H
#error "Include rwsem.h"
#endif

/*
 * RW-semaphores are a spinlock plus a reader-depth count.
 *
 * Note that the semantics are different from the usual
 * Linux rw-sems, in PREEMPT_RT mode we do not allow
 * multiple readers to hold the lock at once, we only allow
 * a read-lock owner to read-lock recursively. This is
 * better for latency, makes the implementation inherently
 * fair and makes it simpler as well.
 */

#include <linux/rtmutex.h>

struct rw_semaphore {
	struct rt_mutex		lock;
	int			read_depth;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};

#define __RWSEM_INITIALIZER(name) \
	{ .lock = __RT_MUTEX_INITIALIZER(name.lock), \
	  RW_DEP_MAP_INIT(name) }

#define DECLARE_RWSEM(lockname) \
	struct rw_semaphore lockname = __RWSEM_INITIALIZER(lockname)

extern void  __rt_rwsem_init(struct rw_semaphore *rwsem, char *name,
				     struct lock_class_key *key);

# define rt_init_rwsem(sem)				\
do {							\
	static struct lock_class_key __key;		\
							\
	rt_mutex_init(&(sem)->lock);			\
	__rt_rwsem_init((sem), #sem, &__key);		\
} while (0)

extern void  rt_down_write(struct rw_semaphore *rwsem);
extern void rt_down_read_nested(struct rw_semaphore *rwsem, int subclass);
extern void rt_down_write_nested(struct rw_semaphore *rwsem, int subclass);
extern void  rt_down_read(struct rw_semaphore *rwsem);
extern int  rt_down_write_trylock(struct rw_semaphore *rwsem);
extern int  rt_down_read_trylock(struct rw_semaphore *rwsem);
extern void  rt_up_read(struct rw_semaphore *rwsem);
extern void  rt_up_write(struct rw_semaphore *rwsem);
extern void  rt_downgrade_write(struct rw_semaphore *rwsem);

#ifndef CONFIG_EJ_COMPAT_RW_SEMAPHORE
#define init_rwsem(sem)                rt_init_rwsem(sem)
#define rwsem_is_locked(s)     rt_mutex_is_locked(&(s)->lock)
#endif

static inline void down_read(struct rw_semaphore *sem)
{
	rt_down_read(sem);
}

static inline int down_read_trylock(struct rw_semaphore *sem)
{
	return rt_down_read_trylock(sem);
}

static inline void down_write(struct rw_semaphore *sem)
{
	rt_down_write(sem);
}

static inline int down_write_trylock(struct rw_semaphore *sem)
{
	return rt_down_write_trylock(sem);
}

static inline void up_read(struct rw_semaphore *sem)
{
	rt_up_read(sem);
}

static inline void up_write(struct rw_semaphore *sem)
{
	rt_up_write(sem);
}

static inline void downgrade_write(struct rw_semaphore *sem)
{
	rt_downgrade_write(sem);
}

static inline void down_read_nested(struct rw_semaphore *sem, int subclass)
{
	return rt_down_read_nested(sem, subclass);
}

static inline void down_write_nested(struct rw_semaphore *sem, int subclass)
{
	rt_down_write_nested(sem, subclass);
}

#ifdef CONFIG_EJ_COMPAT_RW_SEMAPHORE
#include "pickop.h"

struct compat_rw_semaphore {
	__s32                   activity;
	spinlock_t              wait_lock;
	struct list_head        wait_list;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
};

#define __COMPAT_RWSEM_INITIALIZER(name) \
	{ 0, SPIN_LOCK_UNLOCKED, LIST_HEAD_INIT((name).wait_list) __RWSEM_DEP_MAP_INIT(name) }

#define COMPAT_DECLARE_RWSEM(name) \
	struct compat_rw_semaphore name = __COMPAT_RWSEM_INITIALIZER(name)

#define rt_rwsem_is_locked(rws)        (rt_mutex_is_locked(&(rws)->lock))

extern void __compat_init_rwsem(struct compat_rw_semaphore *sem, const char *name,
				struct lock_class_key *key);

#define compat_init_rwsem(sem)					\
do {								\
	static struct lock_class_key __key;			\
								\
	__compat_init_rwsem((sem), #sem, &__key);		\
} while (0)

extern void compat_down_write(struct compat_rw_semaphore *sem);
extern void compat_down_read(struct compat_rw_semaphore *sem);
extern int compat_down_read_trylock(struct compat_rw_semaphore *sem);
extern int compat_down_write_trylock(struct compat_rw_semaphore *sem);
extern void compat_up_read(struct compat_rw_semaphore *sem);
extern void compat_up_write(struct compat_rw_semaphore *sem);
extern void compat_downgrade_write(struct compat_rw_semaphore *sem);

extern void __compat_down_read(struct compat_rw_semaphore *sem);
extern int __compat_down_read_trylock(struct compat_rw_semaphore *sem);
extern void __compat_down_write(struct compat_rw_semaphore *sem);
extern void __compat_down_write_nested(struct compat_rw_semaphore *sem, int subclass);
extern int __compat_down_write_trylock(struct compat_rw_semaphore *sem);
extern void __compat_up_read(struct compat_rw_semaphore *sem);
extern void __compat_up_write(struct compat_rw_semaphore *sem);
extern void __compat_downgrade_write(struct compat_rw_semaphore *sem);

static inline int compat_rwsem_is_locked(struct compat_rw_semaphore *sem)
{
	return (sem->activity != 0);
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC
/*
 * nested locking. NOTE: rwsems are not allowed to recurse
 * (which occurs if the same task tries to acquire the same
 * lock instance multiple times), but multiple locks of the
 * same lock class might be taken, if the order of the locks
 * is always the same. This ordering rule can be expressed
 * to lockdep via the _nested() APIs, but enumerating the
 * subclasses that are used. (If the nesting relationship is
 * static then another method for expressing nested locking is
 * the explicit definition of lock class keys and the use of
 * lockdep_set_class() at lock initialization time.
 * See Documentation/lockdep-design.txt for more details.)
 */
extern void
compat_down_read_nested(struct compat_rw_semaphore *sem, int subclass);
extern void
compat_down_write_nested(struct compat_rw_semaphore *sem, int subclass);
/*
 * Take/release a lock when not the owner will release it.
 *
 * [ This API should be avoided as much as possible - the
 *   proper abstraction for this case is completions. ]
 */
#else /* CONFIG_DEBUG_LOCK_ALLOC */
# define compat_down_read_nested(sem, subclass)	compat_down_read(sem)
# define compat_down_write_nested(sem, subclass)	compat_down_write(sem)
#endif /* CONFIG_DEBUG_LOCK_ALLOC */

#define PICK_RWSEM_OP(...) PICK_FUNCTION(struct compat_rw_semaphore *,	 \
					  struct rw_semaphore *, ##__VA_ARGS__)
#define PICK_RWSEM_OP_RET(...) PICK_FUNCTION_RET(struct compat_rw_semaphore *,\
						  struct rw_semaphore *, ##__VA_ARGS__)

#define init_rwsem(rwsem) PICK_RWSEM_OP(compat_init_rwsem, rt_init_rwsem, rwsem)

#define down_read(rwsem) PICK_RWSEM_OP(compat_down_read, rt_down_read, rwsem)

#define down_read_trylock(rwsem) \
	PICK_RWSEM_OP_RET(compat_down_read_trylock, rt_down_read_trylock, rwsem)

#define down_write(rwsem) PICK_RWSEM_OP(compat_down_write, rt_down_write, rwsem)

#define down_read_nested(rwsem, subclass) \
	PICK_RWSEM_OP(compat_down_read_nested, rt_down_read_nested,	\
		      rwsem, subclass)

#define down_write_nested(rwsem, subclass) \
	PICK_RWSEM_OP(compat_down_write_nested, rt_down_write_nested,	\
		      rwsem, subclass)

#define down_write_trylock(rwsem) \
	PICK_RWSEM_OP_RET(compat_down_write_trylock, rt_down_write_trylock,\
			  rwsem)

#define up_read(rwsem) PICK_RWSEM_OP(compat_up_read, rt_up_read, rwsem)

#define up_write(rwsem) PICK_RWSEM_OP(compat_up_write, rt_up_write, rwsem)

#define downgrade_write(rwsem) \
	PICK_RWSEM_OP(compat_downgrade_write, rt_downgrade_write, rwsem)

#define rwsem_is_locked(rwsem) \
	PICK_RWSEM_OP_RET(compat_rwsem_is_locked, rt_rwsem_is_locked, rwsem)

#endif /* EJ_COMPAT_RW_SEMAPHORE */

#endif
