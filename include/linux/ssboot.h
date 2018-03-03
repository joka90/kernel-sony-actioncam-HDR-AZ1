/* 2012-07-18: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot - core interface
 *
 *  Copyright 2008,2009,2010 Sony Corporation
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
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _LINUX_SSBOOT_H
#define _LINUX_SSBOOT_H

#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/ssboot_stat.h>

/* constant number */
#define SSBOOT_SZ_1KB			(1024)
#define SSBOOT_SZ_1MB			(1024 * 1024)
#define SSBOOT_PG_1MB			(SSBOOT_SZ_1MB / PAGE_SIZE)

/* invalid pfn value */
#define SSBOOT_PFN_NONE			-1UL

/* section information */
#define SSBOOT_SECTION_NORMAL		(0 << 31)
#define SSBOOT_SECTION_CRITICAL		(1 << 31)

typedef struct ssboot_section {
	unsigned long writer_pfn;
	unsigned long start_pfn;
	unsigned long num_pages;
	unsigned long attr;
} ssboot_section_t;

/* image information */
#define SSBOOT_TARGETID_UNKNOWN		0xffff

/* ssboot cmdline size */
#define SSBOOT_CMDLINE_SIZE		256

typedef struct ssboot_image {
	u_int16_t        target_id;
	void             *entry_addr;
	ssboot_section_t *section;
	unsigned long    num_section;
	void             *cmdline;
} ssboot_image_t;

/* image writer descriptor */
typedef struct ssboot_writer {
	struct list_head list;
	int (*prepare)(void *priv);
	int (*write)(ssboot_image_t *image, void *priv);
	int (*cleanup)(void *priv);
	void *priv;
} ssboot_writer_t;

/* image optimizer event */
enum ssboot_optimizer_event {
	 SSBOOT_OPTEVT_FILE,
	 SSBOOT_OPTEVT_ANON,
	 SSBOOT_OPTEVT_MAX
};
extern const char * const ssboot_optimizer_event_name[SSBOOT_OPTEVT_MAX];

/* image optimizer descriptor */
typedef struct ssboot_optimizer {
	int (*optimize)(void *priv);
	int (*start_profiling)(void *priv);
	int (*stop_profiling)(void *priv);
	int (*is_profiling)(void *priv);
	int (*record)(void *priv, enum ssboot_optimizer_event ev, va_list args);
	void *priv;
} ssboot_optimizer_t;

/* proc operations */
#define SSBOOT_PROC_RDWR		0644
#define SSBOOT_PROC_RDONLY		0444
#define SSBOOT_PROC_WRONLY		0200

typedef struct ssboot_proc_ops {
	int    opened;
	mode_t mode;
	size_t write_max;
	size_t write_len;
	char   *write_buf;
	void   *data;
	int    (*open)(struct inode *inode, struct file *file);
	int    (*release)(struct inode *inode, struct file *file);
	struct seq_operations seq_ops;
} ssboot_proc_ops_t;

#define ssboot_single_proc(_name, _mode, _write_max)	\
static ssboot_proc_ops_t _name##_ops = {		\
	.opened		= 0,				\
	.mode		= _mode,			\
	.write_max	= _write_max,			\
	.open		= _name##_open,			\
	.release	= _name##_release,		\
	.seq_ops	= {				\
		.start	= NULL,				\
		.next	= NULL,				\
		.show	= _name##_show,			\
	},						\
}

#define ssboot_seq_proc(_name, _mode, _write_max)	\
static ssboot_proc_ops_t _name##_ops = {		\
	.opened		= 0,				\
	.mode		= _mode,			\
	.write_max	= _write_max,			\
	.open		= _name##_open,			\
	.release	= _name##_release,		\
	.seq_ops	= {				\
		.start	= _name##_start,		\
		.next	= _name##_next,			\
		.show	= _name##_show,			\
	},						\
}

#define ssboot_proc_get_ops_file(f)	\
	((ssboot_proc_ops_t *)((struct seq_file *)(f)->private_data)->private)

