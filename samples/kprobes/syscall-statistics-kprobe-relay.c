/* 2011-01-17: File added and changed by Sony Corporation */
/*
 *  syscall-statistics-kprobe-relay.c - collects the statistics for the
 * 					syscalls in the system and outputs
 * 					to a relayfs channel
 *
 *  Copyright 2008 Sony Corporation.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  The relayfs channel code is referred from relay-apps tar ball from
 *  //http://relayfs.sourceforge.net/
 *  Reference: Documentation/filesystems/relay.txt
 */
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/relay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>

/* This modules's relay channel/control files will be
 * created in /debug/kptool
 */
#define APP_DIR		"kptool"

/* Following set of global data coming from the relay
 * code. The kprobe probes data follows immediately
 * after this
 */
/* app data */
static struct rchan *	chan;
static struct dentry *	dir;
static int		logging;
static int		mappings;
static int		suspended;
static size_t		dropped;
static size_t		subbuf_size = 262144; /* default, can be changed */
static size_t		n_subbufs = 4; /* default, can be changed */

/* channel-management control files */
static struct dentry	*enabled_control;
static struct dentry	*stats_control;
static struct dentry	*create_control;
static struct dentry	*subbuf_size_control;
static struct dentry	*n_subbufs_control;
static struct dentry	*dropped_control;

/* produced/consumed control files */
static struct dentry	*produced_control[NR_CPUS];
static struct dentry	*consumed_control[NR_CPUS];

/* control file fileop declarations */
struct file_operations	enabled_fops;
struct file_operations	stats_fops;
struct file_operations	create_fops;
struct file_operations	subbuf_size_fops;
struct file_operations	n_subbufs_fops;
struct file_operations	dropped_fops;
struct file_operations	produced_fops;
struct file_operations	consumed_fops;

/* forward declarations */
static int create_controls(void);
static void destroy_channel(void);
static void remove_controls(void);
static int ret_handler(struct kretprobe_instance*, struct pt_regs*);

/* locks for kprobe handlers to protect logging buffer */
static DEFINE_SPINLOCK(prebuf_lock);
static DEFINE_SPINLOCK(retbuf_lock);

/* This struct holds the probe point for each entry */
struct probe_info
{
	struct kprobe 		kp;
	long int 		count;
	volatile int 		flag;
	struct kretprobe 	kretp;
	struct timeval		stv1; /* holds syscall start time */
	struct timeval		stv2; /* holds syscall end time */
	struct timeval		stv_all; /* holds all time for each syscall */
};

static int dbg;
module_param(dbg, int, 0);


static struct probe_info probes[]=
{
	#include "syscalls.list"
};

#define NUM (sizeof(probes)/sizeof(struct probe_info))


static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int 		ret = regs_return_value(regs);
	static char 	ret_handler_buf[128];
	unsigned long 	flags;
	int 		scall_timespent = 0;
	time_t		scall_secs = 0;
	suseconds_t	scall_usecs = 0;
	int 		size, i;


	for (i = 0; i < NUM; i++) {
       		if (ri->rp->kp.addr == probes[i].kp.addr) {
			(probes[i].count)++;
			if (dbg == 1) {
				/* get end time */
				do_gettimeofday(&(probes[i].stv2));
				scall_secs = ((probes[i].stv2.tv_sec) - (probes[i].stv1.tv_sec));
				scall_usecs = ((probes[i].stv2.tv_usec) - (probes[i].stv1.tv_usec));
				scall_timespent = ((probes[i].stv2.tv_sec) - (probes[i].stv1.tv_sec))*1000000 + ((probes[i].stv2.tv_usec) - (probes[i].stv1.tv_usec));
				probes[i].stv_all.tv_sec += scall_secs;
				probes[i].stv_all.tv_usec += scall_usecs;
			}
				break;
        	}
	}

	if (logging) {
		spin_lock_irqsave(&retbuf_lock, flags);
		size = sprintf(ret_handler_buf,"%s end for pid(%d) ret value(%d)",
					ri->rp->kp.symbol_name, current->pid, ret);
		relay_write(chan, ret_handler_buf, size);
		if (dbg == 1) {
			size = sprintf(ret_handler_buf,":timespent(uSecs)=%d \n",
					scall_timespent);
			relay_write(chan, ret_handler_buf, size);
		} else {
			size = sprintf(ret_handler_buf,"\n");
			relay_write(chan, ret_handler_buf, size);
		}
		spin_unlock_irqrestore(&retbuf_lock, flags);
	}

	return 0;
}

