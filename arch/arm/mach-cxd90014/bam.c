/*
 * arch/arm/mach-cxd90014/bam.c
 *
 * BAM driver
 *
 * Copyright 2012 Sony Corporation
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
#include <linux/moduleparam.h>
#include <mach/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/regs-bam.h>
#include <mach/regs-avbckg.h>

#include <mach/bam.h>
#include <mach/bootram.h>
#include <asm/sections.h>
#include <asm/pgtable.h>

#define BAM_PROCNAME "bam"

static unsigned char __iomem *bam;
static int bam_irq = -1;
#define BAM_REG(x)	(unsigned long __iomem *)(bam + (x))
#define BAM_CH(ch,x)	(unsigned long __iomem *)(bam + BAM_CH_BASE(ch) + (x))

struct bam_info {
	unsigned long start, end;
	unsigned long mask, mask2;
	unsigned long target;
	unsigned long filter;
};
static struct bam_info bams[BAM_N_CH] = {
#if 0
	{
		.start	= (ulong)pgtbl_area_begin,
		.end	= (ulong)pgtbl_area_end,
		.mask	= 0UL,
		.mask2	= BAM_MEMCHK_DDR_MSK_CPU,
	},
	{
		.start	= (ulong)_text,
		.end	= (ulong)__init_begin,
		.mask	= 0UL,
		.mask2	= 0UL,
	},
	{
		.start	= (ulong)_data,
		.end	= (ulong)_end,
		.mask	= 0UL,
		.mask2	= BAM_MEMCHK_DDR_MSK_CPU,
	},
#else
	{
		.start	= (ulong)pgtbl_area_begin,
		.end	= (ulong)__init_begin,
		.mask	= 0UL,
		.mask2	= BAM_MEMCHK_DDR_MSK_CPU,
	},
	{
		.start	= (ulong)_data,
		.end	= (ulong)_end,
		.mask	= 0UL,
		.mask2	= BAM_MEMCHK_DDR_MSK_CPU,
	},
	{
		.start	= UL(DDR_BASE),
		.end	= UL(DDR_BASE)+0x28000,
		.mask	= 0UL,
		.mask2	= BAM_MEMCHK_DDR_MSK_CPU,
		.filter = BAM_IFLT_DDR_MSK_NAND,
	},
#endif
};

module_param_named(t0,  bams[0].target, ulong, S_IRUSR|S_IWUSR);
module_param_named(s0,  bams[0].start, ulongH, S_IRUSR|S_IWUSR);
module_param_named(e0,  bams[0].end,   ulongH, S_IRUSR|S_IWUSR);
module_param_named(m0,  bams[0].mask,  ulongH, S_IRUSR|S_IWUSR);
module_param_named(mh0, bams[0].mask2, ulongH, S_IRUSR|S_IWUSR);
module_param_named(t1,  bams[1].target, ulong, S_IRUSR|S_IWUSR);
module_param_named(s1,  bams[1].start, ulongH, S_IRUSR|S_IWUSR);
module_param_named(e1,  bams[1].end,   ulongH, S_IRUSR|S_IWUSR);
module_param_named(m1,  bams[1].mask,  ulongH, S_IRUSR|S_IWUSR);
module_param_named(mh1, bams[1].mask2, ulongH, S_IRUSR|S_IWUSR);
module_param_named(t2,  bams[2].target, ulong, S_IRUSR|S_IWUSR);
module_param_named(s2,  bams[2].start, ulongH, S_IRUSR|S_IWUSR);
module_param_named(e2,  bams[2].end,   ulongH, S_IRUSR|S_IWUSR);
module_param_named(m2,  bams[2].mask,  ulongH, S_IRUSR|S_IWUSR);
module_param_named(mh2, bams[2].mask2, ulongH, S_IRUSR|S_IWUSR);
module_param_named(f0,  bams[0].filter, ulongH, S_IRUSR|S_IWUSR);
module_param_named(f1,  bams[1].filter, ulongH, S_IRUSR|S_IWUSR);
module_param_named(f2,  bams[2].filter, ulongH, S_IRUSR|S_IWUSR);

static int bam_mode = 1;
module_param_named(mode, bam_mode, bool, S_IRUSR);
static int bam_noirq = 0;
module_param_named(noirq, bam_noirq, bool, S_IRUSR);
static int bam_panic = 0;
module_param_named(panic, bam_panic, bool, S_IRUSR|S_IWUSR);
static uint bam_panic_delay = 500000; /* usec */
module_param_named(panic_delay, bam_panic_delay, uint, S_IRUSR|S_IWUSR);

