/* 2012-09-14: File added and changed by Sony Corporation */
/*
 * drivers/base/section_group.c - section group support
 */

#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/topology.h>
#include <linux/capability.h>
#include <linux/device.h>
#include <linux/memory.h>
#include <linux/kobject.h>
#include <linux/memory_hotplug.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/mempolicy.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/setup.h>
#include <linux/freezer.h>
#include <linux/memblock.h>
#include <linux/snsc_boot_time.h>

/*
 * Initialize the sysfs support for section groups...
 */

#define SEC_GRP_CLASS_NAME	"section_group"

struct sysdev_class sec_grp_sysdev_class = {
	.name = SEC_GRP_CLASS_NAME,
};


static const char *sec_grp_uevent_name(struct kset *kset, struct kobject *kobj)
{
#ifdef CONFIG_EJ_SECTION_GROUP_NO_UEVENT
	return NULL;
#else
	return SEC_GRP_CLASS_NAME;
#endif /* CONFIG_EJ_SECTION_GROUP_NO_UEVENT */
}

static int sec_grp_uevent(struct kset *kset, struct kobject *kobj,
		      struct kobj_uevent_env *env)
{
	return 0;
}

static struct kset_uevent_ops sec_grp_uevent_ops = {
	.name		= sec_grp_uevent_name,
	.uevent		= sec_grp_uevent,
};

int __init sec_grp_dev_init(void)
{
	int ret;

	sec_grp_sysdev_class.kset.uevent_ops = &sec_grp_uevent_ops;
	ret = sysdev_class_register(&sec_grp_sysdev_class);

	if (ret)
		printk(KERN_ERR "%s() failed: %d\n", __FUNCTION__, ret);
	return ret;
}

static struct section_group sg = {0, };

static ssize_t show_sec_grp_start(struct sys_device *dev,
				  struct sysdev_attribute *attr, char *buf)
{
	struct sec_grp *sg =
		container_of(dev, struct sec_grp, sysdev);
	return sprintf(buf, "0x%08lx\n", sg->start);
}

static ssize_t show_sec_grp_size(struct sys_device *dev,
				 struct sysdev_attribute *attr, char *buf)
{
	struct sec_grp *sg =
		container_of(dev, struct sec_grp, sysdev);
	return sprintf(buf, "0x%08lx\n", sg->size);
}

static ssize_t show_sec_grp_nid(struct sys_device *dev,
				struct sysdev_attribute *attr, char *buf)
{
	struct sec_grp *sg =
		container_of(dev, struct sec_grp, sysdev);
	return sprintf(buf, "%d\n", sg->nid);
}

static ssize_t show_sec_grp_name(struct sys_device *dev,
				 struct sysdev_attribute *attr, char *buf)
{
	struct sec_grp *sg =
		container_of(dev, struct sec_grp, sysdev);
	return sprintf(buf, "%s\n", sg->name);
}

extern struct memory_block *find_memory_block(struct mem_section *section);
extern int memory_block_change_state(struct memory_block *mem,
				     unsigned long to_state, unsigned long from_state_req);

/* Change state of all the sections in sec_grp.
 * state == SEC_GRP_ONLINE: online
 * state == SEC_GRP_OFFLINE: offline
 */