static int kpr_before_hook(struct kprobe *kpr)
{
	static char 	pre_handler_buf[64];
	unsigned long 	flags;
	int		size;

	if (dbg == 1) {
		if (logging) {
			spin_lock_irqsave(&prebuf_lock, flags);
			size = sprintf(pre_handler_buf, "%s start for pid(%d)\n",
					   kpr->symbol_name, current->pid);
			relay_write(chan, pre_handler_buf, size);
			spin_unlock_irqrestore(&prebuf_lock, flags);
		}
	}
	return 0;
}

static int kpr_after_hook(struct kprobe *kpr , struct pt_regs *p)
{
	int i = 0;

	if (dbg == 1) {
		for (i = 0; i < NUM; i++) {
			if (kpr->addr == probes[i].kp.addr) {
				/* count incr is moved to ret_probe handler */
				/* (probes[i].count)++; */

				/* get start time */
				do_gettimeofday(&(probes[i].stv1));
				break;
			}
		}
	}
	return 0;
}

/* print the syscalls statistics to channel and to screen */
static void print_statistics( void )
{
	int 		index;
	static char 	buf[256];
	int 		size;
	time_t		scall_secs=0;
	suseconds_t	scall_usecs=0;

	memset(buf, 0, sizeof(buf));
	size = sprintf(buf, "========================================================\n");
	relay_write(chan, buf, size);

	memset(buf, 0, sizeof(buf));
	size = sprintf(buf, "\n\nSYSCALLS STATISTICS SUMMARY START\n\n");
	relay_write(chan, buf, size);

	for (index = 0; index < NUM; index++) {
		if ((probes[index].count) != 0) {
			memset(buf, '\0', sizeof(buf));
			size = sprintf(buf, "syscall:%-20s;count=%-6ld;",
				probes[index].kp.symbol_name,
				probes[index].count);
			relay_write(chan, buf, size);
			if (dbg == 1) {
				scall_secs = probes[index].stv_all.tv_sec;
				scall_usecs = probes[index].stv_all.tv_usec;
				size = sprintf(buf, "Time= Secs %-3ld : uSecs %-8ld;\n",
					scall_secs, scall_usecs);
				relay_write(chan, buf, size);
			} else {
				size = sprintf(buf,"\n");
				relay_write(chan, buf, size);
			}
		}
	}
	memset(buf, 0, sizeof(buf));
	size = sprintf(buf, "\n\nSYSCALLS STATISTICS SUMMARY END\n\n");
	relay_write(chan, buf, size);

	memset(buf, 0, sizeof(buf));
	size = sprintf(buf, "========================================================\n");
	relay_write(chan, buf, size);
}


/*
 * init - initialize relay channels and insert syscall probes
 *
 * relay files will be named /debug/kptool/cpuXXX
 */
static int init(void)
{
	int index = 0;

	dir = debugfs_create_dir(APP_DIR, NULL);
	if (!dir) {
		printk("Couldn't create relay app directory.\n");
		return -ENOMEM;
	}

	if (create_controls()) {
		debugfs_remove(dir);
		return -ENOMEM;
	}

	printk("\nInserting the syscalls as probe points START\n");

	for (index = 0; index < NUM; index++) {
		probes[index].kp.pre_handler = (kprobe_pre_handler_t) kpr_before_hook;
		probes[index].kp.post_handler = (kprobe_post_handler_t) kpr_after_hook;

		if (register_kprobe(&(probes[index].kp)) < 0) {
                        printk("[%d] %s probe point registration FAIL\n",
				   index, probes[index].kp.symbol_name);
            		(probes[index].flag) = 1;
            		continue;
        	}
		printk("[%d] %s probe point registration PASS\n",
			   index, probes[index].kp.symbol_name);

       		if(register_kretprobe( &(probes[index].kretp)  ) < 0)
            		return -1;
    	}
	printk("\nInserting the syscalls as probe points COMPLETED\n");

	return 0;
}

/*
 * cleanup - destroy channel and remove syscall probes
 */
