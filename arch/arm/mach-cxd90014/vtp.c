/*
 * arch/arm/mach-cxd90014/vtp.c
 *
 * virt to phys driver
 *
 * Copyright 2010,2011,2012 Sony Corporation
 *
 * This code is based on mm/memory.c
 */
/*
 *  linux/mm/memory.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define VTP_PROCNAME "vtp"
#define VTP_NOTMAPPED 0xffffffff

/* HUGETLB is not supported */
static int get_pte(struct mm_struct *mm, unsigned long address, unsigned long *ret)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;
	int found;

	found = 0;
	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || pgd_bad(*pgd)){
		found = -1;
		goto out;
	}
	pud = pud_offset(pgd, address);
	if (pud_none(*pud) || pud_bad(*pud)){
		found = -2;
		goto out;
	}
	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd) || pmd_bad(*pmd)){
		found = -3;
		goto out;
	}
	if ((pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_SECT) {
		found = 2;
		ret[0] = pmd_val(*pmd);
		goto out;
	}
	ptep = pte_offset_map(pmd, address);
	if (!ptep){
		found = -4;
		goto out;
	}
	pte = *ptep;
	pte_unmap(ptep);
	if (!pte_present(pte)){
		found = -5;
		goto out;
	}
	found = 1;
	ret[0] = (unsigned long)pte;
 out:
	return found;
}

static unsigned int pid;
static unsigned long virt;
static struct proc_dir_entry *proc_vtp = NULL;
static char proc_buf[10];

static void update(void)
{
	struct mm_struct *mm;
	int found;
	unsigned long ret[2] = {0}, pfn, phys;
	char *p;

	proc_vtp->size = 0;
	p = proc_buf;
	phys = VTP_NOTMAPPED;
	if (pid == 0)
		mm = &init_mm;
	else {
		struct task_struct *tsk;
		tsk = find_task_by_vpid(pid);
		if (!tsk)
			return;
		mm = tsk->mm;
		if (!mm)
			return;
	}
	down_read(&mm->mmap_sem);
	spin_lock(&mm->page_table_lock);
	found = get_pte(mm, virt, ret);
	spin_unlock(&mm->page_table_lock);
	up_read(&mm->mmap_sem);
	switch (found) {
	case 1: /* found page table */
		pfn = pte_pfn(ret[0]);
		phys = pfn << PAGE_SHIFT;
		break;
	case 2: /* found section mapping */
		phys = ret[0] & 0xfff00000;
		break;
	}
	p += snprintf(proc_buf, sizeof proc_buf, "%lx", phys);
	proc_vtp->size = p - proc_buf;
}

static ssize_t vtp_read(struct file *file, char __user *buf,
			size_t len, loff_t *ppos)
{
	loff_t pos = *ppos;

	if (pos == 0)
		update();
	if (pos < 0  ||  pos >= proc_vtp->size)
		return 0;
	if (pos + len > proc_vtp->size)
		len = proc_vtp->size - pos;
	if (copy_to_user(buf, proc_buf + pos, len))
		return -EFAULT;
	pos += len;
	*ppos = pos;
	return len;
}

static ssize_t vtp_write(struct file *file, const char __user *buf,
			 size_t len, loff_t *ppos)
{
	char arg[32];
	int n;

	if (len > sizeof arg - 1) {
		printk(KERN_ERR "/proc/%s: too long\n", VTP_PROCNAME);
		return -1;
	}
	if (copy_from_user(arg, buf, len)) {
		return -EFAULT;
	}
	arg[len] = '\0';

	n = sscanf(arg, "%u %lx", &pid, &virt);
	if (n != 2){
		printk(KERN_ERR "/proc/%s: invalid format\n", VTP_PROCNAME);
		return -1;
	}

	return len;
}

static struct file_operations proc_vtp_fops = {
	.owner		= THIS_MODULE,
	.read		= vtp_read,
	.write		= vtp_write,
};

static int __init vtp_init(void)
{
	proc_vtp = create_proc_entry(VTP_PROCNAME, 0, NULL);
	if (!proc_vtp) {
		printk(KERN_ERR "vtp: can not create proc entry\n");
		return -1;
	}
	proc_vtp->proc_fops = &proc_vtp_fops;
	pid = virt = 0;
	return 0;
}

module_init(vtp_init);
