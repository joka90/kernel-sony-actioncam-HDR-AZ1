/* 2011-09-05: File added and changed by Sony Corporation */
/*
 * PARTNER-Jet Linux support patch by KMC
 *
 * 2007.02.08 BUGFIX for Application-Mode Debug
 * __kmc_sys_getpidi() and __kmc_do_exit(), changed Label to function
 * 2007.10.01 support SMP
 *            change __KMC_CHECK_PTRACE_REQUEST, specified for ARM-V6
 * 2007.10.19 support Process Trace
 * Ver2.0	08.04.14
 *
 */


#ifndef	__KMC_H__ /* { */
#define	__KMC_H__

#include "__brk_code.h"

#ifdef CONFIG_KMC_PATCH /* { */

// Add 2007.10.12
#ifdef CONFIG_KMC_EVENTTRACKER_SUPPORT /* { */		// 2007/10/19 Support Process Trace
#define KMC_LINUX_EVENTID_NONE		(0)
#define KMC_LINUX_EVENTID_TASKSWITCH	(1)
#define KMC_LINUX_EVENTID_FORK		(2)
#define KMC_LINUX_EVENTID_EXEC		(3)
#define KMC_LINUX_EVENTID_NEWTHREAD	(4)
#define KMC_LINUX_EVENTID_EXIT		(5)

struct task_struct;

void __kmc_linux_event_set_logI(int evtid, int pid, int param);
void __kmc_linux_event_set_logS(int evtid, int pid, const char* pname);
void __kmc_linux_event_set_logT(int evtid, struct task_struct* t);
void __kmc_linux_event_set_Pname(int pid, const char* pname);

#endif /* } CONFIG_KMC_EVENTTRACKER_SUPPORT */

#if defined(CONFIG_KMC_LINUX_PROFILE) || defined(CONFIG_KMC_EVENTTRACKER_SUPPORT) || defined(CONFIG_KMC_NO_USER_PATCH) /* { */
void __kmc_make_fork_tbl(struct task_struct *task_p, int mode, unsigned long param);
#define	__KMC_MAKE_FORK_TBL(cur,n,fn)	__kmc_make_fork_tbl(cur,n,fn)
#else /* }{ */
#define	__KMC_MAKE_FORK_TBL(cur,n,fn)
#endif /* } */

#if defined(CONFIG_KMC_MODULE_DEBUG_NEW) /* { */
#define	__KMC_MODULE_DEBUG_NEW()					\
		void	*__kmc_mod_text;				\
		void	*__kmc_mod_data;				\
		void	*__kmc_mod_bss;					\
		void	*__kmc_mod_exit_text;				\
		void	*__kmc_mod_exit_data;				\
		void	*__kmc_mod_init_text;				\
		void	*__kmc_mod_init_data;				\

#else /* }{ !CONFIG_KMC_MODULE_DEBUG_NEW */
#define	__KMC_MODULE_DEBUG_NEW()
#endif /* } CONFIG_KMC_MODULE_DEBUG_NEW */

#ifdef MODULE /* { */
#if !defined(CONFIG_KMC_MODULE_DEBUG_NEW) /* { */
// When warning is given, please define '__KMC_UNDEF_MODULE_INIT__'
#define __KMC_UNDEF_MODULE_INIT__

// Please define '__KMC_DUMMY_KMC_DRIVER' to evade deletion by optimization
#define	__KMC_DUMMY_KMC_DRIVER

#if !defined(NULL)
#define	NULL	0
#endif

#if defined(CONFIG_KMC_MODULE_AUTO) && defined(__KMC_MODULE_DEBUG) /* { */

#include <linux/version.h>

void __kmc_module_debug_start(void);
void __kmc_module_debug_end(void);

void __kmc_driver_start(void);
void __kmc_driver_end(void);
void __kmc_driver_init_end(void);

char __kmc_driver_tmp __attribute__ ((section (".bss"))) ;

#if defined(__KMC_MODULE_NAME) /* { */
static char __kmc_driver_name[] = __KMC_MODULE_NAME;
#else /* }{ */
static char __kmc_driver_name[] = __KMC_OBJ_NAME;
#endif /* } defined(__KMC_MODULE_NAME) */

#ifdef CONFIG_KMC_ICE_CHK_MOD_DEB /* { */
 #define __KMC_ICE_CHK()						\
    {int ret;								\
    extern asmlinkage int sys_ptrace(long request, long pid, long addr, long data);									\
    if(0x434D4B00 != (0xffffff00&(ret=sys_ptrace(0x4B4D4300,0,0,0)))){	\
	return;								\
    }									\
    if(0 == (0x000000ff&ret)){						\
	return;								\
    }									\
    }
