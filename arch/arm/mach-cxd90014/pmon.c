/*
 * arch/arm/mach-cxd90014/pmon.c
 *
 * Performance Monitor driver
 *
 * Copyright 2010,2011,2012 Sony Corporation
 *
 * This code is based on arch/arm/oprofile/op_model_mpcore.c
 */
/**
 * @file op_model_mpcore.c
 * MPCORE Event Monitor Driver
 * @remark Copyright 2004 ARM SMP Development Team
 * @remark Copyright 2000-2004 Deepak Saxena <dsaxena@mvista.com>
 * @remark Copyright 2000-2004 MontaVista Software Inc
 * @remark Copyright 2004 Dave Jiang <dave.jiang@intel.com>
 * @remark Copyright 2004 Intel Corporation
 * @remark Copyright 2004 Zwane Mwaikambo <zwane@arm.linux.org.uk>
 * @remark Copyright 2004 Oprofile Authors
 *
 * @remark Read the file COPYING
 *
 * @author Zwane Mwaikambo
 *
 *  Counters:
 *    0: PMN0 on CPU0, per-cpu configurable event counter
 *    1: PMN1 on CPU0, per-cpu configurable event counter
 *    2: CCNT on CPU0
 *    3: PMN0 on CPU1
 *    4: PMN1 on CPU1
 *    5: CCNT on CPU1
 *    6: PMN0 on CPU1
 *    7: PMN1 on CPU1
 *    8: CCNT on CPU1
 *    9: PMN0 on CPU1
 *   10: PMN1 on CPU1
 *   11: CCNT on CPU1
 *   12-19: configurable SCU event counters
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <asm/hardware/pmc.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/dap_export.h>

/* L2 */
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#define L2X0_EVENT_CTRL_EN	0x1
#define L2X0_EVENT_RESET_EV0	0x2
#define L2X0_EVENT_RESET_EV1	0x4
#define L2X0_EVENT_CTRL_START	(L2X0_EVENT_CTRL_EN|L2X0_EVENT_RESET_EV0|L2X0_EVENT_RESET_EV1)
#define L2X0_EVENT_CFG_MASK	0xf
#define L2X0_EVENT_CFG_SHIFT	2
#define L2X0_EVENT_CFG_OVF	0x2
#define L2X0_INTR_ALL		0x1f
#define L2X0_INTR_ECNTR		0x01
#endif /* CONFIG_CACHE_L2X0 */

#define PMON_PROCNAME "pmon"

static int pmon_enable = 1;
module_param_named(en, pmon_enable, bool, S_IRUSR);

/* cp15 */
static int pmon_pmn0 = -1;
module_param_named(pmn0, pmon_pmn0, int, S_IRUSR|S_IWUSR);
static int pmon_pmn1 = -1;
module_param_named(pmn1, pmon_pmn1, int, S_IRUSR|S_IWUSR);
static int pmon_ccnt = -1;
module_param_named(ccnt, pmon_ccnt, int, S_IRUSR|S_IWUSR);

struct counter {
	uint ovf; /* overflow count */
	u32 rest; /* last count */
};

/*------------------------- L2 --------------------------*/
#ifdef CONFIG_CACHE_L2X0

# define PMON_NR_SEQ_L2 (1 + 1)

static int pmon_l2ev0 = 0;
module_param_named(l2ev0, pmon_l2ev0, int, S_IRUSR|S_IWUSR);
static int pmon_l2ev1 = 0;
module_param_named(l2ev1, pmon_l2ev1, int, S_IRUSR|S_IWUSR);

#define N_L2_COUNTER	2
static struct counter l2_data[N_L2_COUNTER];

static void pmon_start_l2(void)
{
	writel_relaxed(L2X0_INTR_ALL, VA_PL310+L2X0_INTR_MASK);
	writel_relaxed(L2X0_EVENT_CTRL_START, VA_PL310+L2X0_EVENT_CNT_CTRL);
}

static void pmon_stop_l2(void)
{
	/* stop */
	writel_relaxed(0, VA_PL310+L2X0_EVENT_CNT_CTRL);
	writel_relaxed(0, VA_PL310+L2X0_INTR_MASK);

	/* read counters */
	l2_data[0].rest = readl_relaxed(VA_PL310+L2X0_EVENT_CNT0_VAL);
	l2_data[1].rest = readl_relaxed(VA_PL310+L2X0_EVENT_CNT1_VAL);
}

