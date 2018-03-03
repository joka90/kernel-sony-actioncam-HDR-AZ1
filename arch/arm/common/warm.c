/*
 * arch/arm/common/warm.c
 *
 * image creation driver I/F
 *
 * Copyright 2005,2006,2011 Sony Corporation
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

#include <linux/suspend.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sysfs.h>
#include <linux/pm.h>
#include <linux/vmalloc.h>
#include <linux/lz77.h>
#include <linux/bootmem.h>
#include <linux/dma-mapping.h>
#include <linux/snsc_boot_time.h>

#include <asm/stat.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/setup.h>
#include <asm/mach/warmboot.h>
#include <asm/io.h>
#include <mach/time.h>

#include <mach/noncache.h>
#include <mach/moduleparam.h>
#include "wbi.h"

#if defined (CONFIG_SWAP) && defined (CONFIG_SNSC_SSBOOT)
#include <linux/delay.h>
#include <linux/ssboot.h>
#endif

#define SECTOR_SHIFT 9
#define DEFAULT_SECTOR_SIZE (1 << SECTOR_SHIFT)

extern struct kobject *power_kobj;

#define WBI_MODE(state, mode) ((state) == PM_SUSPEND_DISK && (mode))

#define wb_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

/*
  warm boot device list
 */
static LIST_HEAD(wb_device_list);
static DEFINE_SEMAPHORE(wb_device_sem);

/*
  warm boot memory region list
 */
static LIST_HEAD(wb_mem_list);
static DEFINE_SEMAPHORE(wb_mem_sem);

struct wb_user_mem_reg {
	unsigned long act;
	unsigned long phys;
	unsigned long virt;
	unsigned long size;
};

struct wb_mem_reg {
	struct list_head list;
	unsigned long io_virt;

	struct wb_user_mem_reg user_data;
};

struct wb_header wbheader = {
	.dVersion = CONFIG_WBI_DVERSION,
	.sections = 0,
	.o_data_size = 0,
	.r_data_size = 0
};

struct wb_device *wb_default_device;
struct wbi_compressor *wbi_default_compressor;

enum {
	IMAGE_MODE_NONE = 0,
	IMAGE_MODE_MIN,
	IMAGE_MODE_MAX,
	IMAGE_MODE_ADP,
	IMAGE_MODE_NUM
};


static suspend_state_t pm_state=PM_SUSPEND_MEM;

typedef int (*callback_t)(unsigned long phys, unsigned long virt, unsigned long flag, unsigned long data);

static unsigned long maxnr_page_per_section = -1UL;

static unsigned int image_mode=IMAGE_MODE_NONE;
module_param_named(mode, image_mode, uint, S_IRUGO | S_IWUSR);
static char wbi_mem_dst[64];
module_param_string(mem_dst, wbi_mem_dst, sizeof(wbi_mem_dst), S_IRUGO | S_IWUSR);
static char wbi_mem_work[64];
module_param_string(mem_work, wbi_mem_work, sizeof(wbi_mem_work), S_IRUGO | S_IWUSR);
static unsigned long wbi_tmo = 10 * MSEC_PER_SEC;
module_param_named(tmo, wbi_tmo, ulong, S_IRUGO | S_IWUSR);
#define WBI_PORT_UNDEF 0xfffffffUL
static unsigned long wbi_cancel = WBI_PORT_UNDEF;
module_param_named(cancel, wbi_cancel, port, S_IRUSR);
static int resume_drop = 0;
module_param_named(resume_drop, resume_drop, int, S_IRUSR|S_IWUSR);
static int wbi_boot_drop = 1;
module_param_named(boot_drop, wbi_boot_drop, bool, S_IRUSR);
static int wbi_ignore_drop = 0;
module_param_named(ignore_drop, wbi_ignore_drop, bool, S_IRUSR|S_IWUSR);
static int wbi_errmode = 0;
module_param_named(errmode, wbi_errmode, int, S_IRUSR|S_IWUSR);
static int wbi_chksum = 0;
module_param_named(chksum, wbi_chksum, bool, S_IRUSR|S_IWUSR);


/*------------- Kernel API -----------------*/
int wbi_add_region(ulong phys, ulong size)
{
	struct wb_mem_reg *mem;

	if (!phys || (phys & ~PAGE_MASK)
	    || !size || (size & ~PAGE_MASK)) {
		printk(KERN_ERR "ERROR:WBI:invalid param:0x%lx,0x%lx\n",
		       phys, size);
		return -EINVAL;
	}

	if (!(mem = kmalloc(sizeof(struct wb_mem_reg), GFP_KERNEL))) {
		return -ENOMEM;
	}
	memset(mem, 0, sizeof(*mem));
	mem->user_data.act = 1;
	mem->user_data.virt = 0;
	mem->user_data.phys = phys;
	mem->user_data.size = size;

	down(&wb_mem_sem);
	list_add_tail(&mem->list, &wb_mem_list);
	up(&wb_mem_sem);

	printk(KERN_INFO "wbi: added region 0x%08lx@%08lx\n", size, phys);
	return 0;
}
EXPORT_SYMBOL(wbi_add_region);
/*------------------------------------------*/

/*
  interfaces to warmboot storager driver
 */
static ssize_t image_mode_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", image_mode);
}

static ssize_t image_mode_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned char char_value = *(unsigned char *)buf;
	int value;

	if(n == 0)
		return -EINVAL;

	if(char_value < '0')
		return 0;

	value = char_value - '0';
	if(value<IMAGE_MODE_NUM)
		image_mode = value;
	else
		return -EINVAL;

	return n;
}