static void cleanup(void)
{
	int		index=0;
	time_t		scall_secs=0;
	suseconds_t	scall_usecs=0;


	for (index = 0; index < NUM; index++) {
		if ((probes[index].flag) == 0) {
			unregister_kprobe(&(probes[index].kp));
			unregister_kretprobe(&(probes[index].kretp));
		}
	}

	printk("\n===== syscalls statistics details ===== \n");

	for (index = 0; index < NUM; index++) {
		if ((probes[index].count) != 0) {
			printk("system call = %-20s;count = %-3ld;",
				probes[index].kp.symbol_name, probes[index].count);
			if (dbg == 1) {
				scall_secs = probes[index].stv_all.tv_sec;
				scall_usecs = probes[index].stv_all.tv_usec;
				printk("Time = secs %-3ld : uSecs %-8ld;\n",
					scall_secs, scall_usecs);
			} else {
				printk("\n");
			}

		}
	}

	printk("\nAll syscall probe points are removed.\n");
	destroy_channel();
	remove_controls();
	if (dir)
		debugfs_remove(dir);
}

module_init(init);
module_exit(cleanup);
MODULE_DESCRIPTION("syscall statistics tool using kprobes");
MODULE_LICENSE("GPL");


/* Boilerplate code below here which is taken from relay-apps */

/**
 *	remove_channel_controls - removes produced/consumed control files
 */
static void remove_channel_controls(void)
{
	int i;

	for (i = 0; i < NR_CPUS; i++) {
		if (produced_control[i]) {
			debugfs_remove(produced_control[i]);
			produced_control[i] = NULL;
			continue;
		}
		break;
	}

	for (i = 0; i < NR_CPUS; i++) {
		if (consumed_control[i]) {
			debugfs_remove(consumed_control[i]);
			consumed_control[i] = NULL;
			continue;
		}
		break;
	}
}

/**
 *	create_channel_controls - creates produced/consumed control files
 *
 *	Returns channel on success, negative otherwise.
 */
static int create_channel_controls(struct dentry *parent,
				   const char *base_filename,
				   struct rchan *chan)
{
	unsigned int i;
	char *tmpname = kmalloc(NAME_MAX + 1, GFP_KERNEL);

	if (!tmpname)
		return -ENOMEM;

	for_each_online_cpu(i) {
		sprintf(tmpname, "%s%d.produced", base_filename, i);
		produced_control[i] = debugfs_create_file(tmpname, 0, parent,
					 chan->buf[i], &produced_fops);
		if (!produced_control[i]) {
			printk("Couldn't create relay control file %s.\n",
			       tmpname);
			goto cleanup_control_files;
		}

		sprintf(tmpname, "%s%d.consumed", base_filename, i);
		consumed_control[i] = debugfs_create_file(tmpname, 0, parent,
					 chan->buf[i], &consumed_fops);
		if (!consumed_control[i]) {
			printk("Couldn't create relay control file %s.\n",
			       tmpname);
			goto cleanup_control_files;
		}
	}

	return 0;

cleanup_control_files:
	remove_channel_controls();
	return -ENOMEM;
}

/*
 * subbuf_start() relay callback.
 *
 * Defined so that we can 1) reserve padding counts in the sub-buffers, and
 * 2) keep a count of events dropped due to the buffer-full condition.
 */
static int subbuf_start_handler(struct rchan_buf *buf,
				void *subbuf,
				void *prev_subbuf,
				unsigned int prev_padding)
{
	if (prev_subbuf)
		*((unsigned *)prev_subbuf) = prev_padding;

	if (relay_buf_full(buf)) {
		if (!suspended) {
			suspended = 1;
			printk("cpu %d buffer full!!!\n", smp_processor_id());
		}
		dropped++;
		return 0;
	} else if (suspended) {
		suspended = 0;
		printk("cpu %d buffer no longer full.\n", smp_processor_id());
	}

	subbuf_start_reserve(buf, sizeof(unsigned int));

	return 1;
}

/*
 * file_create() callback.  Creates relay file in debugfs.
 */
static struct dentry *create_buf_file_handler(const char *filename,
					      struct dentry *parent,
					      int mode,
					      struct rchan_buf *buf,
					      int *is_global)
{
	struct dentry *buf_file;

	buf_file = debugfs_create_file(filename, mode, parent, buf,
				       &relay_file_operations);

	return buf_file;
}

/*
 * file_remove() default callback.  Removes relay file in debugfs.
 */