static void bam_dump(void)
{
	int i;
	unsigned long id, addr;

	for (i = 0; i < BAM_N_CH; i++) {
		id = readl_relaxed(BAM_CH(i, BAM_MEMCHK_NG_ID));
		addr = readl_relaxed(BAM_CH(i, BAM_MEMCHK_NG_ADDR));
		printk(KERN_ERR "BAM%d: 0x%03lx 0x%08lx\n", i, id, addr);
	}
}

/*
 * It will be too late, if someone destoryed kernel memory.
 *   If system hang up, please dump these registers via ICE.
 */
static irqreturn_t bam_intr(int irq, void *dev_id)
{
	u32 stat;

	stat = readl_relaxed(BAM_REG(BAM_INT_STATUS));
	printk(KERN_ERR "BAM:memchk:stat=0x%03x\n", stat);
	bam_dump();
	if (bam_panic) {
		ulong expire;
		smp_send_stop();
		expire = mach_read_cycles() + mach_usecs_to_cycles(bam_panic_delay);
		while (!time_after_eq(mach_read_cycles(), expire))
			;
		BUG();
	}
	writel_relaxed(stat, BAM_REG(BAM_INT_STATUS)); /* clear */
	wmb();

	return IRQ_HANDLED;
}

static struct bam_info bam_targets[] = {
	{
		.start	= DDR_BASE,
		.end	= DDR_BASE + DDR_SIZE,
		.target	= BAM_TGT_DDR,
	},
	{
		.start	= CXD90014_ESRAM_BANK(0),
		.end	= CXD90014_ESRAM_BANK(1),
		.target	= BAM_TGT_ESRAM0,
	},
	{
		.start	= CXD90014_ESRAM_BANK(1),
		.end	= CXD90014_ESRAM_BANK(2),
		.target	= BAM_TGT_ESRAM1,
	},
	{
		.start	= CXD90014_ESRAM_BANK(2),
		.end	= CXD90014_ESRAM_BANK(3),
		.target	= BAM_TGT_ESRAM2,
	},
	{
		.start	= CXD90014_ESRAM_BANK(3),
		.end	= CXD90014_ESRAM_BANK(4),
		.target	= BAM_TGT_ESRAM3,
	},
};
#define N_TARGETS	ARRAY_SIZE(bam_targets)

static void bam_setup(void)
{
	u32 target, tgt;
	int i, k;
	struct bam_info *p, *t;

	if (!bam_mode)
		return;

	target = 0x000;
	for (i = 0, p = bams; i < BAM_N_CH; i++, p++) {
		/* do align */
		p->start = (p->start + BAM_MEMCHK_ALIGN) & ~BAM_MEMCHK_ALIGN;
		p->end &= ~BAM_MEMCHK_ALIGN;

		if (p->start > p->end - 1) {
			p->start = p->end = 0UL;
		}

		/* VA-->PA */
		if (PAGE_OFFSET <= p->start && p->end <= PAGE_OFFSET+DDR_SIZE) {
			p->start = __pa(p->start);
			p->end   = __pa(p->end);
		}

		/* find target */
		tgt = BAM_TGT_NONE;
		for (k = 0, t = bam_targets; k < N_TARGETS; k++, t++) {
			if (t->start <= p->start && p->end <= t->end) {
				tgt = t->target;
				p->target = tgt;
				break;
			}
		}
		if (k == N_TARGETS) {
			tgt = p->target;
		}

		target |= tgt << (i * BAM_MON_TGT_SHIFT);

		if (BAM_TGT_NONE == tgt) {
			continue;
		}

		writel_relaxed(p->mask,  BAM_CH(i, BAM_MEMCHK_MSK_IP0));
		writel_relaxed(p->mask2, BAM_CH(i, BAM_MEMCHK_MSK_IP1));
		writel_relaxed(p->start, BAM_CH(i, BAM_MEMCHK_START));
		writel_relaxed(p->end-1, BAM_CH(i, BAM_MEMCHK_END));
		writel_relaxed(p->filter, BAM_CH(i, BAM_ID_SEL(7)));
	}
	writel_relaxed(BAM_INT_ALL, BAM_REG(BAM_INT_STATUS)); /* clear */
	writel_relaxed(BAM_INT_ALL, BAM_REG(BAM_INT_ENABLE));
	writel_relaxed(target, BAM_REG(BAM_MON_TGT));
	writel_relaxed(BAM_MODE_MEMCHK|BAM_CTRLMODE_CPU, BAM_REG(BAM_MODE_SET));
	/* start */
	writel_relaxed(BAM_MON_START, BAM_REG(BAM_MON_CTRL));
	wmb();
}