static int change_sec_grp_state(struct sec_grp *sg, int state)
{
	int ret = 0, old_state, to_state;

	old_state = sg->state;
	if (old_state == state) {
		MEMPLUG_ERR_PRINTK(KERN_WARNING "memplug: section group (nid: %u, size: %lx, start: %lx) already %s\n",
		       sg->nid, sg->size, sg->start, (state == SEC_GRP_OFFLINE) ? "offlined" : "onlined");
		return -EINVAL;
	}
	if (state == SEC_GRP_ONLINE){
		to_state = SEC_GRP_ONLINE;
		ret = online_pages((sg->start) >> PAGE_SHIFT,
				   (sg->size) >> PAGE_SHIFT);
		if(!ret){
			node_set_state(sg->nid, N_NORMAL_MEMORY);
			node_set_online(sg->nid);
		}
	}
	if (state == SEC_GRP_OFFLINE) {
		to_state = SEC_GRP_OFFLINE;
		sg->state = SEC_GRP_GOING_OFFLINE;
#ifdef CONFIG_SNSC_FREEZE_PROCESSES_BEFORE_HOTREMOVE
		{
#ifdef CONFIG_SNSC_PROCESSES_FREEZE_THAW_TIME_MEASUREMENT
			unsigned long long time1, time2, time3, time4;
			unsigned long long time_freeze, time_remove, time_thaw;
			static unsigned long long time_freeze_max = 0, time_freeze_min = -1, time_freeze_total = 0;
			static unsigned long long time_remove_max = 0, time_remove_min = -1, time_remove_total = 0;
			static unsigned long long time_thaw_max = 0,   time_thaw_min = -1,   time_thaw_total = 0;
			static unsigned int counter = 0;
			unsigned long long tmp;
			time1 = sched_clock();
#endif

			ret = freeze_processes();

#ifdef CONFIG_SNSC_PROCESSES_FREEZE_THAW_TIME_MEASUREMENT
			time2 = sched_clock();
#endif

			if (ret != 0)
				MEMPLUG_ERR_PRINTK("%d process(es) not freezed while doing memory hot remove.\n", ret);

			ret = remove_memory(sg->start, sg->size);

#ifdef CONFIG_SNSC_PROCESSES_FREEZE_THAW_TIME_MEASUREMENT
			time3 = sched_clock();
#endif

			thaw_processes();

#ifdef CONFIG_SNSC_PROCESSES_FREEZE_THAW_TIME_MEASUREMENT
			time4 = sched_clock();

			time_freeze = time2 - time1;
			time_remove = time3 - time2;
			time_thaw = time4 -time3;
			if (time_freeze > time_freeze_max)
				time_freeze_max = time_freeze;
			if (time_freeze < time_freeze_min)
				time_freeze_min = time_freeze;
			time_freeze_total += time_freeze;

			if (time_remove > time_remove_max)
				time_remove_max = time_remove;
			if (time_remove < time_remove_min)
				time_remove_min = time_remove;
			time_remove_total += time_remove;

			if (time_thaw > time_thaw_max)
				time_thaw_max = time_thaw;
			if (time_thaw < time_thaw_min)
				time_thaw_min = time_thaw;
			time_thaw_total += time_thaw;

			counter ++;

			printk("counter: %u  unit: ns\n", counter);
			tmp = time_freeze_total;
			do_div(tmp, counter);
			printk("freeze -- min: %11llu\tmax: %11llu\taver: %11llu\n",
			       time_freeze_min, time_freeze_max, tmp);

			tmp = time_remove_total;
			do_div(tmp, counter);
			printk("remove -- min: %11llu\tmax: %11llu\taver: %11llu\n",
			       time_remove_min, time_remove_max, tmp);

			tmp = time_thaw_total;
			do_div(tmp, counter);
			printk("thaw   -- min: %11llu\tmax: %11llu\taver: %11llu\n",
			       time_thaw_min, time_thaw_max, tmp);

#endif
		}
#else
		ret = remove_memory(sg->start, sg->size);
#endif
	}
	if (ret) {
		sg->state = old_state;
		MEMPLUG_ERR_PRINTK("section group %s %s failed.\n",
		       sg->name, (to_state==SEC_GRP_ONLINE) ? "online" : "offline" );
	}
	else {
		sg->state = to_state;
		MEMPLUG_VERBOSE_PRINTK("section group %s %s succeeded.\n",
		       sg->name, (to_state==SEC_GRP_ONLINE) ? "online" : "offline" );
	}

	return ret;
}


static ssize_t show_sec_grp_state(struct sys_device *dev,
				  struct sysdev_attribute *attr, char *buf)
{
	struct sec_grp *sg =
		container_of(dev, struct sec_grp, sysdev);
	ssize_t len = 0;