static int remove_buf_file_handler(struct dentry *dentry)
{
	debugfs_remove(dentry);

	return 0;
}

/*
 * relay callbacks
 */
static struct rchan_callbacks relay_callbacks =
{
	.subbuf_start = subbuf_start_handler,
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};

/**
 *	create_channel - creates channel /debug/APP_DIR/cpuXXX
 *
 *	Creates channel along with associated produced/consumed control files
 *
 *	Returns channel on success, NULL otherwise
 */
static struct rchan *create_channel(unsigned subbuf_size,
				    unsigned n_subbufs)
{
	struct rchan *chan;

	chan = relay_open("cpu", dir, subbuf_size,
			  n_subbufs, &relay_callbacks, NULL);

	if (!chan) {
		printk("relay app channel creation failed\n");
		return NULL;
	}

	if (create_channel_controls(dir, "cpu", chan)) {
		relay_close(chan);
		return NULL;
	}

	logging = 0;
	mappings = 0;
	suspended = 0;
	dropped = 0;

	return chan;
}

/**
 *	destroy_channel - destroys channel /debug/APP_DIR/cpuXXX
 *
 *	Destroys channel along with associated produced/consumed control files
 */
static void destroy_channel(void)
{
	if (chan) {
		relay_close(chan);
		chan = NULL;
	}
	remove_channel_controls();
}

/**
 *	remove_controls - removes channel management control files
 */
static void remove_controls(void)
{
	if (enabled_control)
		debugfs_remove(enabled_control);

	if (stats_control)
		debugfs_remove(stats_control);

	if (subbuf_size_control)
		debugfs_remove(subbuf_size_control);

	if (n_subbufs_control)
		debugfs_remove(n_subbufs_control);

	if (create_control)
		debugfs_remove(create_control);

	if (dropped_control)
		debugfs_remove(dropped_control);
}

/**
 *	create_controls - creates channel management control files
 *
 *	Returns 0 on success, negative otherwise.
 */
static int create_controls(void)
{
	enabled_control = debugfs_create_file("enabled", 0, dir,
					      NULL, &enabled_fops);
	if (!enabled_control) {
		printk("Couldn't create relay control file 'enabled'.\n");
		goto fail;
	}

	stats_control = debugfs_create_file("stats", 0, dir,
					      NULL, &stats_fops);
	if (!stats_control) {
		printk("Couldn't create relay control file 'stats'.\n");
		goto fail;
	}

	subbuf_size_control = debugfs_create_file("subbuf_size", 0, dir,
						  NULL, &subbuf_size_fops);
	if (!subbuf_size_control) {
		printk("Couldn't create relay control file 'subbuf_size'.\n");
		goto fail;
	}

	n_subbufs_control = debugfs_create_file("n_subbufs", 0, dir,
						NULL, &n_subbufs_fops);
	if (!n_subbufs_control) {
		printk("Couldn't create relay control file 'n_subbufs'.\n");
		goto fail;
	}

	create_control = debugfs_create_file("create", 0, dir,
					     NULL, &create_fops);
	if (!create_control) {
		printk("Couldn't create relay control file 'create'.\n");
		goto fail;
	}

	dropped_control = debugfs_create_file("dropped", 0, dir,
					      NULL, &dropped_fops);
	if (!dropped_control) {
		printk("Couldn't create relay control file 'dropped'.\n");
		goto fail;
	}

	return 0;
fail:
	remove_controls();
	return -1;
}

/*
 * control file fileop definitions
 */

/*
 * control files for relay channel management
 */

static ssize_t enabled_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d\n", logging);
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t enabled_write(struct file *filp, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	int enabled;

	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	enabled = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (enabled && chan)
		logging = 1;
	else if (!enabled) {
		logging = 0;
		if (chan) {
			relay_flush(chan);
		}
	}

	return count;
}

/*
 * 'enabled' file operations - boolean r/w
 *
 *  toggles logging to the relay channel
 */
struct file_operations enabled_fops = {
	.owner =	THIS_MODULE,
	.read =		enabled_read,
	.write =	enabled_write,
};

/************************************************************
 * PRINT STATISTICS OPERATIONS
 ***********************************************************/

static ssize_t stats_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d\n", logging);
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t stats_write(struct file *filp, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	int enabled;

	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	enabled = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (enabled && chan) {
		print_statistics();
	}

	return count;
}