void bam_stop(void)
{
	writel_relaxed(BAM_MON_STOP, BAM_REG(BAM_MON_CTRL));
	wmb();
}
EXPORT_SYMBOL(bam_stop);

void bam_memchk_ctrl(int start)
{
	if (start) {
		bam_mode = 1;
		bam_stop();
		bam_setup();
	} else {
		bam_mode = 0;
		bam_stop();
	}
}
EXPORT_SYMBOL(bam_memchk_ctrl);

int bam_memchk(unsigned int id, unsigned long start, unsigned long end,
	       unsigned long long mask)
{
	if (id >= BAM_N_CH) {
		printk(KERN_ERR "ERROR:bam_memchk: invalid id: %u\n", id);
		return -1;
	}
	if ((start & BAM_MEMCHK_ALIGN) || (end & BAM_MEMCHK_ALIGN)) {
		printk(KERN_ERR "WARNING:bam_memchk: start and end address should be aligned (0x%08lu,0x%08lu)", start, end);
		start = (start + BAM_MEMCHK_ALIGN) & ~BAM_MEMCHK_ALIGN;
		end &= ~BAM_MEMCHK_ALIGN;
		printk(KERN_CONT "-->(0x%08lu,0x%08lu)\n", start, end);
	}
	if (start > end - 1) {
		printk(KERN_ERR "ERROR:bam_memchk: start > end-1 (0x%08lu,0x%08lu)\n", start, end - 1);
		return -1;
	}

	bams[id].start = start;
	bams[id].end = end;
	bams[id].mask = (unsigned long)mask;
	bams[id].mask2 = (unsigned long)(mask >> 32);
	return 0;
}
EXPORT_SYMBOL(bam_memchk);

int bam_setfilter(unsigned int id, unsigned long filter)
{
	if (id >= BAM_N_CH) {
		printk(KERN_ERR "ERROR:bam_setfilter: invalid id: %u\n", id);
		return -1;
	}
	bams[id].filter = filter;
	return 0;
}
EXPORT_SYMBOL(bam_setfilter);

void bam_powersave(int save)
{
	unsigned long flags;
	u32 dat;

	local_irq_save(flags);
	if (save) {
		/* CKG_ACLK_PS */
		dat = readl_relaxed(VA_AVBCKG+AVBCKG_ACLK_PS);
		dat &= ~AVBCKG_XPS_A_BAM;
		writel_relaxed(dat, VA_AVBCKG+AVBCKG_ACLK_PS);
		wmb();

		/* CKG_PCLK_PS */
		dat = readl_relaxed(VA_AVBCKG+AVBCKG_PCLK_PS);
		dat &= ~AVBCKG_XPS_P_BAM;
		writel_relaxed(dat, VA_AVBCKG+AVBCKG_PCLK_PS);
		wmb();
	} else {
		/* CKG_PCLK_PS */
		dat = readl_relaxed(VA_AVBCKG+AVBCKG_PCLK_PS);
		dat |= AVBCKG_XPS_P_BAM;
		writel_relaxed(dat, VA_AVBCKG+AVBCKG_PCLK_PS);
		wmb();

		/* CKG_ACLK_PS */
		dat = readl_relaxed(VA_AVBCKG+AVBCKG_ACLK_PS);
		dat |= AVBCKG_XPS_A_BAM;
		writel_relaxed(dat, VA_AVBCKG+AVBCKG_ACLK_PS);
		wmb();
	}
	local_irq_restore(flags);
}

