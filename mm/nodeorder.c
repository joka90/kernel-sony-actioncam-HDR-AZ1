/* 2012-11-09: File added and changed by Sony Corporation */
#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/topology.h>
#include <linux/capability.h>
#include <linux/device.h>
#include <linux/memory.h>
#include <linux/kobject.h>
#include <linux/memory_hotplug.h>
#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

/*
 * External interfaces:
 *
 *  struct nodeorder_t nodeorder[MAX_NODE_ORDER_NU]
 *
 *  void __init nodeorder_init(void)
 *  int is_there_ro_rw_node(void)
 *  int is_nodeorder_valid(int no)
 *  void nodeorder_make_zonelists(void)
 *  int nodeorder_matching_node(const char *filepath)
 *  int nodeorder_proc_show(struct seq_file *m, void *v)
 */

#define MATCH_FAIL	-1
#define MATCH_ASTER	1
#define MATCH_EXACT	2

/*
 * Every node order is made up of several nodes,
 * and is associated with two zonelist:
 *   one without RO(Read Only) nodes (ro_exclude_zl),
 *   the other without RW(Read Write) nodes (rw_exclude_zl).
 *
 * When allocating memory for RO requests, rw_exclude_zl should be used.
 * including allocating page cache for program's text.
 *
 * When allocating memory for RW requests, ro_exclude_zl should be used.
 * including allocating anonymous pages and page cache except text.
 */

struct nodeorder_t nodeorder[MAX_NODE_ORDER_NU];
int default_nodeorder = 0;
/*
 * nodeorder will be completed initialized after nodeorder_make_zonelists()
 * is called in init_post() after all memory sections are added. Before
 * this, all page allocation should not use MPOL_BIND_ORDER mempolicy. Use
 * default mempolicy instead.
 */
int is_nodeorder_init_finished = 0;
static int nodero[MAX_NODES_PER_ORDER];
static int noderw[MAX_NODES_PER_ORDER];
static struct file_nodeorder_t file_nodeorder[MAX_FILE_NODEORDER];

void __init nodeorder_init(void)
{
	int i;

        /*
	 * Initialize all elements to -1, meaning not available.
	 *
	 * Things such as pure_initcall() and arch_initcall() are proven to
	 * be not suitable for this, because they are called too late (in
	 * kernel_init), while we need to call this init function before
	 * kernel parameters are parsed.
	 *
	 */
	for (i = 0; i < MAX_NODE_ORDER_NU; i++) {
		memset(nodeorder[i].node_num, -1, sizeof(nodeorder[i].node_num));
		nodeorder[i].ro_exclude_zl = NULL;
		nodeorder[i].rw_exclude_zl = NULL;
	}
	memset(nodeorder, -1, sizeof(nodeorder));
	memset(nodero, -1, sizeof(nodero));
	memset(noderw, -1, sizeof(noderw));
	for (i = 0; i < MAX_FILE_NODEORDER; i++) {
		file_nodeorder[i].pattern[0] = '\0';
		file_nodeorder[i].nodeorder = -1;
	}
}

/*
 * Return is there any ro/rw node specified by nodero= and noderw= kernel
 * parameter.
 */
int is_there_ro_rw_node(void)
{
	if (nodero[0] == -1 && noderw[0] == -1)
		return 0;
	else
		return 1;
}

/*
 * Check whether a nodeorder provided by user space is valid or not.
 * Return 1 if valid, 0 if not.
 */
int is_nodeorder_valid(int no)
{
	if (no >= MAX_NODE_ORDER_NU)
		return 0;
	if (nodeorder[no].node_num[0] == -1)
		return 0;
	return 1;
}

/* Execution too late. Not good. */
/* arch_initcall(nodeorder_init); */

/* Whether element e exists in array a. */
static int exist_in(int e, int a[])
{
	int i;

	for (i = 0; i < MAX_NODES_PER_ORDER; i++)
		if (e == a[i])
			return 1;

	return 0;
}