static ssize_t image_dev_show(struct kobject *kobj,
			      struct kobj_attribute *attr, char *buf)
{
	struct wb_device *wb_device;
	int offs = 0;

	if(down_interruptible(&wb_device_sem))
		return -EINTR;

	list_for_each_entry(wb_device, &wb_device_list, list) {
		offs += scnprintf(buf+offs, PAGE_SIZE-offs, "%s%s\n",
			       wb_device==wb_default_device?"*":" ",
			       wb_device->name);
	}

	up(&wb_device_sem);
	return offs;
}

static ssize_t image_dev_store(struct kobject *kobj,
			       struct kobj_attribute *attr,
			       const char *buf, size_t n)
{
	char *endp;
	int value;
	struct wb_device *wb_device;

	if(n >= PAGE_SIZE)
		return -EINVAL;

	value = simple_strtoul(buf, &endp, 0);
	/* ignore endp */

	if(!value)
		return -EINVAL;

	if(down_interruptible(&wb_device_sem))
		return -EINTR;

	list_for_each_entry(wb_device, &wb_device_list, list) {
		value--;
		if(value == 0){
			wb_default_device = wb_device;
			break;
		}
	}

	up(&wb_device_sem);
	return value?-EINVAL:n;
}

#define RECORD_LEN (sizeof("0xXXXXXXXX,0xXXXXXXXX,0xXXXXXXXXK\n")-1)
#define USER_DATA_LEN sizeof(struct wb_user_mem_reg)
#define USER_DATA_MASK (~(USER_DATA_LEN-1))

static ssize_t image_mem_show(struct kobject *kobj,
			      struct kobj_attribute *attr, char *buf)
{
	struct wb_mem_reg *mem_reg;
	int offs = 0;

	if(down_interruptible(&wb_mem_sem))
		return -EINTR;

	list_for_each_entry(mem_reg, &wb_mem_list, list) {
		offs += scnprintf(buf+offs, PAGE_SIZE-offs, "%8lX,%8lX,%8luK\n",
				    mem_reg->user_data.phys,
				    mem_reg->user_data.virt,
				    mem_reg->user_data.size >> 10);
	}

	up(&wb_mem_sem);

	return offs;
}

static ssize_t image_mem_store(struct kobject *kobj,
			       struct kobj_attribute *attr,
			       const char *buf, size_t n)
{
	struct wb_mem_reg *mem_reg = NULL;
	struct wb_user_mem_reg *user_data = NULL;
	unsigned long count = n;
	ssize_t ret = n;

	if(count & ~USER_DATA_MASK)
		return -EINVAL;

	if(down_interruptible(&wb_mem_sem))
		return -EINTR;

	while(count){
		count -= USER_DATA_LEN;

		if(!(mem_reg = kmalloc(sizeof(struct wb_mem_reg), GFP_KERNEL))){
			ret = -ENOMEM;
			goto error;
		}

		memset(mem_reg, 0, sizeof(*mem_reg));
		user_data = &mem_reg->user_data;
		memcpy(user_data, buf, USER_DATA_LEN);
		buf += USER_DATA_LEN;

		/*
		  valid value:
		  1, phys && size
		  2, phys, size, virt page alignment
		  3, act 0 || 1
		*/
		if(user_data->act == 0) {//clear all memory region
			printk(KERN_INFO "clear all memory region\n");

			kfree(mem_reg);

			while(!list_empty(&wb_mem_list)){
				mem_reg = list_entry(wb_mem_list.next, struct wb_mem_reg, list);

				list_del(&mem_reg->list);
				kfree(mem_reg);
			}

			continue;
		}

		if((!user_data->phys)
		   || (user_data->phys & ~PAGE_MASK)
		   || (!user_data->size)
		   || (user_data->size & ~PAGE_MASK)
		   || (user_data->virt & ~PAGE_MASK)
		   || (user_data->act != 1))
		{
			kfree(mem_reg);
			ret = -EINVAL;
			break;
		}

		printk(KERN_INFO "add memory region: %8lX(@%08lX),%8lX(%ldK)\n",
		       user_data->phys,
		       user_data->virt,
		       user_data->size,
		       user_data->size >> 10);

		list_add_tail(&mem_reg->list, &wb_mem_list);
	}

 error:
	up(&wb_mem_sem);
	return ret;
}

/* erase the header */
static int __wbi_drop(unsigned char *buf, unsigned long size)
{
	int ret = 0;

	if(wb_default_device->open) {
		ret = wb_default_device->open(wb_default_device);
		if (ret) {
			printk("%s: cannot open ret = %d\n", __func__, ret);
			return ret;
		}
	}

	memset(buf, 0, size);
	ret = wb_default_device->write_sector(wb_default_device, buf, 0, size >> 9);
	if (ret)
		printk("%s: cannot write = %d\n", __func__, ret);
	else
		printk("dropped wbi\n");

	if(wb_default_device->close)
		wb_default_device->close(wb_default_device);

	return ret;
}

static char *wbi_sectorbuf;

static int _wbi_drop(void)
{
	int ret = 0;

	down(&wb_device_sem);

	if (!image_mode)
		goto out;
	if (!wb_default_device)
		goto out;
	if (!wbi_sectorbuf)
		goto out;

	ret = __wbi_drop(wbi_sectorbuf, DEFAULT_SECTOR_SIZE);

out:
	up(&wb_device_sem);

	return ret;
}

int wbi_drop(void)
{
	int ret = 0;

	if (!wbi_ignore_drop) {
		ret = _wbi_drop();
	}
	return ret;
}

