/* 2007-12-21: File changed by Sony Corporation */
#ifndef _ASM_ARM_TOPOLOGY_H
#define _ASM_ARM_TOPOLOGY_H

#include <asm-generic/topology.h>

#ifdef CONFIG_NUMA
#define node_distance(from,to)	(10) /* All nodes with the same distance now */
#define cpu_to_node(cpu)	((void)(cpu),0)
#define node_to_cpumask(node)	((void)node, cpu_online_map)
#define parent_node(node)	((void)(node),0)
#define cpumask_of_node(node)	((void)node, cpu_online_mask)
#define node_to_first_cpu(node)	((void)(node),0)

#define pcibus_to_node(bus)	((void)(bus), -1)
#define pcibus_to_cpumask(bus)	(pcibus_to_node(bus) == -1 ? \
					CPU_MASK_ALL : \
					node_to_cpumask(pcibus_to_node(bus)) \
				)
#define cpumask_of_pcibus(bus)	(pcibus_to_node(bus) == -1 ?		\
				 cpu_all_mask :				\
				 cpumask_of_node(pcibus_to_node(bus)))

/* sched_domains SD_NODE_INIT for ARM machines */
#define SD_NODE_INIT (struct sched_domain) {}
#endif

#endif /* _ASM_ARM_TOPOLOGY_H */