#else /* }{ !CONFIG_KMC_ICE_CHK_MOD_DEB */
 #define __KMC_ICE_CHK()
#endif /* } CONFIG_KMC_ICE_CHK_MOD_DEB */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) /* { */
static void __kmc_driver(void)
{
    asm("   .text");
    asm("   .global   __kmc_driver_end");
    asm("   .long   0x4c434d83");
    asm("__kmc_driver_init_end:");
    __KMC_BRK_CODE();
    asm("   .long   0x4c434d82");
    asm("__kmc_driver_end:");
    __KMC_BRK_CODE();
    asm("   .long   0x4c434d81");
    asm("__kmc_driver_start:");
    __KMC_BRK_CODE();
    asm("   .long   0x4c434d80");
    asm("   .long   __kmc_driver_name");
    asm("   .long   __kmc_driver_tmp");
    asm("   .long   0x4c434d8f");
    asm("   .long   __kmc_init_text");
    asm("   .long   __kmc_exit_text");
    asm("   .long   __kmc_init_data");
    asm("   .long   __kmc_exit_data");

#ifdef __KMC_DUMMY_KMC_DRIVER
    {
	volatile static char *__kmc_dummy_2 = __kmc_driver_name;
	while (NULL != __kmc_dummy_2);
    }
#endif

}

__exitdata int __kmc_exit_data;
__initdata int __kmc_init_data;

void __init __kmc_init_text(void)
{
    __KMC_ICE_CHK();
    __kmc_driver_start();
}

void __init __kmc_init_text_finish(void)
{
    __kmc_driver_init_end();
}

void __exit __kmc_exit_text(void)
{
    __KMC_ICE_CHK();
    __kmc_driver_end();
}

#define	__kmc_module_debug_start	__kmc_init_text
#define	__kmc_module_debug_end		__kmc_exit_text

#else /* }{ LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) */
static void __kmc_driver(void)
{
    asm("   .text");
    asm("   .long   0x4c434d82");
    asm("__kmc_driver_end:");
    __KMC_BRK_CODE();
    asm("   .long   0x4c434d81");
    asm("__kmc_driver_start:");
    __KMC_BRK_CODE();
    asm("   .long   0x4c434d80");
    asm("   .long   __kmc_driver_name");
    asm("   .long   __kmc_driver_tmp");
}

void
__kmc_module_debug_start(void)
{
    __KMC_ICE_CHK();
    __kmc_driver_start();
}

void
__kmc_module_debug_end(void)
{
    __KMC_ICE_CHK();
    __kmc_driver_end();
}
#endif /* } LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

#ifdef __KMC_UNDEF_MODULE_INIT__
#undef	module_init
#undef	module_exit
#endif
#ifdef __KMC_DUMMY_KMC_DRIVER
void (*__kmc_dummy_1)(void) =__kmc_driver;
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) /* { */
#define module_init(initfn)						\
	int init_module(void)						\
	{int ret; __kmc_init_text(); ret = initfn();			\
	__kmc_init_text_finish(); return ret;}
#define module_exit(exitfn)						\
	void cleanup_module(void)					\
	{ exitfn(); __kmc_exit_text(); }

#else /* }{ LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) */
#define module_init(x)							\
	int init_module(void)						\
	{ __kmc_module_debug_start(); return x(); }			\
	static inline __init_module_func_t __init_module_inline(void)	\
	{ return x; }
#define module_exit(x)							\
	void cleanup_module(void)					\
	{ x(); __kmc_module_debug_end(); }				\
	static inline __cleanup_module_func_t __cleanup_module_inline(void) \
	{ return x; }
#endif /* } LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

#endif /* } defined(CONFIG_KMC_MODULE_AUTO) && defined(__KMC_MODULE_DEBUG) */

#endif	/* } !CONFIG_KMC_MODULE_DEBUG_NEW */
#else /* }{ !MODULE */

/* ########## kernel/exit.c { ########## */
extern void __kmc_do_exit(void);
extern void __kmc_sys_getpid(void);

#ifdef CONFIG_KMC_PARTNER_COLLECT_THREAD_OFF /* { */
#define	__KMC_MAX_PT_COUNT	32

extern struct task_struct *__kmc_tss_list[__KMC_MAX_PT_COUNT + 1];

