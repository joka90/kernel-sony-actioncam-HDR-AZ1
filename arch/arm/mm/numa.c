/* 2011-05-27: File added and changed by Sony Corporation */
/*
 * arch/arm/mm/numa.c
 *
 * NUMA support.
 *
 * Copyright 2008-2009 Sony Corporation.
 *
 * Initial code: Copyright (C) 1999-2000 Nicolas Pitre
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/threads.h>
#include <linux/bootmem.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#ifdef CONFIG_SPARSEMEM
#include <asm/sparsemem.h>
#endif
#include <asm/system.h>

static bootmem_data_t node_bootmem_data[MAX_NUMNODES];

pg_data_t node_data[MAX_NUMNODES] = {
  { .bdata = &node_bootmem_data[0] },
  { .bdata = &node_bootmem_data[1] },
  { .bdata = &node_bootmem_data[2] },
  { .bdata = &node_bootmem_data[3] },
#if MAX_NUMNODES == 16 || MAX_NUMNODES == 8
  { .bdata = &node_bootmem_data[4] },
  { .bdata = &node_bootmem_data[5] },
  { .bdata = &node_bootmem_data[6] },
  { .bdata = &node_bootmem_data[7] },
#endif
#if MAX_NUMNODES == 16
  { .bdata = &node_bootmem_data[8] },
  { .bdata = &node_bootmem_data[9] },
  { .bdata = &node_bootmem_data[10] },
  { .bdata = &node_bootmem_data[11] },
  { .bdata = &node_bootmem_data[12] },
  { .bdata = &node_bootmem_data[13] },
  { .bdata = &node_bootmem_data[14] },
  { .bdata = &node_bootmem_data[15] },
#endif
};
/*
 * The line of code below is taken from
 * arch/alpha/mm/numa.c
 */
EXPORT_SYMBOL(node_data);

#ifdef CONFIG_MEMORY_HOTPLUG
pg_data_t *arch_alloc_nodedata(int nid)
{
	return &node_data[nid];
}

void arch_free_nodedata(pg_data_t *pgdat)
{
	return;
}

void arch_refresh_nodedata(int update_node, pg_data_t *update_pgdat)
{
	return;
}
#endif