	switch (sg->state) {
		case SEC_GRP_ONLINE:
			len = sprintf(buf, "online\n");
			break;
		case SEC_GRP_OFFLINE:
			len = sprintf(buf, "offline\n");
			break;
		case SEC_GRP_GOING_OFFLINE:
			len = sprintf(buf, "going-offline\n");
			break;
		default:
			len = sprintf(buf, "ERROR-UNKNOWN-%d\n",
					sg->state);
			WARN_ON(1);
			break;
	}

	return len;
}
static ssize_t
store_sec_grp_state(struct sys_device *dev,
		    struct sysdev_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;

	struct sec_grp *sg =
		container_of(dev, struct sec_grp, sysdev);

	if (!strncmp(buf, "online", min((int)count, 6)))
		ret = change_sec_grp_state(sg, SEC_GRP_ONLINE);
	else if(!strncmp(buf, "offline", min((int)count, 7)))
		ret = change_sec_grp_state(sg, SEC_GRP_OFFLINE);
	else {
		MEMPLUG_ERR_PRINTK(KERN_WARNING "secgrp: parameter format incorrect.\n");
	}

	if (ret)
		return ret;
	return count;
}

static SYSDEV_ATTR(start, 0444, show_sec_grp_start, NULL);
static SYSDEV_ATTR(size, 0444, show_sec_grp_size, NULL);
static SYSDEV_ATTR(nid, 0444, show_sec_grp_nid, NULL);
static SYSDEV_ATTR(name, 0444, show_sec_grp_name, NULL);
static SYSDEV_ATTR(state, 0644, show_sec_grp_state, store_sec_grp_state);

