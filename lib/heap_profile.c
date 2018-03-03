/* 2012-02-22: File added and changed by Sony Corporation */
/*
 * lib/heap_profile.c
 *
 * heap profiler: /proc/profile_heap
 *
 * Copyright 2005-2007, 2010 Sony Corporation
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 *
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>

static int records = 0;
module_param(records, int, S_IRUSR);

#ifdef CONFIG_SNSC_DEBUG_PROFILE_HEAP_RINGBUFF
static unsigned is_heap_index_wrapped = 0;
#endif

#define  RECORDS records

static DEFINE_SEMAPHORE(heap_record_index_sem);
typedef struct {
	unsigned long long time; /* sched_clock */
	unsigned long type;      /* measurement point
				    0:try_to_freepages,
				    1: __vm_enough_memory
				 */
	unsigned long nr_used_pages;
	unsigned int nr_free_pages;
	unsigned long nr_cache_pages;
	unsigned long req_pages;
	int pid; 		/* current pid */
} profile_heap_record_t;

static profile_heap_record_t *profile_heap_records;
static profile_heap_record_t profile_heap_used_max_record = {0,};
static profile_heap_record_t profile_heap_needed_max_record = {0,};
static unsigned long used_max = 0;
static unsigned long needed_max = 0;
static unsigned long heap_record_index = 0;
extern unsigned long totalram_pages;

#include <linux/pagemap.h>
#include <linux/vmstat.h> 	/* for global_page_state
				   nr_cache_pages = cached + bufferram
						    + total_swapcache_pages
				*/

void profile_heap_record(unsigned long type, unsigned long req_pages)
{
	unsigned long used = totalram_pages;

	if (!profile_heap_records)
		return;

	if( down_trylock(&heap_record_index_sem) ) {
		printk(KERN_WARNING "Heap profile down_trylock() failed.\n");
		return;
	}

#ifdef CONFIG_SNSC_DEBUG_PROFILE_HEAP_RINGBUFF
	if(heap_record_index >= RECORDS) {
		heap_record_index -= RECORDS;
		is_heap_index_wrapped = 1;
	}
#else
	if(heap_record_index >= RECORDS) {
		goto out;
	}
#endif

	profile_heap_records[heap_record_index].time = sched_clock();
	profile_heap_records[heap_record_index].type = type;
	profile_heap_records[heap_record_index].nr_free_pages = global_page_state(NR_FREE_PAGES);
	used -= profile_heap_records[heap_record_index].nr_free_pages;
	profile_heap_records[heap_record_index].nr_cache_pages = global_page_state(NR_FILE_PAGES);
	used -= profile_heap_records[heap_record_index].nr_cache_pages;
	profile_heap_records[heap_record_index].pid = current->pid;
	profile_heap_records[heap_record_index].req_pages = req_pages;
	profile_heap_records[heap_record_index].nr_used_pages = used;

	if ( used_max <= used ) {
		used_max = used;
		profile_heap_used_max_record = profile_heap_records[heap_record_index];
	}
	used += req_pages;
	if ( needed_max <= used ) {
		needed_max = used;
		profile_heap_needed_max_record = profile_heap_records[heap_record_index];
	}
	heap_record_index++;
#ifndef CONFIG_SNSC_DEBUG_PROFILE_HEAP_RINGBUFF
out:
#endif
	up(&heap_record_index_sem);
	return ;
}
EXPORT_SYMBOL(profile_heap_record);

static int heap_record_to_string(profile_heap_record_t *record, char *buf, int len)
{
	unsigned long nanosec_rem;
	unsigned long long t;

	t = record->time;
	nanosec_rem = do_div(t, 1000000000);


	return snprintf(buf,len,
			/* time        used   cache  free  used+req  req    pid     type */
			"%8lu.%06lu,%8lu," "%8lu"  ",%8u" ",%8lu"   ",%8lu" ",%6d"  ",%2lu\n",
			(unsigned long)t,
			nanosec_rem/1000,
			record->nr_used_pages  << (PAGE_SHIFT - 10),
			record->nr_cache_pages << (PAGE_SHIFT - 10),
			record->nr_free_pages << (PAGE_SHIFT - 10),
			(record->req_pages + record->nr_used_pages) << (PAGE_SHIFT - 10),
			record->req_pages << (PAGE_SHIFT - 10),
			record->pid,
			record->type) ;
}