static irqreturn_t pmon_l2intr(int irq, void *dev_id)
{
	u32 stat, cfg, cnt;

	if (irq != IRQ_L2) {
		printk(KERN_ERR "pmon_l2intr: illegal irq=%d\n", irq);
		return IRQ_HANDLED;
	}
	stat = readl_relaxed(VA_PL310+L2X0_MASKED_INTR_STAT);
	if (stat & L2X0_INTR_ECNTR) {
		cfg = L2X0_EVENT_CTRL_EN;
		cnt = readl_relaxed(VA_PL310+L2X0_EVENT_CNT0_VAL);
		if (~0 == cnt) { /* overflow */
			l2_data[0].ovf++;
			cfg |= L2X0_EVENT_RESET_EV0;
		}
		cnt = readl_relaxed(VA_PL310+L2X0_EVENT_CNT1_VAL);
		if (~0 == cnt) { /* overflow */
			l2_data[1].ovf++;
			cfg |= L2X0_EVENT_RESET_EV1;
		}
		writel_relaxed(cfg, VA_PL310+L2X0_EVENT_CNT_CTRL);
	}
	writel_relaxed(stat, VA_PL310+L2X0_INTR_CLEAR);
	wmb();
	if (stat & (L2X0_INTR_ALL & ~L2X0_INTR_ECNTR)) {
		printk(KERN_ERR "pmon_l2intr: stat=0x%02x\n", stat);
	}
	return IRQ_HANDLED;
}

static void pmon_setup_l2(void)
{
	u32 cfg;

	memset(l2_data, 0, sizeof l2_data);
	cfg = 0;
	if (pmon_l2ev0) {
		cfg = (pmon_l2ev0 & L2X0_EVENT_CFG_MASK)<<L2X0_EVENT_CFG_SHIFT;
		cfg |= L2X0_EVENT_CFG_OVF;
	}
	writel_relaxed(cfg, VA_PL310+L2X0_EVENT_CNT0_CFG);
	cfg = 0;
	if (pmon_l2ev1) {
		cfg = (pmon_l2ev1 & L2X0_EVENT_CFG_MASK)<<L2X0_EVENT_CFG_SHIFT;
		cfg |= L2X0_EVENT_CFG_OVF;
	}
	writel_relaxed(cfg, VA_PL310+L2X0_EVENT_CNT1_CFG);
}

static int pmon_setup_irq_l2(void)
{
	int ret;

	ret = request_irq(IRQ_L2, pmon_l2intr, IRQF_DISABLED, "L2", NULL);
	if (ret) {
		printk(KERN_ERR "pmon: request_irq %d failed: %d\n", IRQ_L2, ret);
	}
	return ret;
}

static void pmon_show_l2(struct seq_file *seq, int i)
{
	int n;
	u64 count;

	if (0 == i) { /* header */
		seq_printf(seq, "                     0x%x                 0x%x", pmon_l2ev0, pmon_l2ev1);
	} else {
		seq_printf(seq, "L2  :");
		for (n = 0; n < N_L2_COUNTER; n++) {
			count = (u64)l2_data[n].ovf << 32;
			count += l2_data[n].rest;
			seq_printf(seq, " %20llu", count);
		}
	}
}
#else /* CONFIG_CACHE_L2X0 */
# define PMON_NR_SEQ_L2 0
static int  pmon_setup_irq_l2(void) { return 0; }
static void pmon_setup_l2(void) {}
static void pmon_start_l2(void) {}
static void pmon_stop_l2(void) {}
static void pmon_show_l2(struct seq_file *seq, int i) {}
#endif /* CONFIG_CACHE_L2X0 */

/*------------------------ cp15 -------------------------*/
static u32 pmon_pmnc    = 0;
static u32 pmon_pmnintr = 0;
static u32 pmon_pmnen   = 0;
#define N_PMN_COUNTER	3
#define PMON_INFO_PMN0	0
#define PMON_INFO_PMN1	1
#define PMON_INFO_CCNT	2
struct pmon_info {
	struct counter cnt[N_PMN_COUNTER];
} ____cacheline_aligned_in_smp;
static struct pmon_info pmon_data[NR_CPUS] __cacheline_aligned_in_smp;

#define PMON_NR_SEQ_CP15 (1 + NR_CPUS)