#define sec_grp_create_simple_file(sec_grp, attr_name)	\
	sysdev_create_file(&sec_grp->sysdev, &attr_##attr_name)
extern struct sysdev_class memory_sysdev_class;
void add_sec_grp(void)
{
	int i, err;
	long memblock_err;
	struct sec_grp *sec_grp = NULL;

	BOOT_TIME_ADD1("add_sec_grp:BEGIN");
	for (i = 0; i < sg.group_nr; i++) {
		if (sg.sec_grp[i].nid >= MAX_NUMNODES) {
			panic("section group %s's node id >= MAX_NUMNODES(%d). Stop booting.\n",
			      sg.sec_grp[i].name, MAX_NUMNODES);
		}

		err =add_memory(sg.sec_grp[i].nid, sg.sec_grp[i].start, sg.sec_grp[i].size);
		memblock_err = memblock_add(sg.sec_grp[i].start, sg.sec_grp[i].size);
 		if (err || (memblock_err == -1)) {
 			panic("Onlining section group %s failed. Return value: %d. Stop booting.\n",
			      sg.sec_grp[i].name, err);
 		}

		sec_grp = &sg.sec_grp[i];

		sec_grp -> sysdev.cls = &sec_grp_sysdev_class;
		sec_grp -> sysdev.id = i;
		err = sysdev_register(&sec_grp->sysdev);
		if (!err)
			err = sec_grp_create_simple_file(sec_grp, size);
		if (!err)
			err = sec_grp_create_simple_file(sec_grp, start);
		if (!err)
			err = sec_grp_create_simple_file(sec_grp, nid);
		if (!err)
			err = sec_grp_create_simple_file(sec_grp, name);
		if (!err)
			err = sec_grp_create_simple_file(sec_grp, state);

		if (!err)
			err = sysfs_create_link(&memory_sysdev_class.kset.kobj,
					&sec_grp->sysdev.kobj, sec_grp->name);
		if (err)
			panic("symbol link for section group %s failed.",
				sg.sec_grp[i].name);

		if (sec_grp -> init_state == SEC_GRP_ONLINE)
			change_sec_grp_state(sec_grp, SEC_GRP_ONLINE);

		printk(KERN_INFO "memplug: section group (id: %u, size: %lx, start: %lx)added\n",
		       sg.sec_grp[i].nid, sg.sec_grp[i].size, sg.sec_grp[i].start);
	}
	BOOT_TIME_ADD1("add_sec_grp:END");
}

static int __init cmdline_parse_sec_grp(char *p)
{
	int i;
 	unsigned long size;
	const char *s = p;
	if (sg.group_nr >= MAX_SECTION_GROUP_NR) {
		panic("secgrp=%s: number of secgrp exceeds MAX_SECTION_GROUP_NR(%d). Stop booting.\n",
		      s, MAX_SECTION_GROUP_NR);
	}
	sg.sec_grp[sg.group_nr].state = SEC_GRP_OFFLINE;

	size = memparse(p, &p);
  	if (size == 0) {
  		panic("secgrp=%s: size too small. Stop booting.\n", p);
  	}
	if (size & ((1 << SECTION_SIZE_BITS) -1)) {
		panic("sec_grp=%s: seg_grp size is not multiple of section size.\n", s);
	}
  	sg.sec_grp[sg.group_nr].size = size;

	if (*p == '@')
		sg.sec_grp[sg.group_nr].start = memparse(p + 1, &p) &
			~((1 << SECTION_SIZE_BITS) -1);
	else
		goto L1;
	if (*p == '@')
		p ++;
	else
		goto L1;

	sg.sec_grp[sg.group_nr].nid = simple_strtoul(p, &p, 0);

	if (*p == ':') {
		p ++;
		if ( 0 == strncmp(p, "online", i = strlen("online")))
			sg.sec_grp[sg.group_nr].init_state = SEC_GRP_ONLINE;
		else if (0 == strncmp(p, "offline", i = strlen("offline")))
			sg.sec_grp[sg.group_nr].init_state = SEC_GRP_OFFLINE;
		else
			goto L1;
		p += i;
	}
	else
		goto L1;
	if (*p == ':') {
		p ++;
		i = 0;
		while (*p != ' ' && *p != '\0' && i < NAME_MAX) {
			sg.sec_grp[sg.group_nr].name[i++] = *p++;
		}

	}
	else if (*p == ' ' || *p == '\0') {
		/* name is not a must. If no name is specified,
		   section_group%d is used. */
		snprintf(sg.sec_grp[sg.group_nr].name, NAME_MAX,
			"section_group%ld", sg.group_nr);

	}
	else
		goto L1;

	sg.group_nr ++;
	return 1;
L1:
	printk(KERN_WARNING "secgrp: parameter format incorrect.\n");
	return 1;
}

__setup("secgrp=", cmdline_parse_sec_grp);

int is_nid_in_sec_grp(unsigned long nid)
{
	int i;
	for (i = 0; i < sg.group_nr; i++) {
		if (sg.sec_grp[i].nid == nid)
			return 1;
	}
	return 0;
}

int __init is_range_overlap_with_secgrp(unsigned long start, unsigned long end)
{
	int i;
	for (i = 0; i < sg.group_nr; i++) {
		if (end <= sg.sec_grp[i].start ||
		    start >= (sg.sec_grp[i].start + sg.sec_grp[i].size))
			continue;
		else
			return 1;
	}
	return 0;
}

#ifdef CONFIG_SNSC_USE_NODE_ORDER
/* Return whether a node is specified by secgrp= cmd line,
 * no matter it is online or offline.
 */
int is_node_specified_by_secgrp(int nid)
{
	int i;
	for (i = 0; i < sg.group_nr; i++) {
		if (sg.sec_grp[i].size != 0 &&
		    sg.sec_grp[i].nid == nid)
			return 1;
	}
	return 0;
}

static void count_pages(unsigned long pfn, unsigned long num, unsigned long *vm_stat)
{
	unsigned long end = pfn + num;
	memset(vm_stat, 0, sizeof(unsigned int) * NR_SECGRP_STAT_ITEMS);

	for (; pfn < end; pfn++) {
		struct page *page = pfn_to_page(pfn);
		struct address_space *mapping;
		lock_page(page);
		mapping = page_mapping(page);
		if (PageReserved(page)) {
			/* ignore reserved page.  nothing to do */
		}
		else if (page_count(page) == 0) {
			vm_stat[NR_FREE_PAGES] ++;
		} else if (page->mapping == 0) { /* swap cache */
			vm_stat[NR_FILE_MAPPED] ++;
		} else if ((unsigned long)(page->mapping) & PAGE_MAPPING_ANON) { /* anonymous page */
			vm_stat[NR_ANON_PAGES] ++;
		} else if (!((unsigned long)(page->mapping) & PAGE_MAPPING_ANON)) { /* mapped */
			vm_stat[NR_FILE_MAPPED] ++;
			if (page->mapping->open_type == OPENTYPE_RW)
				vm_stat[NR_NODEORDER_RW] ++;
			else
				vm_stat[NR_NODEORDER_RO] ++;
		}
		unlock_page(page);
	}
}

ssize_t print_secgrp_stat(struct class *class, struct class_attribute *attr, char *buf)
{
	int i;
	unsigned long vm_stat[NR_SECGRP_STAT_ITEMS];
	char *ptr = buf;
	char *end = buf + PAGE_SIZE; /* see fs/sysfs/file.c: fill_read_buffer() */
#ifdef CONFIG_ARM
	extern struct meminfo meminfo;
	for (i = 0; i < meminfo.nr_banks; i++) {
		ptr += scnprintf(ptr, end - ptr, "mem%d\n", i);
		ptr += scnprintf(ptr, end - ptr, "addr: 0x%08lx-0x%08lx\n",
				 (unsigned long)meminfo.bank[i].start,
				 (unsigned long)(meminfo.bank[i].start + meminfo.bank[i].size));
		ptr += scnprintf(ptr, end - ptr, "node: %d\n",
				 meminfo.bank[i].node);

		count_pages(meminfo.bank[i].start >> PAGE_SHIFT,
			    meminfo.bank[i].size >> PAGE_SHIFT,
			    vm_stat);

		ptr += scnprintf(ptr, end - ptr, "total: %ld KB\n",
				 meminfo.bank[i].size >> 10);
		ptr += scnprintf(ptr, end - ptr, "anonymous: %ld KB\n",
				 vm_stat[NR_ANON_PAGES] << (PAGE_SHIFT - 10));
		ptr += scnprintf(ptr, end - ptr, "file_mapped: %ld KB\n",
				 vm_stat[NR_FILE_MAPPED] << (PAGE_SHIFT - 10));
		ptr += scnprintf(ptr, end - ptr, "ro: %ld KB\n",
				 vm_stat[NR_NODEORDER_RO] << (PAGE_SHIFT - 10));
		ptr += scnprintf(ptr, end - ptr, "rw: %ld KB\n\n",
				 vm_stat[NR_NODEORDER_RW] << (PAGE_SHIFT - 10));
	}
#endif
	for (i = 0; i < sg.group_nr; i++) {
		ptr += scnprintf(ptr, end - ptr, "name: %s\n",
				 sg.sec_grp[i].name);
		ptr += scnprintf(ptr, end - ptr, "addr: 0x%08lx-0x%08lx\n",
				 sg.sec_grp[i].start,
				 sg.sec_grp[i].start + sg.sec_grp[i].size);
		ptr += scnprintf(ptr, end - ptr, "node: %d\n",
				 sg.sec_grp[i].nid);

		count_pages(sg.sec_grp[i].start >> PAGE_SHIFT,
			    sg.sec_grp[i].size >> PAGE_SHIFT,
			    vm_stat);

		ptr += scnprintf(ptr, end - ptr, "total: %ld KB\n",
				 sg.sec_grp[i].size >> 10);
		ptr += scnprintf(ptr, end - ptr, "anonymous: %ld KB\n",
				 vm_stat[NR_ANON_PAGES] << (PAGE_SHIFT - 10));
		ptr += scnprintf(ptr, end - ptr, "file_mapped: %ld KB\n",
				 vm_stat[NR_FILE_MAPPED] << (PAGE_SHIFT - 10));
		ptr += scnprintf(ptr, end - ptr, "ro: %ld KB\n",
				 vm_stat[NR_NODEORDER_RO] << (PAGE_SHIFT - 10));
		ptr += scnprintf(ptr, end - ptr, "rw: %ld KB\n\n",
				 vm_stat[NR_NODEORDER_RW] << (PAGE_SHIFT - 10));
	}
	return ptr - buf;
}
#endif