extern int is_node_specified_by_secgrp(int);
extern void zoneref_set_zone(struct zone *zone, struct zoneref *zoneref);
/*
 * Build a list of nodes for a nodeorder.
 * @no:         Node order number
 * @is_ro_ex:   If non-0, build ro-exclude node list. If 0, build rw_exclude node list
 *
 * For example:
 *   All nodes: 0, 1, 2, 3, 4, 5, 6, 7, 8
 *   nodeorder = 1:3,4,5,6,7,8
 *   nodero = 1,2,3
 *   noderw = 6,7,8
 *
 *   ro-exclude node list will be: 4,5,6,7,8
 *   rw-exclude node list will be: 3,4,5
 */
static int build_node_list(int no, int is_ro_ex)
{
	int i, j;
	int node_num_in_list;
	int nid, num;
	struct zonelist *zl;
	int nodelist[MAX_NODES_PER_ORDER];
	extern enum zone_type policy_zone;

	memset(nodelist, -1, sizeof(nodelist));

	i = j = 0;
	while ((nid = nodeorder[no].node_num[i++]) != -1) {
		if (nid >= MAX_NUMNODES) {
			panic("nodeorder %d contains node with nid %d >= MAX_NUMNODES %d. Stop booting.\n",
			      no, nid, MAX_NUMNODES);
		}

		if (!exist_in(nid, is_ro_ex?nodero:noderw))
			nodelist[j++] = nid;
	}

	node_num_in_list = j;

	zl = kmalloc(sizeof(struct zonelist), GFP_KERNEL | __GFP_ZERO);
	if (!zl)
		return -ENOMEM;

	zl->zlcache_ptr = NULL;

	num = 0;
	for (i = 0; i < node_num_in_list; i++) {
		for (j = MAX_NR_ZONES - 1; j >= 0; j --) {
			struct zone *z = &NODE_DATA(nodelist[i])->node_zones[j];
			if (z->present_pages > 0 ||
			    /* If z is the ZONE_MOVABLE of a node specified
			     * by secgrp=, add it to zonelist even if it's
			     * initial state is offline. */
			    (is_node_specified_by_secgrp(z->zone_pgdat->node_id) &&
			     strcmp(z->name, "Movable") == 0))
				zoneref_set_zone(z, &zl->_zonerefs[num++]);
		}
	}

	if (is_ro_ex) {
		nodeorder[no].ro_exclude_zl = zl;
	} else {
		nodeorder[no].rw_exclude_zl = zl;
	}

	return 0;
}

/* Check whether kernel parameters are OK. */
static void sanity_check(void)
{
	int i, j;
	unsigned int nid;

	/* check whether default nodeorder is properly set */
	if (!is_nodeorder_valid(default_nodeorder)){
		printk(KERN_WARNING "default_nodeorder is not valid. Force it to 0.\n");
		default_nodeorder = 0;
	}

	/* If the same node is in both nodero and noderw, print a warning message */
	i = 0;
	nid = noderw[i];
	while (nid != -1) {
		if (exist_in(nid, nodero)) {
			panic("Warning: node %d is specified by both nodero= and noderw=. Stop booting.\n", nid);
		}
		nid = noderw[++i];
	}
	/* check whether non-exist node is specified in nodeorder */
	for (i = 0; i < MAX_NODE_ORDER_NU; i++) {
		if (nodeorder[i].node_num[0] != -1) {
			j = 0;
			while (nodeorder[i].node_num[j] != -1) {
				nid = nodeorder[i].node_num[j];
				/* If nodeorder has a non-exist node, ignore it. */
				if (!node_online(nid) && !is_node_specified_by_secgrp(nid)) {
					panic("nodeorder %d has non-exist node %d. Stop booting.\n",
					       i, nid);
				}
				j ++;
			}
		}
	}

	/* check whether filenodeorder is a valid nodeorder */
	for (i = 0; i < MAX_FILE_NODEORDER; i++) {
		if (file_nodeorder[i].pattern[0] != '\0')
			if(!is_nodeorder_valid(file_nodeorder[i].nodeorder)) {
				panic("file_nodeorder %s:%d is not valid. Stop booting.\n",
				       file_nodeorder[i].pattern, file_nodeorder[i].nodeorder);
			}
	}
}

/*
 * Build zone list for all node orders.
 * When this fuction is called, all kernel parameter parsing should have been finished.
 */
