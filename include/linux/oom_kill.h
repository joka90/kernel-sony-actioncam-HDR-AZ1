/*
 * include/linux/oom_kill.h
 *
 * OOM callback prototype definitions
 *
 * Copyright 2010, 2011, 2012 Sony Corporation
 */
#ifndef __OOM_KILL_H__
#define __OOM_KILL_H__

#ifdef CONFIG_OOM_CALLBACK
extern int before_oom_kill_task(struct task_struct *p);
extern void after_oom_kill_task(struct task_struct *p);
#else
static inline int before_oom_kill_task(struct task_struct *p) { return SIGKILL;}
static inline void after_oom_kill_task(struct task_struct *p) {}
#endif /* CONFIG_OOM_CALLBACK */

#endif /* !__OOM_KILL_H__ */
