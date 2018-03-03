/*
 * arch/arm/mach-cxd90014/pgscan.c
 *
 * pgscan
 *
 * Copyright 2010,2011,2012 Sony Corporation
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
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static unsigned int pgscan_records = 30;
module_param_named(n, pgscan_records, uint, S_IRUSR);
#ifdef CONFIG_NUMA
static int pgscan_node = 2;
module_param_named(node, pgscan_node, int, S_IRUSR|S_IWUSR);
#endif /* CONFIG_NUMA */

static int is_match(struct inode *ino)
{
	return S_ISREG(ino->i_mode);
}

#define NAME_LEN 14

struct pgscan_info {
	dev_t dev;
	unsigned long i_no;
	unsigned short n_pages;
	char name[NAME_LEN];
};

static struct pgscan_info *pgscan_buf;
static unsigned int n_pgscan;

static void pgscan_clear(void)
{
	n_pgscan = 0;
}

static struct pgscan_info *pgscan_get(unsigned int idx)
{
	if (idx < n_pgscan)
		return pgscan_buf + idx;
	return NULL;
}

static int pgscan_add(struct inode *ino)
{
	dev_t dev;
	unsigned long i_no;
	struct pgscan_info *p;
	int n;

	dev = ino->i_sb->s_dev;
	i_no = ino->i_ino;
	/* search entry (SLOW) */
	p = pgscan_buf;
	n = n_pgscan;
	while (--n >= 0) {
		if (p->dev == dev  &&  p->i_no == i_no) {
			p->n_pages++;
			return 0;
		}
		p++;
	}
	if (n_pgscan == pgscan_records) {
		printk(KERN_ERR "pgscan: buffer full\n");
		return -1;
	}
	/* add entry */
	p = pgscan_buf + n_pgscan++;
	p->dev = dev;
	p->i_no = i_no;
	p->n_pages = 1;
	p->name[0] = '\0';
	if (!list_empty(&ino->i_dentry)) {
		struct dentry *d;
		d = list_entry(ino->i_dentry.next, struct dentry, d_alias);
		if (d && d->d_name.name) {
			strncpy(p->name, d->d_name.name, NAME_LEN);
			p->name[NAME_LEN - 1] = '\0';
		}
	}
	return 0;
}

/* kernel API */
/* ToDO: mutual exclusion between kernel APIs and proc I/F */
void pgscan_range(unsigned long start_pfn, unsigned long end_pfn)
{
	int total;
	unsigned long pfn;
	struct page *page;
	struct inode *ino;

	printk(KERN_ERR "pgscan_range(0x%lx,0x%lx)\n", start_pfn, end_pfn);
	pgscan_clear();
	total = 0;
	for (pfn = start_pfn; pfn < end_pfn; pfn++) {
		if (!pfn_valid(pfn))
			continue;
		total++;
		page = pfn_to_page(pfn);
		if (PageReserved(page) || PageSwapCache(page)
		    || PageSlab(page) || !page_count(page)
		    || PageAnon(page))
			continue;
		if (page->mapping) {
			ino = page->mapping->host;
			if (is_match(ino)) {
				if (pgscan_add(ino) < 0) {
					break;
				}
			}
		}
	}
	printk(KERN_ERR "%d pages scanned.\n", total);
}

/* kernel API */
void pgscan_dump(void)
{
	int n;
	struct pgscan_info *p;

	p = pgscan_buf;
	n = n_pgscan;
	while (--n >= 0) {
		printk(KERN_ERR "%02x:%02x %8lu %5u %s\n", MAJOR(p->dev),
		       MINOR(p->dev), p->i_no, p->n_pages, p->name);
		p++;
	}
}

/* kernel API */
void show_pgscan(unsigned long start_pfn, unsigned long end_pfn)
{
	pgscan_range(start_pfn, end_pfn);
	pgscan_dump();
}

static void pgscan(void)
{
	int total;
	struct zone *zone;
	unsigned long pfn, end_pfn;
	struct page *page;
	struct inode *ino;

	pgscan_clear();
	total = 0;
	for_each_zone(zone) {
		if (!populated_zone(zone))
			continue;
		end_pfn = zone->zone_start_pfn + zone->spanned_pages;
		for (pfn = zone->zone_start_pfn; pfn < end_pfn; pfn++) {
			if (!pfn_valid(pfn))
				continue;
#ifdef CONFIG_NUMA
			if (zone->node != pgscan_node)
				continue;
#endif /* CONFIG_NUMA */
			total++;
			page = pfn_to_page(pfn);
			if (PageReserved(page) || PageSwapCache(page)
			    || PageSlab(page) || !page_count(page)
			    || PageAnon(page))
				continue;
			if (page->mapping) {
				ino = page->mapping->host;
				if (is_match(ino)) {
					if (pgscan_add(ino) < 0) {
						return;
					}
				}
			}
		}
	}
	printk("%d pages scanned.\n", total);
}

#ifdef CONFIG_PROC_FS
static void *pgscan_start(struct seq_file *seq, loff_t *pos)
{
	return pgscan_get(*pos);
}

static void *pgscan_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	return pgscan_get(*pos);
}

static void pgscan_stop(struct seq_file *seq, void *v)
{
}

static int pgscan_show(struct seq_file *seq, void *v)
{
	const struct pgscan_info *p = v;

	seq_printf(seq, "%02x:%02x %8lu %5u %s\n", MAJOR(p->dev),
		   MINOR(p->dev), p->i_no, p->n_pages, p->name);
	return 0;
}

static struct seq_operations pgscan_ops = {
	.start	= pgscan_start,
	.next	= pgscan_next,
	.stop	= pgscan_stop,
	.show	= pgscan_show
};

static int pgscan_open(struct inode *inode, struct file *file)
{
	pgscan();
	return seq_open(file, &pgscan_ops);
}

static struct file_operations proc_pgscan_ops = {
	.owner		= THIS_MODULE,
	.open		= pgscan_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
#endif

static int __init pgscan_init(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *ent;
#endif /* CONFIG_PROC_FS */
	int size;

	if (!pgscan_records)
		return 0;
	size = sizeof (struct pgscan_info) * pgscan_records;
	pgscan_buf = (struct pgscan_info *)vmalloc(size);
	if (!pgscan_buf) {
		printk(KERN_ERR "pgscan:ERROR: can not allocate %d (%d records)\n", size, pgscan_records);
                return -ENOMEM;
        }

#ifdef CONFIG_PROC_FS
	ent = create_proc_entry("pgscan", 0, NULL);
	if (ent)
		ent->proc_fops = &proc_pgscan_ops;
#endif /* CONFIG_PROC_FS */
	pgscan_clear();
        return 0;
}

module_init(pgscan_init);
