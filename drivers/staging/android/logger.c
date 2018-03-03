/* 2013-08-20: File added and changed by Sony Corporation */
/*
 * drivers/misc/logger.c
 *
 * A Logging Subsystem
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * Robert Love <rlove@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * 2012-11-28 Change the logger's buffer memory from statically alloced to
 * dynamically alloced by kernel params: alog.addr & alog.size(Under this case,
 * add struct trailer_area to record info for Sony's log tool)
 * or kmalloc by itself.
 * Sony Corporation
 */

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include "logger.h"

#include <asm/ioctls.h>

/*
 * logger size of main, events and radio.
 */
#define LOGGER_LOG_MAIN_SIZE (256*1024)
#define LOGGER_LOG_EVENTS_SIZE (128*1024)
#define LOGGER_LOG_RADIO_SIZE (64*1024)
#define TRAILER_AREA_SIZE	(sizeof(struct trailer_area))

/*
 * int logger_clock_src - switches a logger clock source.
 *
 * This value is controlled by kernel parameter "alog.clock".
 *   alog.clock=lowres : Use the wallclock time whose src is kernel timer.
 *                       (default and android as-is)
 *   alog.clock=hires  : Use the wallclock time whose src is higher resolution
 *                       than kernel timer.
 *   alog.clock=cpu    : Use monotonic cpu timer.
 *
 * If this value isn't set, logger uses the kernel timer as the logger
 * clock source.
 */
static int logger_clock_src;

enum CLOCK_SOURCE {
	LOWRES_TIMER,
	HIRES_TIMER,
	CPU_TIMER
};

/*
 * struct logger_log - represents a specific log, such as 'main' or 'radio'
 *
 * This structure lives from module insertion until module removal, so it does
 * not need additional reference counting. The structure is protected by the
 * mutex 'mutex'.
 */
struct logger_log {
	struct trailer_area	*trailer_area; /* the trailer area */
	unsigned int		ov_num;	/* the overrun count */
	unsigned char 		*buffer;/* the ring buffer itself */
	struct miscdevice	misc;	/* misc device representing the log */
	wait_queue_head_t	wq;	/* wait queue for readers */
	struct list_head	readers; /* this log's readers */
	struct mutex		mutex;	/* mutex protecting buffer */
	size_t			w_off;	/* current write head offset */
	size_t			head;	/* new readers start here */
	size_t			size;	/* size of the log */
};

/*
 * struct logger_reader - a logging device open for reading
 *
 * This object lives from open to release, so we don't need additional
 * reference counting. The structure is protected by log->mutex.
 */
struct logger_reader {
	struct logger_log	*log;	/* associated log */
	struct list_head	list;	/* entry in logger_log's list */
	size_t			r_off;	/* current read head offset */
};

/* logger_offset - returns index 'n' into the log via (optimized) modulus */
#define logger_offset(n)	((n) & (log->size - 1))

static unsigned char	*g_logger_addr;
static unsigned int	g_logger_size;
static unsigned char	*g_logger_cur_addr;

/*
 * file_get_log - Given a file structure, return the associated log
 *
 * This isn't aesthetic. We have several goals:
 *
 * 	1) Need to quickly obtain the associated log during an I/O operation
 * 	2) Readers need to maintain state (logger_reader)
 * 	3) Writers need to be very fast (open() should be a near no-op)
 *
 * In the reader case, we can trivially go file->logger_reader->logger_log.
 * For a writer, we don't want to maintain a logger_reader, so we just go
 * file->logger_log. Thus what file->private_data points at depends on whether
 * or not the file was opened for reading. This function hides that dirtiness.
 */
static inline struct logger_log *file_get_log(struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader = file->private_data;
		return reader->log;
	} else
		return file->private_data;
}

/*
 * get_entry_len - Grabs the length of the payload of the next entry starting
 * from 'off'.
 *
 * Caller needs to hold log->mutex.
 */
static __u32 get_entry_len(struct logger_log *log, size_t off)
{
	__u16 val;

	switch (log->size - off) {
	case 1:
		memcpy(&val, log->buffer + off, 1);
		memcpy(((char *) &val) + 1, log->buffer, 1);
		break;
	default:
		memcpy(&val, log->buffer + off, 2);
	}

	return sizeof(struct logger_entry) + val;
}

/*
 * do_read_log_to_user - reads exactly 'count' bytes from 'log' into the
 * user-space buffer 'buf'. Returns 'count' on success.
 *
 * Caller must hold log->mutex.
 */