static void pmon_start_pmu(void *info)
{
	wrpmc(PMSELR, PMSEL_PMN0);
	wrpmc(PMXEVTYPER, pmon_pmn0);

	wrpmc(PMSELR, PMSEL_PMN1);
	wrpmc(PMXEVTYPER, pmon_pmn1);

	wrpmc(PMOVSR, (PMNC_PMN0 | PMNC_PMN1 | PMNC_CCNT));
	wrpmc(PMINTENSET, pmon_pmnintr);
	wrpmc(PMCNTENSET, pmon_pmnen);

	wrpmc(PMCR, pmon_pmnc);
}

static void pmon_stop_pmu(void *info)
{
	int cpu = smp_processor_id();
	u32 val;

	/* stop */
	wrpmc(PMCR, 0);

	/* read counters */
	wrpmc(PMSELR, PMSEL_PMN0);
	val = rdpmc(PMXEVCNTR);
	pmon_data[cpu].cnt[PMON_INFO_PMN0].rest = val;

	wrpmc(PMSELR, PMSEL_PMN1);
	val = rdpmc(PMXEVCNTR);
	pmon_data[cpu].cnt[PMON_INFO_PMN1].rest = val;

	val = rdpmc(PMCCNTR);
	pmon_data[cpu].cnt[PMON_INFO_CCNT].rest = val;
}

static irqreturn_t pmon_intr(int irq, void *dev_id)
{
	int n;
	u32 pmnintr, pmnovf;

	n = irq - IRQ_PMU_BASE;
	cxd90014_cti_ack(n);
	if (n < 0  ||  n >= NR_CPUS) {
		printk(KERN_ERR "pmon_intr: illegal irq=%d\n", irq);
		return IRQ_HANDLED;
	}
	pmnovf  = rdpmc(PMOVSR);
	pmnintr = rdpmc(PMINTENSET);

	if ((pmnovf & PMNC_PMN0) && (pmnintr & PMNC_PMN0)) {
		wrpmc(PMSELR, PMSEL_PMN0);
		wrpmc(PMXEVCNTR, 0);
		pmon_data[n].cnt[PMON_INFO_PMN0].ovf++;
	}
	if ((pmnovf & PMNC_PMN1) && (pmnintr & PMNC_PMN1)) {
		wrpmc(PMSELR, PMSEL_PMN1);
		wrpmc(PMXEVCNTR, 0);
		pmon_data[n].cnt[PMON_INFO_PMN1].ovf++;
	}
	if ((pmnovf & PMNC_CCNT) && (pmnintr & PMNC_CCNT)) {
		wrpmc(PMCCNTR, 0);
		pmon_data[n].cnt[PMON_INFO_CCNT].ovf++;
	}
	wrpmc(PMOVSR, pmnovf); /* clear overflow flags */

	return IRQ_HANDLED;
}

static void pmon_route_irq(void)
{
	int i;

	for (i = 0; i < NR_CPUS; i++) {
		irq_set_affinity(IRQ_PMU(i), cpumask_of(i));
	}
}

static void pmon_setup_pmu(void)
{
	u32 pmnc = 0, pmnintr = 0, pmnen = 0;

	pmon_route_irq();
	memset(pmon_data, 0, sizeof pmon_data);
	if (pmon_pmn0 >= 0) {
		pmnc    |= PMNC_E;
		pmnintr |= PMNC_PMN0;
		pmnen   |= PMNC_PMN0;
	}
	if (pmon_pmn1 >= 0) {
		pmnc    |= PMNC_E;
		pmnintr |= PMNC_PMN1;
		pmnen   |= PMNC_PMN1;
	}
	if (pmon_ccnt >= 0) {
		pmnc    |= PMNC_E;
		pmnintr |= PMNC_CCNT;
		pmnen   |= PMNC_CCNT;
	}
	pmon_pmnen   = pmnen;
	pmon_pmnintr = pmnintr;
	pmon_pmnc    = pmnc | PMNC_PMNX_CLR | PMNC_CCNT_CLR;
}