EXPORT_SYMBOL(wbi_drop);

int wbi_resume_drop(void)
{
	int ret = 0;

	if (!image_mode)
		return 0;

	if (resume_drop) {
		ret = _wbi_drop();
	}

	return ret;
}

static ssize_t image_drop_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t image_drop_store(struct kobject *kobj,
			        struct kobj_attribute *attr,
			        const char *buf, size_t n)
{
	int ret;

	if (n == 0 || *buf != '1')
		return -EINVAL;

	ret = _wbi_drop();

	if (!ret)
		ret = n;

	return ret;
}

wb_attr(image_mode);
wb_attr(image_dev);
wb_attr(image_mem);
wb_attr(image_drop);

#define WBI_CMDLINE_MAX_ENTRY 16
static struct wb_mem_reg wbi_cmdline_mem[WBI_CMDLINE_MAX_ENTRY];
static unsigned int wbi_cmdline_idx = 0;

static unsigned long wbi_cmdline_get_val(char *p, char **retp)
{
	unsigned long val = memparse(p, retp);

	if (val == 0 || val & ~PAGE_MASK) {
		printk(KERN_ERR "wbi: cmdline invalid value %s\n", p);
		return 0;
	}

	return val;
}

static void __init wbi_cmdline_add_region(unsigned long phys, unsigned long size)
{
	struct wb_mem_reg *mem;

	down(&wb_mem_sem);

	if (wbi_cmdline_idx >= WBI_CMDLINE_MAX_ENTRY) {
		printk(KERN_ERR "wbi: cmdline too many entry %d (>=%d)\n", wbi_cmdline_idx, WBI_CMDLINE_MAX_ENTRY);
		goto out;
	}

	mem = &wbi_cmdline_mem[wbi_cmdline_idx++];
	mem->user_data.act = 1;
	mem->user_data.virt = 0;
	mem->user_data.phys = phys;
	mem->user_data.size = size;

	list_add_tail(&mem->list, &wb_mem_list);

	printk(KERN_INFO "wbi: added region 0x%08lx@%08lx\n", size, phys);

#ifdef CONFIG_EJ_CLEAR_MEMORY
#define LOADER_REGION_END (DDR_BASE + 0x28000)
	if (wbi_memclr()  &&  LOADER_REGION_END < phys) {
		printk(KERN_INFO "memclr: 0x%lx-0x%lx\n", phys, phys+size-1);
		memset((void *)PHYS_TO_NONCACHE(phys), 0, size);
	}
#endif /* CONFIG_EJ_CLEAR_MEMORY */

out:
	up(&wb_mem_sem);
}

static int wbi_cmdline_parse(char *p, unsigned long *phys, unsigned long *size)
{
	*size = wbi_cmdline_get_val(p, &p);
	if (!*size)
		return 1;

	if (*p != '@') {
		printk(KERN_ERR "wbi: cmdline invalid delimiter %s\n", p);
		return 1;
	}

	p++;

	*phys = wbi_cmdline_get_val(p, &p);
	if (!*phys)
		return 1;

	return 0;
}

static int __init wbi_cmdline_parse_mem(char *p)
{
	unsigned long phys, size;

	if (wbi_cmdline_parse(p, &phys, &size)) return 1;

	wbi_cmdline_add_region(phys, size);

	return 1;
}
__setup("wbimem=", wbi_cmdline_parse_mem);

static int __init create_warm_boot_proc(void)
{
	int error = 0;

	error = sysfs_create_file(power_kobj, &image_mode_attr.attr);
	if (error)
		goto out;
	error = sysfs_create_file(power_kobj, &image_dev_attr.attr);
	if (error){
		goto out;
	}
	error = sysfs_create_file(power_kobj, &image_mem_attr.attr);
	if (error){
		goto out;
	}
	error = sysfs_create_file(power_kobj, &image_drop_attr.attr);
	if (error){
		goto out;
	}

 out:
	return error;
}

late_initcall(create_warm_boot_proc);

static void iounmap_ext_pages(void)
{
	struct wb_mem_reg *mem_reg;

	down(&wb_mem_sem);

	list_for_each_entry(mem_reg, &wb_mem_list, list) {
		if(mem_reg->user_data.virt)
			continue;

		if(mem_reg->io_virt){
			iounmap((void __iomem *)mem_reg->io_virt);
			mem_reg->io_virt = 0;
		}
	}

	up(&wb_mem_sem);
}

static int ioremap_ext_pages(void)
{
	struct wb_mem_reg *mem_reg;
	const char __user *buf;
	unsigned long test, i;
	int error = 0;

	down(&wb_mem_sem);

	list_for_each_entry(mem_reg, &wb_mem_list, list) {
		if(mem_reg->user_data.virt){
			mem_reg->io_virt = mem_reg->user_data.virt;

			for(i = 0, buf = (const char __user *)(mem_reg->io_virt);
			    i < mem_reg->user_data.size>>PAGE_SHIFT;
			    i++, buf += PAGE_SIZE)
			{
				if(copy_from_user(&test, buf, sizeof(test))){
					printk(KERN_INFO "cannot access address: %p(%08lx,%08lx,%08lx)\n",
					       buf,
					       mem_reg->user_data.phys,
					       mem_reg->io_virt,
					       mem_reg->user_data.size);
					break;
				}
			}

			if(i == mem_reg->user_data.size>>PAGE_SHIFT){
				printk(KERN_INFO "using user provided virt address: %8lX\n",
				       mem_reg->io_virt);
				continue;
			}

			mem_reg->user_data.virt = 0;
		}

		mem_reg->io_virt = (unsigned long) ioremap_nocache(mem_reg->user_data.phys,
							mem_reg->user_data.size);

		if(!mem_reg->io_virt){
			printk(KERN_INFO "ioremap error: %8lX,%8luK",
			       mem_reg->user_data.phys,
			       mem_reg->user_data.size >> 10);

			error = -ENOMEM;

			break;
		}
	}

	up(&wb_mem_sem);

	return error;
}