static ssize_t do_read_log_to_user(struct logger_log *log,
				   struct logger_reader *reader,
				   char __user *buf,
				   size_t count)
{
	size_t len;

	/*
	 * We read from the log in two disjoint operations. First, we read from
	 * the current read head offset up to 'count' bytes or to the end of
	 * the log, whichever comes first.
	 */
	len = min(count, log->size - reader->r_off);
	if (copy_to_user(buf, log->buffer + reader->r_off, len))
		return -EFAULT;

	/*
	 * Second, we read any remaining bytes, starting back at the head of
	 * the log.
	 */
	if (count != len)
		if (copy_to_user(buf + len, log->buffer, count - len))
			return -EFAULT;

	reader->r_off = logger_offset(reader->r_off + count);

	return count;
}

/*
 * logger_read - our log's read() method
 *
 * Behavior:
 *
 * 	- O_NONBLOCK works
 * 	- If there are no log entries to read, blocks until log is written to
 * 	- Atomically reads exactly one log entry
 *
 * Optimal read size is LOGGER_ENTRY_MAX_LEN. Will set errno to EINVAL if read
 * buffer is insufficient to hold next entry.
 */
static ssize_t logger_read(struct file *file, char __user *buf,
			   size_t count, loff_t *pos)
{
	struct logger_reader *reader = file->private_data;
	struct logger_log *log = reader->log;
	ssize_t ret;
	DEFINE_WAIT(wait);

start:
	while (1) {
		prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

		mutex_lock(&log->mutex);
		ret = (log->w_off == reader->r_off);
		mutex_unlock(&log->mutex);
		if (!ret)
			break;

		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -EINTR;
			break;
		}

		schedule();
	}

	finish_wait(&log->wq, &wait);
	if (ret)
		return ret;

	mutex_lock(&log->mutex);

	/* is there still something to read or did we race? */
	if (unlikely(log->w_off == reader->r_off)) {
		mutex_unlock(&log->mutex);
		goto start;
	}

	/* get the size of the next entry */
	ret = get_entry_len(log, reader->r_off);
	if (count < ret) {
		ret = -EINVAL;
		goto out;
	}

	/* get exactly one entry from the log */
	ret = do_read_log_to_user(log, reader, buf, ret);

out:
	mutex_unlock(&log->mutex);

	return ret;
}

/*
 * get_next_entry - return the offset of the first valid entry at least 'len'
 * bytes after 'off'.
 *
 * Caller must hold log->mutex.
 */
static size_t get_next_entry(struct logger_log *log, size_t off, size_t len)
{
	size_t count = 0;
	log->ov_num = 0;

	do {
		size_t nr = get_entry_len(log, off);
		off = logger_offset(off + nr);
		count += nr;
		log->ov_num++;
	} while (count < len);

	return off;
}

/*
 * clock_interval - is a < c < b in mod-space? Put another way, does the line
 * from a to b cross c?
 */
static inline int clock_interval(size_t a, size_t b, size_t c)
{
	if (b < a) {
		if (a < c || b >= c)
			return 1;
	} else {
		if (a < c && b >= c)
			return 1;
	}

	return 0;
}

/*
 * fix_up_readers - walk the list of all readers and "fix up" any who were
 * lapped by the writer; also do the same for the default "start head".
 * We do this by "pulling forward" the readers and start head to the first
 * entry after the new write head.
 *
 * The caller needs to hold log->mutex.
 */
static void fix_up_readers(struct logger_log *log, size_t len)
{
	size_t old = log->w_off;
	size_t new = logger_offset(old + len);
	struct logger_reader *reader;

	if (clock_interval(old, new, log->head)) {
		log->head = get_next_entry(log, log->head, len);
		if (log->trailer_area) {
			log->trailer_area->num_of_entry -= log->ov_num;
			/* Update number of entries in the buffer */
		}
	}

	list_for_each_entry(reader, &log->readers, list)
		if (clock_interval(old, new, reader->r_off))
			reader->r_off = get_next_entry(log, reader->r_off, len);

	if (log->trailer_area) {
		log->trailer_area->start_offset = log->head;
			/* Update the first available entry for access */
		log->trailer_area->num_of_entry += 1;
			/* Update number of entries in the buffer */
	}
}

/*
 * do_write_log - writes 'len' bytes from 'buf' to 'log'
 *
 * The caller needs to hold log->mutex.
 */
static void do_write_log(struct logger_log *log, const void *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	memcpy(log->buffer + log->w_off, buf, len);

	if (count != len)
		memcpy(log->buffer, buf + len, count - len);

	log->w_off = logger_offset(log->w_off + count);

}

/*
 * do_write_log_user - writes 'len' bytes from the user-space buffer 'buf' to
 * the log 'log'
 *
 * The caller needs to hold log->mutex.
 *
 * Returns 'count' on success, negative error code on failure.
 */
