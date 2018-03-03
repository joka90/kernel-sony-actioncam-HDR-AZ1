/*
 * arch/arm/mach-cxd90014/pm.c
 *
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
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/bootmem.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/page.h>
#include <asm/mach/arch.h>
#include <mach/system.h>
#include <asm/mach/time.h>
#include <mach/time.h>
#include <mach/pm.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/irq.h>
#include <linux/delay.h>
#include <mach/regs-timer.h>
#include <mach/hardware.h>
#include <mach/memory.h>
#include <linux/em_export.h>

#include <mach/regs-gpio.h>
#include <mach/platform.h>
#include <mach/cxd90014_cfg.h>

#ifdef CONFIG_SNSC_BOOT_TIME
#include <linux/snsc_boot_time.h>
#endif
#ifdef CONFIG_SNSC_SSBOOT
#include <linux/ssboot.h>
#endif

#include <asm/mach/warmboot.h>
#include <asm/cacheflush.h>

static int dc = 0;
module_param_named(dc, dc, int, S_IRUSR|S_IWUSR);
static int logbuf_noclear = 0;
module_param_named(logbuf_noclear, logbuf_noclear, int, S_IRUSR|S_IWUSR);

#ifndef CONFIG_CXD90014_SIMPLE_SUSPEND
extern unsigned long cxd90014_sus_end_msg;   /* sleep.S */
module_param_named(sus_msg, cxd90014_sus_end_msg, ulong, S_IRUSR|S_IWUSR);

/*
 * partial refresh ddr
 */
extern unsigned long cxd90014_ddr_pasr0;   /* sleep.S */
module_param_named(pasr0, cxd90014_ddr_pasr0, ulongH, S_IRUSR|S_IWUSR);
extern unsigned long cxd90014_ddr_pasr1;   /* sleep.S */
module_param_named(pasr1, cxd90014_ddr_pasr1, ulongH, S_IRUSR|S_IWUSR);

/*
 * resume vector
 */
static unsigned long cxd90014_resume_vector = 0xffffffffUL;
module_param_named(resume, cxd90014_resume_vector, ulongH, S_IRUSR);

/*
 * suspend error flag
 */
static unsigned long cxd90014_sus_error = 0xffffffffUL;
module_param_named(sus_error, cxd90014_sus_error, ulongH, S_IRUSR);

#endif /* !CONFIG_CXD41XX_SIMPLE_SUSPEND */

extern int cxd90014_cpu_suspend(int);
extern void cxd90014_cpu_resume(void);
#ifdef CONFIG_SNSC_SSBOOT
extern void cxd90014_cpu_resume_profile(void);
extern void cxd90014_cpu_resume_optimize(void);
extern void cxd90014_cpu_resume_ssboot_pre(void);
extern void cxd90014_cpu_resume_ssboot(void);
extern void ssboot_setmode(unsigned long);
extern int  cxd_rewrite_wbheader(const char*, unsigned long);

extern unsigned long cxd90014_ssboot_stat;
#endif

#ifdef CONFIG_OSAL_UDIF
extern int udif_driver_suspend(void);
extern int udif_driver_resume(void);
#else
# define udif_driver_suspend() (0)
# define udif_driver_resume() do {} while (0)
#endif /* CONFIG_OSAL_UDIF */

void cxd90014_pm_setup(void)
{
#ifndef CONFIG_CXD90014_SIMPLE_SUSPEND
	unsigned int ch, bit;
	extern unsigned long cxd90014_xpower_off_addr;    /* sleep.S */
	extern unsigned long cxd90014_xpower_off_bitmask; /* sleep.S */
	extern void cxd90014_cpu_resume(void);

	/* resume vector */
	if (0xffffffffUL == cxd90014_resume_vector) {
		printk("resume vector: not defined\n");
	} else {
		printk("resume vector: 0x%08lx\n", cxd90014_resume_vector);
		printk("resume entry:  0x%08lx\n", (unsigned long)virt_to_phys(cxd90014_cpu_resume));
		*(unsigned long *)phys_to_virt(cxd90014_resume_vector) = virt_to_phys(cxd90014_cpu_resume);
	}
	/* sus_error flag */
	if (0xffffffffUL == cxd90014_sus_error) {
		printk("sus_error address: not defined\n");
	} else {
		printk("sus_error address: 0x%08lx\n", cxd90014_sus_error);
		*(unsigned long *)phys_to_virt(cxd90014_sus_error) = 0UL;
	}

	/* XPOWER_OFF */
	if (0xffffffffUL == cxd90014_xpower_off) {
		cxd90014_xpower_off_addr = 0xffffffffUL; /* invalid mark */
	} else {
		ch   = cxd90014_xpower_off & 0xff;
		bit  = (cxd90014_xpower_off >> 8) & 0xff;
		cxd90014_xpower_off_addr = PA_GPIO(ch,WDATA)+GPIO_CLR;
		cxd90014_xpower_off_bitmask = 1 << bit;
	}

#endif /* CONFIG_CXD90014_SIMPLE_SUSPEND */
}