static inline void __kmc_do_exit__(struct task_struct *tsk)
{
    int i;
    extern void __kmc_do_exit(void);

#ifdef CONFIG_KMC_EVENTTRACKER_SUPPORT
    __kmc_linux_event_set_logT(KMC_LINUX_EVENTID_EXIT, tsk);
#endif

    if(__kmc_tss_list[0]!=0){
	for(i=1;i<(__KMC_MAX_PT_COUNT + 1);++i){
	    if(__kmc_tss_list[i]==tsk){
		__kmc_tss_list[0]=(struct task_struct *)i;
// Change 2007.02.08
//		  asm("__kmc_do_exit:");
//		  asm("    nop");
		__kmc_do_exit();
	    }
	}
    }
}
#else /* }{ !CONFIG_KMC_PARTNER_COLLECT_THREAD_OFF */
#define	__KMC_MAX_THREAD_COUNT	256
#define	__KMC_MAX_PT_COUNT	4

extern struct task_struct *__kmc_tss_list_array[__KMC_MAX_PT_COUNT + 1][__KMC_MAX_THREAD_COUNT];

#ifdef  CONFIG_SMP
#include        <linux/spinlock.h>
#endif

static inline void __kmc_do_exit__(struct task_struct *tsk)
{
    int i;
    extern void __kmc_do_exit(void);

#ifdef CONFIG_KMC_EVENTTRACKER_SUPPORT
    __kmc_linux_event_set_logT(KMC_LINUX_EVENTID_EXIT, tsk);
#endif

    if(__kmc_tss_list_array[0][0] != 0){
#ifdef CONFIG_SMP
        static spinlock_t __kmc_do_exit_lock=SPIN_LOCK_UNLOCKED;
        spin_lock(&__kmc_do_exit_lock);
#endif
	for(i = 1; i < (__KMC_MAX_PT_COUNT + 1); ++i){
	    int j;
	    struct task_struct **tpp;
	    struct task_struct *tp;

	    tpp = __kmc_tss_list_array[i];
	    for(j = 0; j < __KMC_MAX_THREAD_COUNT; ++j){
		tp = *(tpp++);
		if(tp == 0) break;
		if(tp == tsk){
		    while(j < (__KMC_MAX_THREAD_COUNT - 1)){
			if(*tpp == 0) break;
			*(tpp -1 ) = *tpp;
			*tpp = tsk;
			++tpp; ++j;
		    }
		    __kmc_tss_list_array[0][0] = (struct task_struct *)i;
		    __kmc_tss_list_array[0][1] = (struct task_struct *)j;
// Change 2007.02.08
//		    asm("__kmc_do_exit:");
//		    asm("    nop");
		    __kmc_do_exit();
		    i = (__KMC_MAX_PT_COUNT + 1);
		    break;
		}
	    }
	}
#ifdef CONFIG_SMP
        spin_unlock(&__kmc_do_exit_lock);
#endif
    }
}
#endif /* } CONFIG_KMC_PARTNER_COLLECT_THREAD_OFF */
#define	__KMC_DO_EXIT(t)	__kmc_do_exit__(t)
/* ########## kernel/exit.c } ########## */

/* ########## kernel/timer.c { ########## */
#ifdef CONFIG_KMC_PARTNER_VIRTUAL_ICE_OFF /* { */
#define	__KMC_SYS_GETPID()
#else /* }{ !CONFIG_KMC_PARTNER_VIRTUAL_ICE_OFF */
// Change 2007.02.08
/*
#define	__KMC_SYS_GETPID()					\
	{							\
		extern int __kmc_sys_getpid_flag;		\
		extern void __kmc_sys_getpid(void);		\
		if (__kmc_sys_getpid_flag) {			\
		    asm("__kmc_sys_getpid:");			\
		    asm("    nop");				\
		}						\
	}
*/
#ifdef  CONFIG_SMP
#define __KMC_SYS_GETPID()                                              \
        {                                                               \
                extern int __kmc_sys_getpid_flag;                       \
                extern void __kmc_sys_getpid(void);                     \
                if (__kmc_sys_getpid_flag==(*(int *)(&current->cpus_allowed))) {        \
                        __kmc_sys_getpid();                             \
                }                                                       \
        }
#else
#define	__KMC_SYS_GETPID()						\
	{								\
		extern int __kmc_sys_getpid_flag;			\
		extern void __kmc_sys_getpid(void);			\
		if (__kmc_sys_getpid_flag) {				\
			__kmc_sys_getpid();				\
		}							\
	}
#endif
#endif /* } CONFIG_KMC_PARTNER_VIRTUAL_ICE_OFF */
/* ########## kernel/timer.c } ########## */