void nodeorder_make_zonelists(void)
{
	int i = 0;
	int tmp_n;

	sanity_check();

	/*
	 * nodeorder 0 should include all available nodes, which will be
	 * used as default nodeorder when default nodeorder is not
	 * specified.
	 */
	for_each_node(tmp_n) {
		if (node_online(tmp_n) || is_node_specified_by_secgrp(tmp_n))
			nodeorder[0].node_num[i++] = tmp_n;
	}

	for (i = 0; i < MAX_NODE_ORDER_NU; i++) {

		if (nodeorder[i].node_num[0] != -1) {
			build_node_list(i, 1);
			build_node_list(i, 0);
		}
	}

	is_nodeorder_init_finished = 1;
}

/*
 * Whether a filename matches a pattern.Return matching level on success,
 * or -1 on failure.
 *
 * A matching level is a number describing how exactly the name is matched.
 * For example:
 *
 *   filename = "abc"
 *   pattern1 = "abc"
 *   pattern2 = "ab*
 *
 *   filename matches both patterns, but pattern1 should have a higher
 *   matching level for it matches more exactly.
 */
static int nodeorder_pattern_match(const char *filename, const char *pattern)
{
        int i = 0, j = 0;
	while (filename[i] != '\0' && pattern[j] != '\0') {
		if (filename[i] == pattern[j]) {
			i ++;
			j ++;
			continue;
		}

		if (pattern[j] == '*') {
			return MATCH_ASTER;
		}

		return MATCH_FAIL; /* doesn't match */
	}

	if (filename[i] == '\0' && pattern[j] == '\0') {
		return MATCH_EXACT;
	} else if (filename[i] == '\0' && pattern[j] == '*') {
		return MATCH_ASTER;
	} else {
		return MATCH_FAIL;
	}
}

/* Return the file name in a full file path. */
static const char *file_name(const char *filepath)
{
	const char *p = filepath;

	while (*filepath != '\0') {
		if (*filepath == '/')
			p = filepath + 1; /* record the last '/' */
		filepath ++;
	}

	return p;
}

/* Whether the filename matches any of pattern registered. Return
 * corresponding nodeorder if matched. Else -1.
 */
int nodeorder_matching_node(const char *filepath)
{
	int i, ml = -1, ml_new, hi_i = 0;

	for (i = 0; i < MAX_FILE_NODEORDER && file_nodeorder[i].pattern[0] != '\0'; i++) {

		ml_new = nodeorder_pattern_match(file_name(filepath), file_nodeorder[i].pattern);

		if (ml_new > ml) {
			ml = ml_new;
			hi_i = i;
		}
	}

	if (ml == -1)
		return NODEORDER_HAS_NO_FILE_ORDER;
	else
		return file_nodeorder[hi_i].nodeorder;
}

/*
 * Parse string p to nodes.
 * The string should be start with a node number and seperate each number with ','.
 * Like: "1,2,3,4,5" or "5,3,1".
 * Result nodes will be stored into node_array[].
 * para_string points to original parameter string, only used to output the error messages.
 */
static int __init parse_nodes(char *p, int node_array[], char *para_string)
{
	int i = 0;

	while (1) {
		node_array[i++] = simple_strtoul(p, &p, 0);

		if (i > MAX_NODES_PER_ORDER) {
			printk(KERN_WARNING "Error in \"%s\": node number exceeded MAX_NODES_PER_ORDER: %d\n",
			       para_string, MAX_NODES_PER_ORDER);
			return -1;
		}

		/* node number should be seperated by ','. */
		if (*p == ',') {
			p ++;
			continue;
		}
		/* Parameter string ends with space or null. */
		if (*p == ' ' || *p == '\0')
			break;
		printk(KERN_WARNING "Error in \"%s\": format incorrect. '%c'(0x%x): ',' or space expected.\n",
		       para_string, *p, *p);
		return -1;
	}

	return 0;
}

static int __init cmdline_parse_nodeorder(char *p)
{
	int i;
	char para[256] = "nodeorder=";
	char *pp = p;

	i = simple_strtoul(p, &p, 0);
	if (i >= MAX_NODE_ORDER_NU) {
		printk(KERN_WARNING "node order %d exceeded MAX_NODE_ORDER_NU: %d\n",
		       i, MAX_NODE_ORDER_NU);
		return 1;
	}

	if (*p != ':') { /* The first seperator should be ':'. */
		printk(KERN_WARNING "node order %d format incorrect. '%c'(0x%x): ':' expected.\n", i, *p, *p );
		return 1;
	}

	p ++;
	parse_nodes(p, nodeorder[i].node_num, strncat(para, pp, 256 - strlen(para) - 1));
	return 1;
}