#ifdef CONFIG_EJ_WBI_DEBUG
#include <mach/kslog.h>
#endif /* CONFIG_EJ_WBI_DEBUG */

void wb_printk_init(void)
{
#ifdef CONFIG_EJ_WBI_DEBUG
	kslog_init();
#endif /* CONFIG_EJ_WBI_DEBUG */
}

int wb_printk(const char *fmt, ...)
{
	va_list args;
	int len;

	if(wb_default_device->flag&WBI_SILENT_MODE)
		return 0;

	va_start(args, fmt);

	len = vprintk(fmt, args);
#ifdef CONFIG_EJ_WBI_DEBUG
	vksprintf(fmt, args);
#endif /* CONFIG_EJ_WBI_DEBUG */

	va_end(args);

	return len;
}

static inline unsigned long default_flags(void)
{
	unsigned long flag=0;
#ifdef CONFIG_WBI_COMPRESS
	flag |= WBI_COMPRESS;
#endif
#ifdef CONFIG_WBI_NOT_KERNEL_RO
	flag |= WBI_NOT_KERNEL_RO;
#endif
	if (wbi_chksum) {
		flag |= WBI_CKSUM;
	}
#ifdef CONFIG_WBI_LZ77_COMPRESS
	flag |= WBI_LZ77_COMPRESS;
#endif
	return flag;
}

enum {
	START=0,
	DOING,
	END,
};

static int count_ext_pages(callback_t callback, unsigned long data)
{
	struct wb_mem_reg *mem_reg;
	int phys, off;
	int ret;

	list_for_each_entry(mem_reg, &wb_mem_list, list) {
		for(phys = mem_reg->user_data.phys, off = 0;
		    off < mem_reg->user_data.size;
		    off += PAGE_SIZE, phys += PAGE_SIZE)
		{
			if((ret=callback(phys, mem_reg->io_virt + off, DOING, data)))
				return ret;
		}
	}

	return 0;
}

static void free_some_memory(unsigned mode)
{
#if defined (CONFIG_SWAP) && defined (CONFIG_SNSC_SSBOOT)
	const char *swapfile = ssboot_get_swapfile();

	if (get_ssboot_stat() != SSBOOT_CREATE_MINSSBI)
		return;

	if (swapfile) {
		ssboot_set_swapfile(swapfile);
		ssboot_swapon();
	}
	printk("Freeing memory(use swap parititon): ");
	while (__shrink_all_memory(10*1024*1024, 1, 1)) {
		printk("(%s):Active(anon)%lu Inactive(anon)%lu\n",
		       __FUNCTION__, global_page_state(NR_ACTIVE_ANON), global_page_state(NR_INACTIVE_ANON));
		msleep(100);
	}
	while (global_page_state(NR_WRITEBACK));
	drain_all_pages();
	lru_add_drain_all();

	if (swapfile) {
		ssboot_swap_set_ro();
	}
#else
	if(mode!=IMAGE_MODE_MIN) return;
	printk("Freeing memory: ");
	while (shrink_all_memory(10000))
		printk(".");
#endif
	printk("|\n");
}

static int is_head_of_free_region(struct page *page)
{
        struct zone *zone = page_zone(page);
	int order, t;
	struct list_head *curr;

	for_each_migratetype_order(order, t) {
		list_for_each(curr, &zone->free_area[order].free_list[t]) {
			if (page == list_entry(curr, struct page, lru)) {
				return 1 << order;
			}
		}
	}

        return 0;
}

extern const void _kern_text_start,_kern_text_end, _stext, _end;
static int is_kernel_ro_pfn(unsigned long pfn)
{
	unsigned long kn_pfn_start = PFN_DOWN(__pa(&_kern_text_start));
	unsigned long kn_pfn_end = PFN_UP(__pa(&_kern_text_end));

	if(default_flags() & WBI_NOT_KERNEL_RO)
		return (pfn>=kn_pfn_start && pfn<kn_pfn_end);
	else
		return 0;
}

static int saveable(struct zone * zone, unsigned long * zone_pfn)
{
	unsigned long pfn = *zone_pfn + zone->zone_start_pfn;
	unsigned long chunk_size;
	struct page * page;

	watchdog_touch();
	if (!pfn_valid(pfn))
		return 0;

	page = pfn_to_page(pfn);

	if (is_kernel_ro_pfn(pfn)){
		return 0;
	}

	if (pfn_to_nid(pfn) != 0) {
		return 0;
	}

	if (PageReserved(page) && is_ext_reserved_area(PFN_PHYS(pfn))) {
		return 0;
	}

	if ((chunk_size = is_head_of_free_region(page))) {
		*zone_pfn += chunk_size - 1;
		return 0;
	}

	return 1;
}