/* ########## kernel/sched.c { ########## */
#ifdef CONFIG_KMC_TRACE_EXT_OFF /* { */
#define	__KMC_SCHED_CALL(p,n)
#else /* }{ !CONFIG_KMC_TRACE_EXT_OFF */
#define KMC_MAX_SHEDULE_LIST    1024
#ifdef CONFIG_MN10300 /* { */
#define	__KMC_SCHED_CALL(p,n)						\
	{								\
		extern void (*__kmc_schedule_call)(struct task_struct *,struct task_struct *);								\
		*(long *)(0xC00001C0) |= 0x01000000;			\
		__kmc_schedule_call(p,n);				\
	}
#else /* }{ !CONFIG_MN10300 */
#define	__KMC_SCHED_CALL(p,n)						\
	{								\
		extern void (*__kmc_schedule_call)(struct task_struct *,struct task_struct *);								\
		__kmc_schedule_call(p,n);				\
	}
#endif /* } CONFIG_MN10300 */
#endif /* CONFIG_KMC_TRACE_EXT_OFF } */
/* ########## kernel/sched.c } ########## */

/* ########## kernel/ksyms.c { ########## */
#ifdef CONFIG_KMC_ICE_CHK_MOD_DEB /* { */
#define	__KMC_EXPORT_SYMBOL_PTRACE()					\
	extern asmlinkage int sys_ptrace(long request, long pid, long addr, long data);									\
	EXPORT_SYMBOL(sys_ptrace)
#else /* }{ !CONFIG_KMC_ICE_CHK_MOD_DEB */
#define	__KMC_EXPORT_SYMBOL_PTRACE()
#endif /* } CONFIG_KMC_ICE_CHK_MOD_DEB */
/* ########## kernel/ksyms.c } ########## */

/* ########## arch/XXX/kernel/ptrace.c { ########## */
#ifdef CONFIG_KMC_PARTNER_AVAILABLE_OFF /* { */
#define	__KMC_CHECK_PTRACE_REQUEST(req,r)
#else /* }{ !CONFIG_KMC_PARTNER_AVAILABLE_OFF */
#if defined(CONFIG_ARM) && defined(CONFIG_CPU_V6)
#define __KMC_CHECK_PTRACE_REQUEST(req,r)		\
		if(req == 0x4B4D4300){			\
			__asm__ __volatile__("mrc p14, 0, %0, c0, c1, 0": "=r" (r) : : "cc"); \
			ret = (ret & 0x4000) ? 1 : 0;	\
			goto out;			\
		}
#else
#define	__KMC_CHECK_PTRACE_REQUEST(req,r)		\
		if(req == 0x4B4D4300){			\
			extern int __kmc_available_ice;	\
			r = __kmc_available_ice;	\
			goto out;			\
		}
#endif
#endif /* } CONFIG_KMC_PARTNER_AVAILABLE_OFF */
/* ########## arch/XXX/kernel/ptrace.c } ########## */

/* ########## init/main.c { ########## */
#ifdef CONFIG_MN10300 /* { */
#define __KMC_CHECK_AVAILABLE_ICE()			\
{							\
	extern int __kmc_available_ice;			\
	if(*(unsigned int *)(0xc00001a0) & 0x00080000)	\
		__kmc_available_ice = 1;		\
}
#else /* }{ !CONFIG_MN10300 */
#define __KMC_CHECK_AVAILABLE_ICE()
#endif /* } CONFIG_MN10300 */
/* ########## init/main.c } ########## */

/* ########## kernel/module.c ########## */
#ifdef CONFIG_KMC_MODULE_DEBUG_NEW
extern char __kmc_debug_module_name[8][64];
extern struct module *__kmc_debug_module[8];
extern void *__kmc_tmp_mod_array[7];

void __kmc_mod_init_brfore(struct module *mod, int index);
void __kmc_mod_init_after(struct module *mod);
void __kmc_mod_exit(struct module *mod);


#define	__KMC_MOD_INIT_BEFORE(mod)								\
	{											\
		int	_i, _st;								\
		for (_i = 0; _i < 8; _i++) {							\
			_st = strnicmp(mod->name, __kmc_debug_module_name[_i], 64);		\
			if (0 == _st) {								\
				__kmc_debug_module[_i] = mod;					\
				__kmc_mod_init_brfore(mod, _i);					\
				break;								\
			} else {								\
				if (strchr(__kmc_debug_module_name[_i], '-')) {			\
					char	__tmp_name[64], *__p;				\
					__p = strcpy(__tmp_name, __kmc_debug_module_name[_i]);	\
					while ((__p = strchr(__p, '-'))) {			\
						*__p = '_';					\
					}							\
					_st = strnicmp(mod->name, __tmp_name, 64);		\
					if (0 == _st) {         				\
						__kmc_debug_module[_i] = mod;   		\
						__kmc_mod_init_brfore(mod, _i); 		\
						break;						\
					}							\
				} 								\
			}									\
		} 										\
	}