static int __init cmdline_parse_default_nodeorder(char *p)
{
	default_nodeorder = simple_strtoul(p, &p, 0);
	return 1;
}

static int __init cmdline_parse_nodero(char *p)
{
	char para[256] = "nodero=";
	parse_nodes(p, nodero, strncat(para, p, 256 - strlen(para) - 1));
	return 1;
}

static int __init cmdline_parse_noderw(char *p)
{
	char para[256] = "noderw=";
	parse_nodes(p, noderw, strncat(para, p, 256 - strlen(para) - 1));
	return 1;
}

static int __init cmdline_parse_file_nodeorder(char *p)
{
	static int i = 0;
	int j;

	if (i >= MAX_FILE_NODEORDER) {
		printk(KERN_WARNING "file_order number exceeded MAX_FILE_NODEORDER: %d\n",
		       MAX_FILE_NODEORDER);
		return 0;
	}

	j = 0;
	while (*p != ':' && *p != '\0' && j < MAX_PATTERN_LEN)
		file_nodeorder[i].pattern[j++] = *p++;

	p++; /* skip the ':' */

	file_nodeorder[i].nodeorder = simple_strtoul(p, &p, 0);

	i++;

	return 0;
}
__setup("nodeorder=", cmdline_parse_nodeorder);
__setup("default_nodeorder=", cmdline_parse_default_nodeorder);
__setup("nodero=", cmdline_parse_nodero);
__setup("noderw=", cmdline_parse_noderw);
__setup("file_nodeorder=", cmdline_parse_file_nodeorder);

#ifdef CONFIG_SNSC_USE_NODE_ORDER_DEBUG
int nodeorder_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	struct zonelist *zl;

	for (i = 0; i < MAX_NODE_ORDER_NU; i++)
		if (nodeorder[i].node_num[0] != -1) {
			seq_printf(m, "nodeorder %d: ", i);
			for (j = 0; j < MAX_NODES_PER_ORDER; j++) {
				if (nodeorder[i].node_num[j] != -1)
					seq_printf(m, "%d ", nodeorder[i].node_num[j]);
			}
			seq_printf(m, "\n");
		}

	seq_printf(m, "nodero: ");
	for (i = 0; i < MAX_NODES_PER_ORDER; i++) {
		if (nodero[i] != -1)
			seq_printf(m, "%d ", nodero[i]);
	}
	seq_printf(m, "\n");

	seq_printf(m, "noderw: ");
	for (i = 0; i < MAX_NODES_PER_ORDER; i++) {
		if (noderw[i] != -1)
			seq_printf(m, "%d ", noderw[i]);
	}
	seq_printf(m, "\n");

	seq_printf(m, "default_nodeorder: %d\n\n", default_nodeorder);

	seq_printf(m, "file_nodeorder:\n");
	for(i = 0; i < MAX_FILE_NODEORDER; i++) {
		if (file_nodeorder[i].nodeorder == -1)
			break;
		seq_printf(m, "  %s: %d\n",
			       file_nodeorder[i].pattern,
			       file_nodeorder[i].nodeorder);
	}
	seq_printf(m, "\n");

#define SHOW_ZONE_LIST(is_ro_ex)									\
	zl = (is_ro_ex) ? nodeorder[i].ro_exclude_zl : nodeorder[i].rw_exclude_zl; 			\
	seq_printf(m, "zone lists(%s) of nodeorder %d:\n", (is_ro_ex)?"ro_ex":"rw_ex", i); 		\
	j = 0;												\
	while (zl->_zonerefs[j].zone != NULL) {								\
		seq_printf(m, "\tnode %d, zone %s\n", zl->_zonerefs[j].zone->node, zl->_zonerefs[j].zone->name);	\
		j++;											\
	}

	for (i = 0; i < MAX_NODE_ORDER_NU; i++) {
		if (nodeorder[i].node_num[0] != -1) {
			SHOW_ZONE_LIST(1);
			SHOW_ZONE_LIST(0);
		}
	}

	return 0;
}
#endif
