/* 2010-06-23: File added and changed by Sony Corporation */
/*
 * lib/profile.c
 *
 * profiler: /proc/profile_record and /proc/sched_clock
 *
 * Copyright 2005-2007 Sony Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/snsc_debug_profile.h>

typedef struct {
	unsigned long long time;
	const char *caller;
	const char *callee;
	const void *addr; 	//callee address
} profile_record_t;

#define RECORDS CONFIG_SNSC_DEBUG_PROFILE_RECORDS_NUM
static profile_record_t profile_records[RECORDS];
static char profilie_strings[RECORDS][16];
static unsigned record_index;
#ifdef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
static unsigned is_index_wrapped;
#endif
static DEFINE_SEMAPHORE(record_index_sem);

void profile_soft_init(void)
{
	record_index = 0;
#ifdef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
	is_index_wrapped = 0;
#endif
}


void profile_record(const char *caller,const char *callee)
{
	if(down_trylock(&record_index_sem)){
		printk("profile_record down_trylock() failed.\n");
		return;
	}

#ifdef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
	if(record_index >= RECORDS)
	{
		record_index -= RECORDS;
		is_index_wrapped = 1;
	}
#else
	if(record_index >= RECORDS)
		goto out;
#endif

	profile_records[record_index].time = sched_clock();
	profile_records[record_index].caller = caller;
	profile_records[record_index].callee = callee;
	profile_records[record_index++].addr = NULL;

#ifndef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
out:
#endif
	up(&record_index_sem);
}

EXPORT_SYMBOL(profile_record);

void profile_record_p(const char *caller,const void *addr)
{
	if(down_trylock(&record_index_sem)){
		printk("profile_record_p down_trylock() failed.\n");
		return;
	}

#ifdef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
	if(record_index >= RECORDS)
	{
		record_index -= RECORDS;
		is_index_wrapped = 1;
	}
#else
	if(record_index >= RECORDS)
		goto out;
#endif

	profile_records[record_index].time = sched_clock();
	profile_records[record_index].caller = caller;
	profile_records[record_index].callee = NULL;
	profile_records[record_index++].addr = addr;

#ifndef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
out:
#endif
	up(&record_index_sem);
}

EXPORT_SYMBOL(profile_record_p);

static int sched_clock_proc_write(struct file *file, const char *buffer,
				 unsigned long count, void *data)
{
	char *buf;

	if(down_interruptible(&record_index_sem))
		return -EINTR;

#ifdef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
	if(record_index >= RECORDS)
	{
		record_index -= RECORDS;
		is_index_wrapped = 1;
	}
#else
	if(record_index >= RECORDS)
		goto out;
#endif

	buf = profilie_strings[record_index];

	strncpy(buf, buffer, count>15?15:count);
	buf[count>15?15:count] = 0;


	profile_records[record_index].time = sched_clock();
	profile_records[record_index].caller = "user";
	profile_records[record_index].callee = buf;
	profile_records[record_index++].addr = NULL;

#ifndef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
out:
#endif
	up(&record_index_sem);
	return count;
}

static int sched_clock_proc_read(char *page, char **start, off_t off, int count,
				 int *eof, void *data)
{
	unsigned tlen;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = sched_clock();
	nanosec_rem = do_div(t, 1000000000);
	tlen = snprintf(page, PAGE_SIZE,
		       "[ %5lu.%06lu ] -profile shell\n",
		       (unsigned long)t,
		       nanosec_rem/1000);
	*eof = 1;
	return tlen;
}

static int __init debug_proc_sched_clock(void)
{
	struct proc_dir_entry *ent;
	ent = create_proc_entry("sched_clock", S_IFREG|S_IRUGO|S_IWUSR, NULL);
	if(!ent) {
		printk(KERN_ERR "create sched_clock proc entry failed\n");
		return -ENOMEM;
	}
	ent->read_proc = sched_clock_proc_read;
	ent->write_proc = sched_clock_proc_write;
	return 0;
}

late_initcall(debug_proc_sched_clock);

static int record_to_string(profile_record_t *record, char *buf, int len)
{
	unsigned long nanosec_rem;
	unsigned long long t;
	char namebuf[KSYM_NAME_LEN+1];
	t = record->time;

	if(record->addr){
		char *modname;
		unsigned long offset,size;
		record->callee = kallsyms_lookup((unsigned long)record->addr, &size, &offset, &modname, namebuf);
	}

	nanosec_rem = do_div(t, 1000000000);
	return snprintf(buf,len,
			"[ %5lu.%06lu ] -profile %s %s\n",
			(unsigned long)t,
			nanosec_rem/1000,
			record->caller,
			record->callee);
}

static int profile_record_proc_write(struct file *file, const char *buffer,
				 unsigned long count, void *data)
{
	if(down_interruptible(&record_index_sem))
		return -EINTR;

	profile_soft_init();

	up(&record_index_sem);
	return count;
}

static int profile_record_proc_read(char *page, char **start, off_t off, int count,
				 int *eof, void *data)
{
	unsigned long tlen=0;
	profile_record_t *record;

	if(down_interruptible(&record_index_sem))
		return -EINTR;

#ifdef CONFIG_SNSC_DEBUG_PROFILE_RINGBUFF
	/*
     * case1: discard mode
	 * case2: ring buffer mode -- if (is_index_wrapped == 1)
	 *
	 *   off= 0               ri            RECORDS
	 * case1: | 1.from boot   |   unused    |
	 * case2: | 2.wrapped new | 1.used      |
	 *                        |<-adj (cont.)|
	 *        |-(cont.)  adj->|
	 *
	 */
	if (!is_index_wrapped)
	{
		if(off >= record_index){
			*eof = 1;
			goto out;
		}
		record = &profile_records[off];
	}
	else
	{
		unsigned adjust = off + record_index;
		if (adjust >= RECORDS)
			adjust -= RECORDS;
		if(off >= RECORDS)
		{
			*eof = 1;
			goto out;
		}
		record = &profile_records[adjust];
	}
#else
	if(off >= record_index){
		*eof = 1;
		goto out;
	}
	record = &profile_records[off];
#endif
	tlen = record_to_string(record, page, PAGE_SIZE);
	*start = (char *)1;
 out:
	up(&record_index_sem);
	return tlen>count?0:tlen;
}

static int __init debug_proc_profile(void)
{
	struct proc_dir_entry *ent;

	profile_soft_init();

	ent = create_proc_entry("profile_record", S_IFREG|S_IRUGO|S_IWUSR, NULL);
	if (!ent) {
		printk(KERN_ERR "create profile_record proc entry failed\n");
		return -ENOMEM;
	}
	ent->read_proc = profile_record_proc_read;
	ent->write_proc = profile_record_proc_write;
	return 0;
}

void profile_dump_record(void (*print)(char *))
{
	char buf[128];
	int i;
	profile_record(__FUNCTION__, "dump");
	for(i=0; i<record_index; i++){
		record_to_string(&profile_records[i], buf, 128);
		print(buf);
	}
}

late_initcall(debug_proc_profile);