/*
 * 'stats' file operations - boolean r/w
 *
 *  prints the probe statistics to the relay channel
 */
struct file_operations stats_fops = {
	.owner =	THIS_MODULE,
	.read =		stats_read,
	.write =	stats_write,
};


static ssize_t create_read(struct file *filp, char __user *buffer,
			   size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%d\n", !!chan);

	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t create_write(struct file *filp, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	int create;

	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	create = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (create) {
		destroy_channel();
		chan = create_channel(subbuf_size, n_subbufs);
		if(!chan)
			return count;
	} else
		destroy_channel();

	return count;
}

/*
 * 'create' file operations - boolean r/w
 *
 *  creates/destroys the relay channel
 */
struct file_operations create_fops = {
	.owner =	THIS_MODULE,
	.read =		create_read,
	.write =	create_write,
};

static ssize_t subbuf_size_read(struct file *filp, char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%zu\n", subbuf_size);

	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t subbuf_size_write(struct file *filp, const char __user *buffer,
				 size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t size;

	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	size = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	subbuf_size = size;

	return count;
}

/*
 * 'subbuf_size' file operations - r/w
 *
 *  gets/sets the subbuffer size to use in channel creation
 */
struct file_operations subbuf_size_fops = {
	.owner =	THIS_MODULE,
	.read =		subbuf_size_read,
	.write =	subbuf_size_write,
};

static ssize_t n_subbufs_read(struct file *filp, char __user *buffer,
			      size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%zu\n", n_subbufs);

	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t n_subbufs_write(struct file *filp, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t n;

	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	n = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	n_subbufs = n;

	return count;
}

/*
 * 'n_subbufs' file operations - r/w
 *
 *  gets/sets the number of subbuffers to use in channel creation
 */
struct file_operations n_subbufs_fops = {
	.owner =	THIS_MODULE,
	.read =		n_subbufs_read,
	.write =	n_subbufs_write,
};

static ssize_t dropped_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%zu\n", dropped);

	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

/*
 * 'dropped' file operations - r
 *
 *  gets the number of dropped events seen
 */
struct file_operations dropped_fops = {
	.owner =	THIS_MODULE,
	.read =		dropped_read,
};


/*
 * control files for relay produced/consumed sub-buffer counts
 */

static int produced_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;

	return 0;
}

static ssize_t produced_read(struct file *filp, char __user *buffer,
			     size_t count, loff_t *ppos)
{
	struct rchan_buf *buf = filp->private_data;

	return simple_read_from_buffer(buffer, count, ppos,
				       &buf->subbufs_produced,
				       sizeof(buf->subbufs_produced));
}

/*
 * 'produced' file operations - r, binary
 *
 *  There is a .produced file associated with each per-cpu relay file.
 *  Reading a .produced file returns the number of sub-buffers so far
 *  produced for the associated relay buffer.
 */
struct file_operations produced_fops = {
	.owner =	THIS_MODULE,
	.open =		produced_open,
	.read =		produced_read
};

static int consumed_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;

	return 0;
}

static ssize_t consumed_read(struct file *filp, char __user *buffer,
			     size_t count, loff_t *ppos)
{
	struct rchan_buf *buf = filp->private_data;

	return simple_read_from_buffer(buffer, count, ppos,
				       &buf->subbufs_consumed,
				       sizeof(buf->subbufs_consumed));
}

static ssize_t consumed_write(struct file *filp, const char __user *buffer,
			      size_t count, loff_t *ppos)
{
	struct rchan_buf *buf = filp->private_data;
	size_t consumed;

	if (copy_from_user(&consumed, buffer, sizeof(consumed)))
		return -EFAULT;

	relay_subbufs_consumed(buf->chan, buf->cpu, consumed);

	return count;
}

/*
 * 'consumed' file operations - r/w, binary
 *
 *  There is a .consumed file associated with each per-cpu relay file.
 *  Writing to a .consumed file adds the value written to the
 *  subbuffers-consumed count of the associated relay buffer.
 *  Reading a .consumed file returns the number of sub-buffers so far
 *  consumed for the associated relay buffer.
 */
struct file_operations consumed_fops = {
	.owner =	THIS_MODULE,
	.open =		consumed_open,
	.read =		consumed_read,
	.write =	consumed_write,
};