static void pmon_show_cp15(struct seq_file *seq, int i)
{
	int cpu, n;
	u64 count;

	if (0 == i) { /* header */
		seq_printf(seq, "                    ");
		if (pmon_pmn0 >= 0)
			seq_printf(seq, "0x%02x", pmon_pmn0);
		else
			seq_printf(seq, "----");
		seq_printf(seq, "                ");
		if (pmon_pmn1 >= 0)
			seq_printf(seq, "0x%02x", pmon_pmn1);
		else
			seq_printf(seq, "----");
		seq_printf(seq, "                CCNT");
	} else {
		cpu = i - 1;
		seq_printf(seq, "CPU%d:", cpu);
		for (n = 0; n < N_PMN_COUNTER; n++) {
			count = (u64)pmon_data[cpu].cnt[n].ovf << 32;
			count += pmon_data[cpu].cnt[n].rest;
			seq_printf(seq, " %20llu", count);
		}
	}
}
/*-------------------------------------------------------*/

static void pmon_start(void)
{
	/* setup */
	pmon_setup_pmu();
	cxd90014_cti_setup();
	pmon_setup_l2();

	/* start */
	wmb();
	on_each_cpu(pmon_start_pmu, NULL, 1);
	pmon_start_l2();
}

static void pmon_stop(void)
{
	pmon_stop_l2();
	on_each_cpu(pmon_stop_pmu, NULL, 1);
}

static int pmon_setup_irq(void)
{
	int i, ret;

	/* cp15 */
	for (i = 0; i < NR_CPUS; i++) {
		ret = request_irq(IRQ_PMU(i), pmon_intr, IRQF_DISABLED,
				  "pmn", NULL);
		if (ret) {
			printk(KERN_ERR "pmon: request_irq %d failed: %d\n",
			       IRQ_PMU(i), ret);
			goto err;
		}
	}
	/* L2 */
	ret = pmon_setup_irq_l2();
	if (ret)
		goto err;

	return 0;

 err:
	while (i--) {
		free_irq(IRQ_PMU(i), NULL);
	}
	return ret;
}

/*----------------------------------------------------------------*/
static int pmon_write(struct file *file, const char __user *buf,
		      size_t len, loff_t *ppos)
{
	char arg[16];
	int n, start;

	if (len > sizeof arg - 1) {
		printk(KERN_ERR "/proc/%s: too long.\n", PMON_PROCNAME);
		return -1;
	}
	if (copy_from_user(arg, buf, len)) {
		return -EFAULT;
	}
	arg[len] = '\0';

	n = sscanf(arg, "%d", &start);
	if (n != 1) {
		printk(KERN_ERR "/proc/%s: invalid format.\n", PMON_PROCNAME);
		return -1;
	}
	if (start) {
		pmon_start();
	} else {
		pmon_stop();
	}

	return len;
}

#define PMON_NR_SEQ	(PMON_NR_SEQ_CP15 + PMON_NR_SEQ_L2)

static void *pmon_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos < PMON_NR_SEQ)
		return pos;
	return NULL;
}

static void *pmon_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos < PMON_NR_SEQ)
		return pos;
	return NULL;
}

static void pmon_seq_stop(struct seq_file *seq, void *v)
{
}

static int pmon_seq_show(struct seq_file *seq, void *v)
{
	int i = *(loff_t *)v;

	if (i < PMON_NR_SEQ_CP15) {
		pmon_show_cp15(seq, i);
	} else {
		pmon_show_l2(seq, i - PMON_NR_SEQ_CP15);
	}
	seq_printf(seq, "\n");
	return 0;
}

static struct seq_operations pmon_seq_ops = {
	.start	= pmon_seq_start,
	.next	= pmon_seq_next,
	.stop	= pmon_seq_stop,
	.show	= pmon_seq_show,
};

static int pmon_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &pmon_seq_ops);
}

static struct file_operations proc_pmon_ops = {
	.owner		= THIS_MODULE,
	.open		= pmon_seq_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
	.write		= pmon_write,
};

static struct proc_dir_entry *proc_pmon = NULL;

static int __init pmon_init(void)
{
	if (!pmon_enable)
		return 0;

	proc_pmon = create_proc_entry(PMON_PROCNAME, 0, NULL);
	if (!proc_pmon) {
		printk(KERN_ERR "pmon: can not create proc entry\n");
		return -1;
	}
	proc_pmon->proc_fops = &proc_pmon_ops;

	if (pmon_setup_irq()) {
		remove_proc_entry(PMON_PROCNAME, NULL);
		return -1;
	}

	return 0;
}

module_init(pmon_init);