void __attribute__((weak)) cxd90014_early_resume(void)
{
	/* none */
}

static int _cxd90014_pm_enter(int mode)
{
	int err=0;
#ifdef CONFIG_SNSC_DEBUG_PROFILE
	extern void profile_soft_init(void);
#endif

#ifdef CONFIG_QEMU
	return 0;
#endif

	/* go zzz */
	if(cxd90014_cpu_suspend(mode)) {
		err = -1;
		printk("suspend error!\n");
	}

	cpu_init();

#ifdef CONFIG_WARM_BOOT_IMAGE
	if (!err && PM_SUSPEND_DISK == pm_get_state() && !pm_is_mem_alive()) {
		extern void clear_logbuf(void);
		if (!logbuf_noclear) {
			clear_logbuf();
		}
		printk("WARMBOOT\n");
#ifdef CONFIG_SNSC_SSBOOT
		ssboot_setmode(cxd90014_ssboot_stat);
#endif
	}
#endif /* CONFIG_WARM_BOOT_IMAGE */

	cxd90014_early_resume();

#ifdef CONFIG_SNSC_BOOT_TIME
	boot_time_takeover();
#endif
#ifdef CONFIG_SNSC_DEBUG_PROFILE
	profile_soft_init();
#endif
	cxd90014_timer_early_init();

	return err;
}

unsigned long sleep_phys_sp(void *sp)
{
	return virt_to_phys(sp);
}

static u8 reboot_reason;
static u16 reboot_timer;
static u16 state_timer;

static void cxd90014_pm_halt(int reboot)
{
	cxd90014_cpu_suspend((reboot)?PM_ENTER_RESET:PM_ENTER_ERROR);

	/* never reached */
}

static void pm_suspend_error(int mode)
{
	wbi_drop();
#ifndef CONFIG_CXD90014_SIMPLE_SUSPEND
	*(unsigned long *)phys_to_virt(cxd90014_sus_error) = 1UL;
#endif /* CONFIG_CXD90014_SIMPLE_SUSPEND */
	cxd90014_pm_halt(0);
	/* never reached */
}

void em_reboot(int force)
{
	flush_cache_all();
	outer_clean_all();
	pm_machine_reset(force);
}

/*
 * Called after processes are frozen, but before we shut down devices.
 */
static int cxd90014_pm_begin(suspend_state_t state)
{
	return warm_boot_pm_begin(state);
}

static int cxd90014_pm_prepare(void)
{
	if (unlikely(udif_driver_suspend()))
		pm_suspend_error(0);

	return warm_boot_pm_prepare();
}

static int cxd90014_pm_enter(suspend_state_t state)
{
	return _cxd90014_pm_enter(PM_ENTER_NORMAL);
}

/*
 * Called after devices are re-setup, but before processes are thawed.
 */
static void cxd90014_pm_finish(void)
{
	reboot_reason = 0;
	reboot_timer = 0;
	state_timer = 0;

	warm_boot_pm_finish();
	udif_driver_resume();

#ifdef CONFIG_SNSC_SSBOOT
	if (cxd90014_ssboot_stat == SSBOOT_SSBOOT_PRE) {
		cxd_rewrite_wbheader(WBI_VER, (unsigned long)cxd90014_cpu_resume_ssboot);
		cxd90014_ssboot_stat = SSBOOT_SSBOOT;
		printk("SSBI Create Completed!\n");
	}
#endif /* CONFIG_SNSC_SSBOOT */

	wbi_resume_drop();
}

