/* 2008-02-18: File changed by Sony Corporation */
#ifndef _LINUX_NUMA_H
#define _LINUX_NUMA_H


#ifdef CONFIG_NODES_SHIFT
#define NODES_SHIFT     CONFIG_NODES_SHIFT
#else
#define NODES_SHIFT     0
#endif

#define MAX_NUMNODES    (1 << NODES_SHIFT)

#define	NUMA_NO_NODE	(-1)

#ifdef CONFIG_NUMA
#if MAX_NUMNODES != 4 && MAX_NUMNODES != 8 && MAX_NUMNODES != 16
# error Please specify a right NODES_SHIFT.
#endif
#endif

#endif /* _LINUX_NUMA_H */