static int count_data_pages(callback_t callback, unsigned long data)
{
	struct zone *zone;
	unsigned long zone_pfn;
	unsigned long phys, virt;
	int ret;

	for_each_zone(zone) {
		if (!is_highmem(zone)) {
			for (zone_pfn = 0; zone_pfn < zone->spanned_pages; ++zone_pfn)
				if(saveable(zone, &zone_pfn)) {
					phys = PFN_PHYS(zone_pfn + zone->zone_start_pfn);
					virt = (unsigned long)phys_to_virt(phys);
					if((ret=callback(phys, virt, DOING, data)))
						return ret;
				}
		}
	}

	return 0;
}

static int count_pages(callback_t callback, unsigned long data)
{
	int ret;

	if((ret=callback(0, 0, START, data)))
		return ret;

	if((ret=count_data_pages(callback, data)))
		return ret;

	if((ret=count_ext_pages(callback, data)))
		return ret;

	if((ret=callback(0, 0, END, data)))
		return ret;

	return 0;
}

void wbi_calc_cksum(unsigned long *cksum, unsigned long *buf, unsigned long size)
{
	int i;

	for(i=0; i<size/4; i++){
		*cksum ^= buf[i];
	}
}

void wbi_recalc_cksum(unsigned long *cksum, unsigned long *buf, unsigned long size)
{
	if(!(default_flags() & WBI_CKSUM)) return;
	if(!cksum) return;
	wbi_calc_cksum(cksum, buf, size);
}

#include <mach/cxd90014_cfg.h>
#include <mach/regs-gpio.h>
int wbi_is_canceled(void)
{
	unsigned long port, bit;
	unsigned long val;

	if (wbi_cancel == WBI_PORT_UNDEF)
		return 0;

	port = wbi_cancel & 0xff;
	bit = (wbi_cancel >> 8) & 0xff;

	val = readl(VA_GPIO(port,RDATA));

	return !!(val & (1 << bit));
}

static ulong wbi_expire;

int wbi_is_timeout(void)
{
	return time_after_eq(mach_read_cycles(), wbi_expire);
}

//#define MAX_SECTIONS 1024

static unsigned long cur_page, cur_page_virt, cur_section, sections, pages, nr_page;
static unsigned long section_data_order;
static struct wb_section *section_data;

int wbi_section_idx(struct wb_section *p)
{
	return p - section_data;
}

/*
  we append 1 section for the possibility of new section
  because of section_data memory allocation
*/
#define ADD_SECTION_MARGIN(sect) do {sect += 64 + sect / 2;} while(0)

static int count_sections_p1(unsigned long phys, unsigned long virt, unsigned long flag, unsigned long data)
{
	if(flag==START){
		cur_page = -1UL;
		cur_page_virt = -1UL;
		sections = 0;
		pages = 0;
		nr_page = 0;
		return 0;
	}
	else if(flag==END){
		ADD_SECTION_MARGIN(sections);

		if(section_data){
			wb_msg("freeing section_data: %p\n", section_data);
			vfree(section_data);
			section_data = NULL;
		}

		wbheader.sections = sections;
		section_data = vmalloc(sections * sizeof(*section_data));
		if(!section_data){
			wb_msg("section_data allocation failed\n");
			return -ENOMEM;
		}

		pages += (1<<section_data_order);
		wb_msg("%s:%ld, %ld\n", __FUNCTION__, sections, pages);

		return 0;
	}

	// a new section, we consider physical & virtual continue area to be one section
	if(++nr_page >= maxnr_page_per_section
           || (phys != (cur_page+PAGE_SIZE) || virt != (cur_page_virt+PAGE_SIZE))){
		nr_page = 0;
		sections ++;
	}

	cur_page = phys;
	cur_page_virt = virt;

	pages ++;
	return 0;
}

static unsigned long calc_o_image_size(struct wb_header *header)
{
	unsigned long image_header = RU_SECTOR_SIZE(sizeof(*header));
	unsigned long section_headers = RU_SECTOR_SIZE(header->sections*sizeof(struct wb_section));

	return (image_header + section_headers + header->o_data_size);}

static unsigned long calc_r_image_size(struct wb_header *header)
{
	unsigned long image_header = RU_SECTOR_SIZE(sizeof(*header));
	unsigned long section_headers = RU_SECTOR_SIZE(header->sections*sizeof(struct wb_section));

	/*
	  it is hard to estimate the size of compressed-WBI in current stage,
	  and we just suppose the maximum deflation rate to be 50%
	  so it may fail during the tranfer of WBI because of size-overflow
	  even though the capacity check is passed
	 */
	if(default_flags() & WBI_COMPRESS){
		if(header->o_data_size == header->r_data_size)
			return (image_header + section_headers + (header->o_data_size / 5));
		else
			return (image_header + section_headers + header->r_data_size);
	}

	return (image_header + section_headers + header->o_data_size);
}