static ssize_t do_write_log_from_user(struct logger_log *log,
				      const void __user *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	if (len && copy_from_user(log->buffer + log->w_off, buf, len))
		return -EFAULT;

	if (count != len)
		if (copy_from_user(log->buffer, buf + len, count - len))
			return -EFAULT;

	log->w_off = logger_offset(log->w_off + count);

	return count;
}

/*
 * logger_aio_write - our write method, implementing support for write(),
 * writev(), and aio_write(). Writes are our fast path, and we try to optimize
 * them above all else.
 */
ssize_t logger_aio_write(struct kiocb *iocb, const struct iovec *iov,
			 unsigned long nr_segs, loff_t ppos)
{
	struct logger_log *log = file_get_log(iocb->ki_filp);
	size_t orig = log->w_off;
	struct logger_entry header;
	struct timespec now;
	ssize_t ret = 0;
	unsigned long long t;

	switch (logger_clock_src) {
	case LOWRES_TIMER:
		now = current_kernel_time();
		break;
	case HIRES_TIMER:
		getnstimeofday(&now);
		break;
	case CPU_TIMER:
		t = cpu_clock(raw_smp_processor_id());

		/*
		 * The cpu_clock returns nano sec, so is divided by 10^9
		 * to convert to timespce.
		 */
		now.tv_sec = t / NSEC_PER_SEC;
		now.tv_nsec = t % NSEC_PER_SEC;
		break;
	default:
		now = current_kernel_time();
	}

	header.pid = current->tgid;
	header.tid = current->pid;
	header.sec = now.tv_sec;
	header.nsec = now.tv_nsec;
	header.len = min_t(size_t, iocb->ki_left, LOGGER_ENTRY_MAX_PAYLOAD);

	/* null writes succeed, return zero */
	if (unlikely(!header.len))
		return 0;

	mutex_lock(&log->mutex);

	/*
	 * Fix up any readers, pulling them forward to the first readable
	 * entry after (what will be) the new write offset. We do this now
	 * because if we partially fail, we can end up with clobbered log
	 * entries that encroach on readable buffer.
	 */
	fix_up_readers(log, sizeof(struct logger_entry) + header.len);

	do_write_log(log, &header, sizeof(struct logger_entry));

	while (nr_segs-- > 0) {
		size_t len;
		ssize_t nr;

		/* figure out how much of this vector we can keep */
		len = min_t(size_t, iov->iov_len, header.len - ret);

		/* write out this segment's payload */
		nr = do_write_log_from_user(log, iov->iov_base, len);
		if (unlikely(nr < 0)) {
			log->w_off = orig;
			mutex_unlock(&log->mutex);
			return nr;
		}

		iov++;
		ret += nr;
	}

	mutex_unlock(&log->mutex);

	/* wake up any blocked readers */
	wake_up_interruptible(&log->wq);

	return ret;
}

static struct logger_log *get_log_from_minor(int);

/*
 * logger_open - the log's open() file operation
 *
 * Note how near a no-op this is in the write-only case. Keep it that way!
 */
static int logger_open(struct inode *inode, struct file *file)
{
	struct logger_log *log;
	int ret;

	ret = nonseekable_open(inode, file);
	if (ret)
		return ret;

	log = get_log_from_minor(MINOR(inode->i_rdev));
	if (!log)
		return -ENODEV;

	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader;

		reader = kmalloc(sizeof(struct logger_reader), GFP_KERNEL);
		if (!reader)
			return -ENOMEM;

		reader->log = log;
		INIT_LIST_HEAD(&reader->list);

		mutex_lock(&log->mutex);
		reader->r_off = log->head;
		list_add_tail(&reader->list, &log->readers);
		mutex_unlock(&log->mutex);

		file->private_data = reader;
	} else
		file->private_data = log;

	return 0;
}

/*
 * logger_release - the log's release file operation
 *
 * Note this is a total no-op in the write-only case. Keep it that way!
 */
static int logger_release(struct inode *ignored, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader = file->private_data;
		list_del(&reader->list);
		kfree(reader);
	}

	return 0;
}

/*
 * logger_poll - the log's poll file operation, for poll/select/epoll
 *
 * Note we always return POLLOUT, because you can always write() to the log.
 * Note also that, strictly speaking, a return value of POLLIN does not
 * guarantee that the log is readable without blocking, as there is a small
 * chance that the writer can lap the reader in the interim between poll()
 * returning and the read() request.
 */