#define ssboot_proc_get_ops_seq(s)	\
	((ssboot_proc_ops_t *)(s)->private)

/* list of proc interface */
typedef struct ssboot_proc_list {
	const char        *name;
	ssboot_proc_ops_t *ops;
} ssboot_proc_list_t;

extern struct proc_dir_entry *ssboot_proc_root;
extern struct proc_dir_entry *ssboot_proc_optimizer;

/* core interface */
#ifdef CONFIG_SNSC_SSBOOT
void __init ssboot_init(void);
int ssboot_prepare(void);
int ssboot_snapshot(void *entry_addr);
int ssboot_write(void);
int ssboot_finish(void);
int ssboot_is_writing(void);
int ssboot_is_resumed(void);
int ssboot_is_profiling(void);
int ssboot_is_rewriting(void);
void ssboot_set_target_id(u_int16_t target_id);
int ssboot_memmap_register(u_int64_t phys_addr, void *virt_addr, size_t len);
int ssboot_pfn_valid(unsigned long pfn);
int ssboot_pfn_valid_range(unsigned long pfn, unsigned long num);
void* ssboot_pfn_to_virt(unsigned long pfn);
unsigned long ssboot_virt_to_pfn(void *virt_addr);
int ssboot_invalidate_page_cache(const char *filename);
int ssboot_writer_register(ssboot_writer_t *writer);
int ssboot_writer_unregister(ssboot_writer_t *writer);
int ssboot_optimizer_register(ssboot_optimizer_t *optimizer);
int ssboot_optimizer_unregister(ssboot_optimizer_t *optimizer);
int ssboot_optimizer_is_profiling(void);
int ssboot_optimizer_record(enum ssboot_optimizer_event ev, ...);
int ssboot_region_register_normal(u_int64_t phys_addr, size_t len);
int ssboot_region_register_critical(u_int64_t phys_addr, size_t len);
int ssboot_region_register_discard(u_int64_t phys_addr, size_t len);
int ssboot_region_register_work(u_int64_t phys_addr, size_t len);
int ssboot_region_unregister(u_int64_t phys_addr, size_t len);
int ssboot_proc_create_entry(const char *name, ssboot_proc_ops_t *ops,
			     struct proc_dir_entry *parent);
void ssboot_proc_remove_entry(const char *name, struct proc_dir_entry *parent);

extern void ssboot_set_swapfile(const char *filename);
extern const char* ssboot_get_swapfile(void);
extern int ssboot_swapon(void);
extern int ssboot_swap_set_ro(void);
extern int ssboot_optimizer_start_profiling(void);
extern int ssboot_optimizer_stop_profiling(void);
extern int ssboot_optimizer_optimize(void);

extern unsigned long get_ssboot_stat(void);

#else /* CONFIG_SNSC_SSBOOT */
static inline int ssboot_is_profiling(void)
{
	return 0;
}
static inline int ssboot_is_rewriting(void)
{
	return 0;
}
static inline int ssboot_optimizer_is_profiling(void)
{
	return 0;
}
static inline int ssboot_optimizer_record(enum ssboot_optimizer_event ev, ...)
{
	return 0;
}
static inline int ssboot_optimizer_start_profiling(void)
{
	return 0;
}
static inline int ssboot_optimizer_stop_profiling(void)
{
	return 0;
}
#endif /* CONFIG_SNSC_SSBOOT */

/* for debug */
#ifdef CONFIG_SNSC_SSBOOT_DEBUG
#define ssboot_dbg(format, arg...)   printk(KERN_INFO "ssboot: " format, ##arg)
#else
#define ssboot_dbg(format, arg...)
#endif
#define ssboot_info(format, arg...)  printk(KERN_INFO "ssboot: " format, ##arg)
#define ssboot_err(format, arg...)   printk(KERN_ERR  "ssboot: " format, ##arg)

#endif /* _LINUX_SSBOOT_H */