static int count_sections_p2(unsigned long phys, unsigned long virt, unsigned long flag, unsigned long data)
{
	struct wb_header *header = (struct wb_header *)data;

	if(flag==START){
		cur_page = -1UL;
		cur_page_virt = -1UL;
		cur_section = -1UL;
		sections = 0;
		pages = 0;
		nr_page = 0;
		return 0;
	}
	else if(flag==END){
		BUG_ON(sections > header->sections);

		header->sections = sections;
		header->o_data_size = pages*PAGE_SIZE;
		header->r_data_size = header->o_data_size;
		INIT_CKSUM(header->kern_cksum);

		if(default_flags() & WBI_NOT_KERNEL_RO){
			unsigned long kn_pfn_start = PAGE_ALIGN(__pa(&_kern_text_start));
			unsigned long kn_pfn_end = __pa(&_kern_text_end)&PAGE_MASK;

			header->kern_start = kn_pfn_start;
			header->kern_size = (kn_pfn_end-kn_pfn_start);
			wbi_recalc_cksum(&header->kern_cksum,
					 phys_to_virt(kn_pfn_start),
					 (kn_pfn_end-kn_pfn_start));

			wb_msg("%s:%08lx,%08lx -> %08lx\n", __FUNCTION__, kn_pfn_start, kn_pfn_end, header->kern_cksum);
		}

		wb_msg("%s:%ld, %ld, %08lx(%08lx)\n", __FUNCTION__, header->sections, pages,
		       calc_o_image_size(header), calc_r_image_size(header));

		return 0;
	}

	if(++nr_page >= maxnr_page_per_section
	   || (phys != (cur_page+PAGE_SIZE) || virt != (cur_page_virt+PAGE_SIZE))){// a new section
		sections ++;
		cur_section ++;
		nr_page = 0;

		INIT_CKSUM(section_data[cur_section].cksum);
		section_data[cur_section].addr = phys;
		section_data[cur_section].olen = 0;
		section_data[cur_section].flag = 0;
		section_data[cur_section].rlen = 0;
		section_data[cur_section].virt = virt;
	}

	cur_page = phys;
	cur_page_virt = virt;

	pages ++;
	section_data[cur_section].olen += PAGE_SIZE;

	return 0;
}

static int do_make_image(unsigned long entry, unsigned long adjust_vma)
{
	int ret = 0;
	u32 t0;

	wb_printk_init();

	if (wbi_is_canceled()) {
		ret = -ECANCELED;
		goto close_error;
	}
	/* set timeout */
	wbi_expire = mach_read_cycles() + mach_usecs_to_cycles(wbi_tmo*USEC_PER_MSEC);

	t0 = mach_read_cycles();
	wbheader.resume_vector = virt_to_phys((void *)entry);

	if(wb_default_device->open)
		if((ret=wb_default_device->open(wb_default_device)<0))
			goto send_error;

	do {
		unsigned long image_size;

		watchdog_touch();
		/*
		  phase 2:
		  count real sections,
		  fill section meta data,
		*/
		if((ret=count_pages(count_sections_p2, (unsigned long)&wbheader)))
			goto send_error;

		image_size = calc_r_image_size(&wbheader);
		if(wb_default_device->capacity
		   && wb_default_device->capacity<image_size){
#ifdef CONFIG_SNSC_SSBOOT
			wb_msg("Warning: %s image size may be greater than device capacity!%lu > %lu\n",
			       __FUNCTION__, image_size, wb_default_device->capacity);
#else
			unsigned long shrink_pages = image_size - wb_default_device->capacity;
			unsigned long shrinked_pages = 0, to_shrink, tmp;

			ret = -ENOSPC;
			if (image_mode != IMAGE_MODE_ADP) {
				wb_msg("error: %s image size greater than device capacity!%lu > %lu\n", __FUNCTION__, image_size, wb_default_device->capacity);
				goto send_error;
			}

			shrink_pages = PAGE_ALIGN(shrink_pages)>>PAGE_SHIFT;
			to_shrink = shrink_pages;

			wb_msg("Image too huge!\n");
			wb_msg("Trying to free %lu pages:", shrink_pages);

			while (to_shrink && (tmp=shrink_all_memory(to_shrink))) {
				shrinked_pages += tmp;
				to_shrink -= tmp;
				watchdog_touch();
			}

			wb_msg("%lu freed -> %s\n", shrinked_pages, shrinked_pages>=shrink_pages?"OK":"FAIL");

			if (shrinked_pages>=shrink_pages)
				continue;

			goto send_error;
#endif
		}

		break;
	} while(1);

	/* send image header */
	memcpy(wbheader.fVersion, WBI_INVAL_VER, 4);
	if((ret=wbi_send_data(&wbheader, sizeof(wbheader)))) {
		goto send_error;
	}
	if((ret=wbi_flush()))
		goto send_error;

	/*
	  phase 2:
	  send section
	  and section meta data
	*/
	if((ret=wbi_send_sections(&wbheader, section_data, sections)))
		goto send_error;

	wb_msg("%s: %lu sections, %ld pages, %08lx->%08lx\n",
	       __FUNCTION__, sections, pages,
	       calc_o_image_size(&wbheader), calc_r_image_size(&wbheader));

	if((ret=wbi_flush()))
		goto send_error;

	/* overwrite image header */
	wb_msg("rewrite header for compressed image!\n");
#ifdef CONFIG_SNSC_SSBOOT
	switch (get_ssboot_stat()) {
	case SSBOOT_CREATE_MINSSBI :
		memcpy(wbheader.fVersion, WBI_PROF_VER, 4);
		break ;
	case SSBOOT_CREATE_OPTSSBI :
		memcpy(wbheader.fVersion, WBI_PRE_VER, 4);
		break ;
	default :
		printk(KERN_ERR "%s: Warning: unexpected case (%lu).\n",
		       __FUNCTION__, get_ssboot_stat());
		break ;
	}
#else
	memcpy(wbheader.fVersion, WBI_VER, 4);
#endif
	wbi_rewrite_header(&wbheader);

	if(wb_default_device->close) {
		ret = wb_default_device->close(wb_default_device);
		if (ret)
			goto close_error;
	}

	t0 = mach_read_cycles() - t0;
	wb_printk("do_make_image: %d [us]\n", mach_cycles_to_usecs(t0));

	wbi_stat();
	return 0;

 send_error:
	if(wb_default_device->close)
		wb_default_device->close(wb_default_device);

	if(ret == -ENOSPC && default_flags() & WBI_COMPRESS){
		wb_msg("NO SPACE error!\n");
		ret = 0;
	}

	__wbi_drop(wbi_sectorbuf, DEFAULT_SECTOR_SIZE);

 close_error:

	if (ret == -ECANCELED) {
		wb_printk("%s: canceled to create wbi\n", __FUNCTION__);
		return 0;
	}

	if (!wbi_errmode) {
		wb_printk("%s: ret=%d\n", __FUNCTION__, ret);
		return 0;
	}

	wb_printk("%s: SUSPNED is aborted - error code (%d)!\n", __FUNCTION__, ret);
	return ret;
}