static int __devinit bam_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_ERR "ERROR:BAM: can not allocate mem resource.\n");
		return -ENODEV;
	}
	bam = (unsigned char __iomem *)IO_ADDRESSP(res->start);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_ERR "ERROR:BAM: can not allocate irq resource.\n");
		return -ENODEV;
	}
	bam_irq = res->start;

	bam_powersave(false);
	bam_setup();
	if (!bam_noirq) {
		ret = request_irq(bam_irq, bam_intr,
				  IRQF_NO_THREAD|IRQF_DISABLED, "avb", NULL);
		if (ret) {
			printk(KERN_ERR "WARNING:BAM:request_irq %d failed:%d, change to NOIRQ mode.\n",
			       bam_irq, ret);
			bam_noirq = 1;
			bam_irq = -1;
		}
	}
	return 0;
}

static int __devexit bam_remove(struct platform_device *pdev)
{
	if (bam_irq >= 0) {
		free_irq(bam_irq, NULL);
	}
	return 0;
}

#ifdef CONFIG_PM
static int bam_suspend(struct platform_device *dev, pm_message_t state)
{
	if (bam_irq >= 0) {
		disable_irq(bam_irq);
	}
	return 0;
}

static int bam_resume(struct platform_device *dev)
{
	bam_powersave(false);
	bam_setup();
	if (bam_irq >= 0) {
		enable_irq(bam_irq);
	}
	return 0;
}
#endif /* CONFIG_PM */

static struct platform_driver bam_driver = {
	.probe = bam_probe,
	.remove = bam_remove,
#ifdef CONFIG_PM
	.suspend = bam_suspend,
	.resume	 = bam_resume,
#endif
	.driver = {
		.name = "bam",
	},
};

/*
   Usage
   	stop:  echo 0 > /proc/bam
	start: echo 1 > /proc/bam
*/
static ssize_t bam_write(struct file *file, const char __user *buf,
			 size_t len, loff_t *ppos)
{
	char arg[10];

	if (len > sizeof arg - 1) {
		printk(KERN_ERR "/proc/%s: too long\n", BAM_PROCNAME);
		return -1;
	}
	if (copy_from_user(arg, buf, len)) {
		return -EFAULT;
	}
	arg[len] = '\0';

	if ('0' == arg[0]) { /* stop */
		bam_memchk_ctrl(0);
	} else if ('1' == arg[0]) { /* start */
		bam_memchk_ctrl(1);
	}

	return len;
}

#define BAM_NR_SEQ	4

static void *bam_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos < BAM_NR_SEQ)
		return pos;
	return NULL;
}

static void *bam_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos < BAM_NR_SEQ)
		return pos;
	return NULL;
}

static void bam_seq_stop(struct seq_file *seq, void *v)
{
}

static int bam_seq_show(struct seq_file *seq, void *v)
{
	int i = *(loff_t *)v;

	if (!bam) {
		if (!i) {
			seq_printf(seq, "BAM: off\n");
		}
		return 0;
	}
	if (!i) {
		seq_printf(seq, "BAM:memchk: %s", (bam_mode)?"enabled":"disabled");
		if (bam_noirq) {
			seq_printf(seq, ",noirq");
		}
		seq_printf(seq, "\n");
	} else {
		seq_printf(seq, "    %d: %lu,0x%08lx-0x%08lx,0x%08lx:%08lx\n",
			   i-1, bams[i-1].target,
			   bams[i-1].start, bams[i-1].end,
			   bams[i-1].mask2, bams[i-1].mask);
	}
	return 0;
}

static struct seq_operations bam_seq_ops = {
	.start	= bam_seq_start,
	.next	= bam_seq_next,
	.stop	= bam_seq_stop,
	.show	= bam_seq_show,
};

static int bam_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &bam_seq_ops);
}

static struct file_operations proc_bam_fops = {
	.owner		= THIS_MODULE,
	.open		= bam_seq_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
	.write		= bam_write,
};

static struct proc_dir_entry *proc_bam = NULL;

static int __init bam_init(void)
{
	proc_bam = create_proc_entry(BAM_PROCNAME, 0, NULL);
	if (!proc_bam) {
		printk(KERN_ERR "ERROR:BAM:can not create proc entry\n");
		return -1;
	}
	proc_bam->proc_fops = &proc_bam_fops;
	return platform_driver_register(&bam_driver);
}

static void __exit bam_cleanup(void)
{
	platform_driver_unregister(&bam_driver);
	remove_proc_entry(BAM_PROCNAME, NULL);
}

module_init(bam_init);
module_exit(bam_cleanup);