static unsigned int logger_poll(struct file *file, poll_table *wait)
{
	struct logger_reader *reader;
	struct logger_log *log;
	unsigned int ret = POLLOUT | POLLWRNORM;

	if (!(file->f_mode & FMODE_READ))
		return ret;

	reader = file->private_data;
	log = reader->log;

	poll_wait(file, &log->wq, wait);

	mutex_lock(&log->mutex);
	if (log->w_off != reader->r_off)
		ret |= POLLIN | POLLRDNORM;
	mutex_unlock(&log->mutex);

	return ret;
}

static long logger_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct logger_log *log = file_get_log(file);
	struct logger_reader *reader;
	long ret = -ENOTTY;

	mutex_lock(&log->mutex);

	switch (cmd) {
	case LOGGER_GET_LOG_BUF_SIZE:
		ret = log->size;
		break;
	case LOGGER_GET_LOG_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (log->w_off >= reader->r_off)
			ret = log->w_off - reader->r_off;
		else
			ret = (log->size - reader->r_off) + log->w_off;
		break;
	case LOGGER_GET_NEXT_ENTRY_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (log->w_off != reader->r_off)
			ret = get_entry_len(log, reader->r_off);
		else
			ret = 0;
		break;
	case LOGGER_FLUSH_LOG:
		if (!(file->f_mode & FMODE_WRITE)) {
			ret = -EBADF;
			break;
		}
		list_for_each_entry(reader, &log->readers, list)
			reader->r_off = log->w_off;
		log->head = log->w_off;

		log->ov_num = 0;
		if (log->trailer_area) {
			log->trailer_area->start_offset = log->head;
			log->trailer_area->num_of_entry = 0;
		}

		ret = 0;
		break;
	}

	mutex_unlock(&log->mutex);

	return ret;
}

static const struct file_operations logger_fops = {
	.owner = THIS_MODULE,
	.read = logger_read,
	.aio_write = logger_aio_write,
	.poll = logger_poll,
	.unlocked_ioctl = logger_ioctl,
	.compat_ioctl = logger_ioctl,
	.open = logger_open,
	.release = logger_release,
};

static int __init parse_boot_param_mem_phyaddr(char* val)
{
	if (!val) {
		printk(KERN_ERR "Error parameter for alog.addr\n");
		return 0;
	}
	g_logger_addr = __va(simple_strtol(val, NULL, 0));
	printk(KERN_INFO "alog.addr is : %p\n", (void *)__pa(g_logger_addr));
	return 1;
}

static int __init parse_boot_param_mem_size(char* val)
{
	if (!val) {
		printk(KERN_ERR "Error parameter for alog.size\n");
		return 0;
	}
	g_logger_size = simple_strtol(val, NULL, 0);
	printk(KERN_INFO "alog.size is: %dK\n", g_logger_size/1024);
	return 1;
}

__setup("alog.addr=", parse_boot_param_mem_phyaddr);
__setup("alog.size=", parse_boot_param_mem_size);

/*
 * Defines a log structure with name 'NAME' and a size of 'SIZE' bytes, which
 * must be a power of two, greater than LOGGER_ENTRY_MAX_LEN, and less than
 * LONG_MAX minus LOGGER_ENTRY_MAX_LEN.
 */
#define DEFINE_LOGGER_DEVICE(VAR, NAME, SIZE) \
static struct logger_log VAR = { \
	.trailer_area = NULL, \
	.misc = { \
		.minor = MISC_DYNAMIC_MINOR, \
		.name = NAME, \
		.fops = &logger_fops, \
		.parent = NULL, \
	}, \
	.wq = __WAIT_QUEUE_HEAD_INITIALIZER(VAR .wq), \
	.readers = LIST_HEAD_INIT(VAR .readers), \
	.mutex = __MUTEX_INITIALIZER(VAR .mutex), \
	.w_off = 0, \
	.head = 0, \
	.size = SIZE, \
};

/*
 * When alloc logger mem by this driver, use this to init!
 */
#define SET_LOGGER_BUFF(VAR, SIZE) \
	VAR .buffer = g_logger_cur_addr; \
	g_logger_cur_addr += SIZE;

/*
 * When use mem from kernel boot param init the logger mem with this
 */