/*
  to disable the Memory Dump from Warm-boot-image booted kernel
 */
int cxd_create_warmbootimage(unsigned long entry, unsigned long adjust_vma)
{
	if(!WBI_MODE(pm_state, image_mode))
		return 0;

	if(wb_default_device == NULL)
	    return 0;

	return do_make_image(entry, adjust_vma);
}

int cxd_rewrite_wbheader(const char *fver, unsigned long entry)
{
	int ret = 0;

	if(wb_default_device->open)
		if((ret=wb_default_device->open(wb_default_device)<0))
			goto send_error;

	wbi_read_header(&wbheader);
	memcpy(wbheader.fVersion, fver, 4);
	wbheader.resume_vector = virt_to_phys((void *)entry);
	wbi_rewrite_header(&wbheader);

	if(wb_default_device->close) {
		ret = wb_default_device->close(wb_default_device);
		if (ret)
			goto close_error;
	}
	return 0;

 send_error:
	if(wb_default_device->close)
		wb_default_device->close(wb_default_device);

 close_error:
	if (ret == -ECANCELED) {
		wb_printk("%s: canceled to create wbi\n", __FUNCTION__);
		return 0;
	}

	if (!wbi_errmode) {
		wb_printk("%s: ret=%d\n", __FUNCTION__, ret);
		return 0;
	}

	wb_printk("%s: SUSPNED is aborted - error code (%d)!\n", __FUNCTION__, ret);

	return ret;
}

static struct wb_device *wb_prev_drop;
/*
  interfaces to warmboot storage driver
 */
int wb_register_device(struct wb_device *device,int def)
{
	int ret = 0;

	if(!device)
		return -ENODEV;

	down(&wb_device_sem);

	list_add_tail(&device->list, &wb_device_list);

	if(def)
		wb_default_device = device; /* overwrite lately registerd */

	if(device->phys_sector_size==0)
		device->phys_sector_size = DEFAULT_SECTOR_SIZE;

	if(device->sector_size<512){
		device->sector_size = 512;
		printk("Warning: set sector size of %s to 512!\n", device->name);
	}
	if (!wbi_sectorbuf) {
		wbi_sectorbuf = kmalloc(DEFAULT_SECTOR_SIZE, GFP_KERNEL);
		if (!wbi_sectorbuf) {
			printk("%s: allocate buf failed!\n", __FUNCTION__);
			ret = -ENOMEM;
			goto out;
		}
	}

	wb_msg("wb_default_device: sector_size=0x%lx,phys_sector_size=0x%lx\n",
	       wb_default_device->sector_size,
	       wb_default_device->phys_sector_size);

	if (wbi_boot_drop  &&  wb_prev_drop != wb_default_device) {
		wb_prev_drop = wb_default_device;
		__wbi_drop(wbi_sectorbuf, DEFAULT_SECTOR_SIZE);
	}

 out:
	up(&wb_device_sem);

	return ret;
}

int wb_unregister_device(struct wb_device *device)
{
	struct list_head *ptr;

	if(!device)
		return -ENODEV;

	down(&wb_device_sem);

	list_del(&device->list);
	if(list_empty(&wb_device_list)){
		wb_default_device = NULL;
	}
	else if(wb_default_device == device){
		ptr = wb_device_list.next;
		wb_default_device = list_entry(ptr, struct wb_device, list);
	}
	if (wbi_sectorbuf) {
		kfree(wbi_sectorbuf);
		wbi_sectorbuf = NULL;
	}

	up(&wb_device_sem);

	return 0;
}

int wbi_register_compressor(struct wbi_compressor *compressor)
{
	if(!compressor)
		return -EINVAL;

	down(&wb_device_sem);

	wbi_default_compressor = compressor; /* overwrite lately registerd */

	up(&wb_device_sem);

	return 0;
}

int warm_boot_pm_begin(suspend_state_t state)
{
	int ret;
	ulong work, wlen, dst, dlen;

	pm_state = state;

	if(!WBI_MODE(pm_state, image_mode) || !wb_default_device)
		return 0;

	BOOT_TIME_ADD1("B warm_boot_pm_begin");
	if ((default_flags() & WBI_COMPRESS) && wbi_default_compressor) {
		unsigned long max_slen;

		if (!wbi_default_compressor->deflate
		    || !wbi_default_compressor->get_max_slen
		    || wbi_cmdline_parse(wbi_mem_dst, &dst, &dlen)
		    || wbi_cmdline_parse(wbi_mem_work, &work, &wlen))
			return -EINVAL;

		if (wbi_setup(work, &wlen, dst, &dlen) < 0)
			return -EINVAL;

		max_slen = wbi_default_compressor->get_max_slen(dlen, wlen);
		maxnr_page_per_section = max_slen / PAGE_SIZE;
		if (!maxnr_page_per_section) return -EINVAL;

		if (wbi_default_compressor->init) {
			ret = wbi_default_compressor->init();
			if (ret) return ret;
		}
	}

	free_some_memory(image_mode);

	wbheader.flag = default_flags();
	wbheader.sector_size = wb_default_device->phys_sector_size;

	/*phase 1: estimate sections*/
	ret = count_pages(count_sections_p1, (unsigned long)&wbheader);
	if (ret) goto out;

	BOOT_TIME_ADD1("E warm_boot_pm_begin");
	return 0;

out:
	if ((default_flags() & WBI_COMPRESS) && wbi_default_compressor) {
		if (wbi_default_compressor->exit)
			wbi_default_compressor->exit();
	}

	return ret;
}