static int cxd90014_valid(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM || state == PM_SUSPEND_DISK;
}

/*
 * Set to PM_DISK_FIRMWARE so we can quickly veto suspend-to-disk.
 */
static struct platform_suspend_ops cxd90014_pm_ops = {
	.valid		= cxd90014_valid,
	.begin		= cxd90014_pm_begin,
	.prepare	= cxd90014_pm_prepare,
	.enter		= cxd90014_pm_enter,
	.finish		= cxd90014_pm_finish,
};

extern struct kobject *power_kobj;

#define wb_attr(_name)							\
    static ssize_t _name##_show(struct kobject *s, struct kobj_attribute *attr, \
				char *buf)				\
    {									\
	return snprintf(buf, PAGE_SIZE, "%u\n", _name);			\
    }									\
    static ssize_t _name##_store(struct kobject *s, struct kobj_attribute *attr, \
				 const char * buf, size_t n)		\
    {									\
	char *endp;							\
	int value;							\
									\
	value = simple_strtoul(buf, &endp, 0);				\
	if(*endp)							\
	    return -EINVAL;						\
									\
	_name = value;							\
	return n;							\
    }									\
    static struct kobj_attribute _name##_attr = {			\
	.attr	= {							\
	    .name = __stringify(_name),					\
	    .mode = 0644,						\
	},								\
	.show	= _name##_show,						\
	.store	= _name##_store,					\
    }

wb_attr(reboot_reason);
wb_attr(reboot_timer);
wb_attr(state_timer);

static int __init cxd90014_pm_init(void)
{
	int error = 0;

	error = sysfs_create_file(power_kobj, &reboot_reason_attr.attr);
	if (error) {
		goto out;
	}
	error = sysfs_create_file(power_kobj, &reboot_timer_attr.attr);
	if (error) {
		goto out;
	}
	error = sysfs_create_file(power_kobj, &state_timer_attr.attr);
	if (error) {
		goto out;
	}

	suspend_set_ops(&cxd90014_pm_ops);
 out:
	return error;
}

late_initcall(cxd90014_pm_init);

unsigned int cxd90014_mem_alive = 0; /* set @ sleep.S */

/* check if memory data is kept during the last halt */
unsigned int pm_is_mem_alive(void)
{
        return cxd90014_mem_alive;
}
EXPORT_SYMBOL(pm_is_mem_alive);

#ifdef CONFIG_SNSC_SSBOOT
unsigned long get_ssboot_stat(void)
{
	return cxd90014_ssboot_stat;
}
EXPORT_SYMBOL(get_ssboot_stat);
#endif

#ifdef CONFIG_WARM_BOOT_IMAGE
int cxd90014_create_warmbootimage(void)
{
	cxd90014_mem_alive = 0;
#ifdef CONFIG_SNSC_SSBOOT
	switch(cxd90014_ssboot_stat) {
	case SSBOOT_CREATE_MINSSBI:
		/* Now Create Min SSBI Sequence (ColdBoot) */
		return cxd_create_warmbootimage((unsigned long)cxd90014_cpu_resume_profile, 0);
	case SSBOOT_PROFILE:
		/* Now Profile Sequence (SnapshotBoot with Min SSBI). No Create SSBI ! */
		return cxd_rewrite_wbheader(WBI_PROF_VER, (unsigned long)cxd90014_cpu_resume_optimize);
	case SSBOOT_CREATE_OPTSSBI:
		/* Now Create Opt SSBI Sequence (SnapshotBoot with Min SSBI) */
		cxd_create_warmbootimage((unsigned long)cxd90014_cpu_resume_ssboot_pre, 0);
		em_reboot(1);
		return 0; /* NOT REACHED */
	default:
		return cxd_create_warmbootimage((unsigned long)cxd90014_cpu_resume, 0);
	}
#else
	return cxd_create_warmbootimage((unsigned long)cxd90014_cpu_resume, 0);
#endif
}
#else
int cxd90014_create_warmbootimage(void)
{
	return 0;
}
#endif