static unsigned int is_first_line = 1;
#define PROFILE_HEAP_HEADINGS "\ntime,used,cache,free,needed,req,pid,type\n"
#define HEADINGS_LEN sizeof(PROFILE_HEAP_HEADINGS)
#define MAX_NEEDED_STR "Max needed record\n"
#define MAX_NEEDED_STRLEN sizeof(MAX_NEEDED_STR)
#define MAX_USED_STR "\nMax used record\n"
#define MAX_USED_STRLEN sizeof(MAX_USED_STR)
static int profile_heap_proc_read(char *page, char **start, off_t off, int count,
				  int *eof, void *data)
{
	unsigned long tlen = 0;
	unsigned long index = heap_record_index;
	profile_heap_record_t *record;

	if (!profile_heap_records)
		return -EINVAL;

	if(down_interruptible(&heap_record_index_sem))
		return -EINTR;

#ifdef CONFIG_SNSC_DEBUG_PROFILE_HEAP_RINGBUFF
	if (!is_heap_index_wrapped) {
		if(off >= index) {
			*eof = 1;
			is_first_line = 1;
			goto out;
		}
		record = &profile_heap_records[off];
	}
	else {
		unsigned adjust = off + index;
		if (adjust >= RECORDS)
			adjust -= RECORDS;
		if(off >= RECORDS) {
			*eof = 1;
			is_first_line = 1;
			goto out;
		}
		record = &profile_heap_records[adjust];
	}
#else
	if(off >= index) {
		*eof = 1;
		is_first_line = 1;
		goto out;
	}
	record = &profile_heap_records[off];

#endif	/* CONFIG_SNSC_DEBUG_PROFILE_HEAP_RINGBUFF */

	if ( is_first_line ) {
		strncpy(page,MAX_NEEDED_STR,MAX_NEEDED_STRLEN);
		tlen += MAX_NEEDED_STRLEN - 1;
		tlen += heap_record_to_string(&profile_heap_needed_max_record,page+tlen, PAGE_SIZE-tlen);
		strncpy(page+tlen,MAX_USED_STR,MAX_USED_STRLEN);
		tlen += MAX_USED_STRLEN - 1;
		tlen += heap_record_to_string(&profile_heap_used_max_record,page+tlen, PAGE_SIZE-tlen);
		strncpy(page+tlen, PROFILE_HEAP_HEADINGS, HEADINGS_LEN );
		tlen += HEADINGS_LEN - 1;
		is_first_line = 0;
	}

	tlen += heap_record_to_string(record, page+tlen, PAGE_SIZE-tlen);
	*start = (char *)1;
 out:
	up(&heap_record_index_sem);
	return tlen>count?0:tlen;
}

static int profile_heap_proc_write(struct file *file, const char *buffer,
				 unsigned long count, void *data)
{
	if(down_interruptible(&heap_record_index_sem))
		return -EINTR;

	heap_record_index = 0;	/* Reset index in order to record again */
				/* in Non-RingBuff Mode. */
	used_max = 0;
	needed_max = 0;
#ifdef CONFIG_SNSC_DEBUG_PROFILE_HEAP_RINGBUFF
	is_heap_index_wrapped = 0;
#endif

	up(&heap_record_index_sem);
	return count;
}

static int __init debug_heap_proc_profile(void)
{
	struct proc_dir_entry *ent;

	if (0 >= records)
		return 0;
	profile_heap_records = (profile_heap_record_t *)vmalloc(sizeof (profile_heap_record_t) * records);
	if (!profile_heap_records) {
		printk(KERN_ERR "ERROR: can not allocate profile_heap (%d records)\n", records);
		return -ENOMEM;
	}

	ent = create_proc_entry("profile_heap", S_IFREG|S_IRUGO|S_IWUSR, NULL);
	if (!ent) {
		printk(KERN_ERR "create profile_heap proc entry failed\n");
		vfree(profile_heap_records);
		return -ENOMEM;
	}
	ent->read_proc = profile_heap_proc_read;
	ent->write_proc = profile_heap_proc_write;
	return 0;
}
late_initcall(debug_heap_proc_profile);