/* check if still opened files */
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/fdtable.h>
#include <mach/errind.h>

static int wbi_get_opened_mnt(char *name, struct vfsmount **mnt)
{
	int ret;
	struct path path;

	*mnt = NULL;

	if (*name != '/') {
		wb_printk("wbi: check if opened files: %s is invalid path\n", name);
		return -EINVAL;
	}

	ret = kern_path(name, 0, &path);
	if (ret) {
		wb_printk("wbi: check if opened files: %s is invalid (%d)\n", name, ret);
		return ret;
	}

	if (!may_umount(path.mnt))
		*mnt = path.mnt;

	path_put(&path);

	return 0;
}

static void wbi_dump_opened_files(struct vfsmount *mnt)
{
	struct task_struct *g, *p;

	read_lock(&tasklist_lock);
	do_each_thread(g, p) {
		int fd;
		struct files_struct *files = get_files_struct(p);

		if (!files)
			continue;

		spin_lock(&files->file_lock);
		for (fd = 0; fd < files_fdtable(files)->max_fds; fd++) {
			struct path *path;
			struct file *file = fcheck_files(files, fd);

			if (!file)
				continue;

			path = &file->f_path;
			if (mnt == path->mnt)
				wb_printk("%s (pid:%d) still opens \'%s\' (fd:%d)\n", p->comm, p->pid, path->dentry->d_name.name, fd);
		}
		spin_unlock(&files->file_lock);

		put_files_struct(files);

	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);
}

#define MAX_MNTS 30
static char *wbi_mount_points[MAX_MNTS] = { NULL };
module_param_array_named(chk_opened, wbi_mount_points, charp, NULL, S_IRUGO | S_IWUSR);

static int wbi_chk_opened_files(void)
{
	int i, ret = 0;
	char **mpts = wbi_mount_points;

	wb_printk("checking opened files\n");
	for (i = 0; i < MAX_MNTS && *mpts; i++, mpts++) {
		struct vfsmount *mnt;
		int err = wbi_get_opened_mnt(*mpts, &mnt);

		watchdog_touch();

		if (err) {
			ret = err;
		} else if (mnt) {
			/* found opened files */
			wb_printk("WARNING: %s is opened\n", *mpts);
			wbi_dump_opened_files(mnt);
			ret = 1;
		} else {
			/* NO error */
			wb_printk("OK: checked opened files: %s\n", *mpts);
		}
	}

	return ret;
}

int warm_boot_pm_prepare(void)
{
	int ret = 0;

	if(!WBI_MODE(pm_state, image_mode) || !wb_default_device)
		return 0;

	if (wbi_chk_opened_files()) {
		wb_printk("some files are still opened, stopping all of operations\n");
		_wbi_drop();
#ifdef CONFIG_EXCEPTION_MONITOR
		errind_start(ERRIND_OOM);
#endif
		while (1)
			;
	}

	BOOT_TIME_ADD1("B warm_boot_pm_prepare");
	if(wb_default_device->suspend_prepare)
		if((ret=wb_default_device->suspend_prepare(wb_default_device)))
			goto error;

	if(ioremap_ext_pages())
		goto error;

	BOOT_TIME_ADD1("E warm_boot_pm_prepare");
	return 0;

 error:
	warm_boot_pm_finish();
	return ret;
}

void warm_boot_pm_finish(void)
{
	if(!WBI_MODE(pm_state, image_mode) || !wb_default_device)
		return;

	BOOT_TIME_ADD1("B warm_boot_pm_finish");
	iounmap_ext_pages();

	if(section_data){
		wb_msg("freeing section_data: %p\n", section_data);
		vfree(section_data);
		section_data = NULL;
	}

	if(wb_default_device->suspend_finish)
		wb_default_device->suspend_finish(wb_default_device);

	if ((default_flags() & WBI_COMPRESS) && wbi_default_compressor) {
		if (wbi_default_compressor->exit)
			wbi_default_compressor->exit();
	}
	BOOT_TIME_ADD1("E warm_boot_pm_finish");
}

suspend_state_t pm_get_state(void)
{
	return pm_state;
}

EXPORT_SYMBOL(wb_register_device);
EXPORT_SYMBOL(wb_unregister_device);
EXPORT_SYMBOL(pm_get_state);

#ifdef CONFIG_EJ_CLEAR_MEMORY
static unsigned long phys_memclr = 0UL;

static int __init parse_memclr(char *str)
{
	phys_memclr = memparse(str, &str);
	return 0;
}
early_param("wbi_memclr", parse_memclr);

#define WBI_MEMCLR_NONE	0
#define WBI_MEMCLR_EXEC	1

int __init wbi_memclr(void)
{
	unsigned long memclr;

	if (!phys_memclr) {
		return 0;
	}
	memclr = *(unsigned long *)__va(phys_memclr);
	return (WBI_MEMCLR_EXEC == memclr);
}
#endif /* CONFIG_EJ_CLEAR_MEMORY */