#define	__KMC_MOD_INIT_AFTER(mod, ret)	\
	{	\
		int	_i;	\
		for (_i = 0; _i < 8; _i++) {	\
			if (__kmc_debug_module[_i] == mod) {	\
				__kmc_mod_init_after(mod);	\
				if (0 != ret) {			\
					__kmc_mod_exit(mod);	\
					__kmc_debug_module[_i] = (void *)0;	\
				}				\
				break;				\
			}					\
		}	\
	}

#define	__KMC_MOD_EXIT(mod)	\
	{	\
		int	_i;	\
		for (_i = 0; _i < 8; _i++) {	\
			if (__kmc_debug_module[_i] == mod) {	\
				__kmc_mod_exit(mod);	\
				__kmc_debug_module[_i] = (void *)0;	\
				break;				\
			}					\
		}	\
	}

#define	__KMC_CHK_MOD_SEC(mod, secstrings, sechdrs) \
	if (0 == strnicmp(".text", secstrings + sechdrs[i].sh_name, sizeof(".text"))) { \
	 	__kmc_tmp_mod_array[0] = dest; \
	} \
	if (0 == strnicmp(".data", secstrings + sechdrs[i].sh_name, sizeof(".data"))) { \
		__kmc_tmp_mod_array[1] = dest; \
	} \
	if (0 == strnicmp(".bss", secstrings + sechdrs[i].sh_name, sizeof(".bss"))) { \
		__kmc_tmp_mod_array[2] = dest; \
	} \
	if (0 == strnicmp(".exit.text", secstrings + sechdrs[i].sh_name, sizeof(".exit.text"))) { \
		__kmc_tmp_mod_array[3] = dest; \
	} \
	if (0 == strnicmp(".exit.data", secstrings + sechdrs[i].sh_name, sizeof(".exit.data"))) { \
		__kmc_tmp_mod_array[4] = dest; \
	} \
	if (0 == strnicmp(".init.text", secstrings + sechdrs[i].sh_name, sizeof(".init.text"))) { \
		__kmc_tmp_mod_array[5] = dest; \
	} \
	if (0 == strnicmp(".init.data", secstrings + sechdrs[i].sh_name, sizeof(".init.data"))) { \
		__kmc_tmp_mod_array[6] = dest; \
	}

#define	__KMC_SET_MOD_SEC(mod) \
	mod->__kmc_mod_text = __kmc_tmp_mod_array[0]; \
	mod->__kmc_mod_data = __kmc_tmp_mod_array[1]; \
	mod->__kmc_mod_bss = __kmc_tmp_mod_array[2]; \
	mod->__kmc_mod_exit_text = __kmc_tmp_mod_array[3]; \
	mod->__kmc_mod_exit_data = __kmc_tmp_mod_array[4]; \
	mod->__kmc_mod_init_text = __kmc_tmp_mod_array[5]; \
	mod->__kmc_mod_init_data = __kmc_tmp_mod_array[6];

#else
#define	__KMC_MOD_EXIT(mod)
#define	__KMC_MOD_INIT_BEFORE(mod)
#define	__KMC_MOD_INIT_AFTER(mod, ret)
#define	__KMC_CHK_MOD_SEC(mod, secstrings, sechdrs)
#define	__KMC_SET_MOD_SEC(mod)
#endif

#endif /* } MODULE */
#else /* }{ !CONFIG_KMC_PATCH */

#define	__KMC_SCHED_CALL(p,n)
#define	__KMC_DO_EXIT(t)
#define	__KMC_SYS_GETPID()
#define	__KMC_EXPORT_SYMBOL_PTRACE()
#define	__KMC_CHECK_PTRACE_REQUEST(req,r)

#define	__KMC_MOD_EXIT(mod)
#define	__KMC_MOD_INIT_BEFORE(mod)
#define	__KMC_MOD_INIT_AFTER(mod, ret)
#define	__KMC_CHK_MOD_SEC(mod, secstrings, sechdrs)
#define	__KMC_SET_MOD_SEC(mod)

#define	__KMC_MAKE_FORK_TBL(cur,n,fn)

#define	__KMC_CHECK_AVAILABLE_ICE()

#define	__KMC_MODULE_DEBUG_NEW()

#endif /* } CONFIG_KMC_PATCH */


#endif /* } __KMC_H__ */