#define INIT_LOGGER_DEVICE(VAR, NAME, SIZE) \
	VAR .buffer = g_logger_cur_addr;\
	g_logger_cur_addr += SIZE;\
	VAR .trailer_area = (struct trailer_area *)g_logger_cur_addr;\
	memset(&(VAR .trailer_area->magic), '\0', sizeof(uint64_t));\
	if (!strncmp(NAME, "log_main", 8)) {\
		strncpy((char *)&(VAR .trailer_area->magic),\
		"MAIN", sizeof(uint64_t));\
		VAR .trailer_area->next_info_offset = 0;\
	} else if (!strncmp(NAME, "log_events", 10)) {\
		strncpy((char *)&(VAR .trailer_area->magic),\
		"EVENTS", sizeof(uint64_t));\
		VAR .trailer_area->next_info_offset = \
		LOGGER_LOG_EVENTS_SIZE + TRAILER_AREA_SIZE; \
	} \
	VAR .trailer_area->log_size = SIZE;\
	VAR .trailer_area->start_offset = 0;\
	VAR .trailer_area->num_of_entry = 0;\
	g_logger_cur_addr += TRAILER_AREA_SIZE;\
	printk(KERN_INFO"%s start: %p-trailer: %p-nxt addr: %p-magic: %s\n",\
	NAME, VAR .buffer, VAR .trailer_area, g_logger_cur_addr,\
					(char *)&(VAR .trailer_area->magic));

DEFINE_LOGGER_DEVICE(log_main, LOGGER_LOG_MAIN, LOGGER_LOG_MAIN_SIZE)
DEFINE_LOGGER_DEVICE(log_events, LOGGER_LOG_EVENTS, LOGGER_LOG_EVENTS_SIZE)
DEFINE_LOGGER_DEVICE(log_radio, LOGGER_LOG_RADIO, LOGGER_LOG_RADIO_SIZE)

static struct logger_log *get_log_from_minor(int minor)
{
	if (log_main.misc.minor == minor)
		return &log_main;
	if (log_events.misc.minor == minor)
		return &log_events;
	if (log_radio.misc.minor == minor)
		return &log_radio;
	return NULL;
}

static int __init init_log(struct logger_log *log)
{
	int ret;

	ret = misc_register(&log->misc);
	if (unlikely(ret)) {
		printk(KERN_ERR "logger: failed to register misc "
		       "device for log '%s'!\n", log->misc.name);
		return ret;
	}

	printk(KERN_INFO "logger: created %luK log '%s'\n",
	       (unsigned long) log->size >> 10, log->misc.name);

	return 0;
}

static int __init logger_init(void)
{
	int ret;
	char *radiobuf = NULL;
	if (g_logger_addr) {
		g_logger_cur_addr = g_logger_addr;
		printk(KERN_INFO "Logger@Get alog.addr: %p & alog.size: %dK\n",
			(void *)__pa(g_logger_addr), g_logger_size/1024);
		INIT_LOGGER_DEVICE(log_main, LOGGER_LOG_MAIN,
			LOGGER_LOG_MAIN_SIZE);
		INIT_LOGGER_DEVICE(log_events, LOGGER_LOG_EVENTS,
			LOGGER_LOG_EVENTS_SIZE);
		/*
		 * log buffer for radio related will be out of trailer area
		 */
		radiobuf = kmalloc(LOGGER_LOG_RADIO_SIZE, GFP_KERNEL);
		if (!radiobuf) {
			printk(KERN_INFO "Logger@kmalloc for logger radio failed\n");
			return -1;
		}
		log_radio.buffer = radiobuf;
	} else {
		printk(KERN_ERR "Logger@Error can't get alog.addr & alog.size! Alloc memory by logger itself!\n");
		g_logger_addr = kmalloc(LOGGER_LOG_MAIN_SIZE
			+ LOGGER_LOG_EVENTS_SIZE
			+ LOGGER_LOG_RADIO_SIZE, GFP_KERNEL);
		if (!g_logger_addr) {
			printk(KERN_ERR "Can't alloc memory for logger driver!\n");
			return -1;
		}
		g_logger_cur_addr = g_logger_addr;
		SET_LOGGER_BUFF(log_main, LOGGER_LOG_MAIN_SIZE)
		SET_LOGGER_BUFF(log_events, LOGGER_LOG_EVENTS_SIZE)
		SET_LOGGER_BUFF(log_radio, LOGGER_LOG_RADIO_SIZE)
	}

	ret = init_log(&log_main);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_events);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_radio);
	if (unlikely(ret))
		goto out;

out:
	return ret;
}
device_initcall(logger_init);

static int __init set_logger_clock_src(char *str)
{
	if (strcmp(str, "lowres") == 0)
		logger_clock_src = LOWRES_TIMER;
	else if (strcmp(str, "hires") == 0)
		logger_clock_src = HIRES_TIMER;
	else if (strcmp(str, "cpu") == 0)
		logger_clock_src = CPU_TIMER;

	printk(KERN_INFO "logger: logger_clock_src is '%s'\n", str);
	return 1;
}

__setup("alog.clock=", set_logger_clock_src);
