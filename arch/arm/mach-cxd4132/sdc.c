/*
 * arch/arm/mach-cxd4132/sdc.c
 *
 * SDC driver
 *
 * Copyright 2010 Sony Corporation
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

#include <linux/udif/module.h>
#include <linux/udif/proc.h>
#include <linux/udif/mutex.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/print.h>
#include <linux/udif/macros.h>
#include <mach/hardware.h>
#include <mach/regs-dmc.h>
#include "sdc.h"
#include "sdc-zqcal-api.h"

#define DRV_VERSION	"1.0"
#define DRV_NAME	"sdc"
#define PROC_NAME	DRV_NAME
#define MAX_SDC_DEV	UDIF_NR_SDC
#define TIMEOUT		100 /*ms*/

/* SDC device allocator */
static struct sdc_dev_t sdc_devs[MAX_SDC_DEV];
static UDIF_MUTEX sdc_devs_sem;

static struct sdc_dev_t *sdc_get_device(void)
{
	UDIF_INT i;
	struct sdc_dev_t *dev;

	udif_mutex_lock(&sdc_devs_sem);
	/* find free slot */
	for (i = 0, dev = sdc_devs; i < MAX_SDC_DEV; i++, dev++) {
		if (SDC_DEV_FREE == dev->magic) { /* found */
			break;
		}
	}
	if (MAX_SDC_DEV == i) {
		udif_printk(KERN_ERR DRV_NAME ":number of device exceed\n");
		dev = NULL;
		goto out;
	}
	dev->magic = SDC_DEV_MAGIC; /* allocate */

 out:
	udif_mutex_unlock(&sdc_devs_sem);
	return dev;
}

static struct sdc_dev_t *sdc_find_device(const UDIF_DEVICE *udev)
{
	UDIF_INT i;
	struct sdc_dev_t *dev;

	udif_mutex_lock(&sdc_devs_sem);
	for (i = 0, dev = sdc_devs; i < MAX_SDC_DEV; i++, dev++) {
		if (SDC_DEV_MAGIC == dev->magic
		    && dev->udev == udev) { /* match */
			break;
		}
	}
	if (MAX_SDC_DEV == i) {
		dev = NULL;
	}
	udif_mutex_unlock(&sdc_devs_sem);
	return dev;
}

static void sdc_put_device(struct sdc_dev_t *dev)
{
	udif_mutex_lock(&sdc_devs_sem);
	dev->magic = SDC_DEV_FREE; /* free */
	udif_mutex_unlock(&sdc_devs_sem);
}

/*--- Low level I/O ---*/

static UDIF_U32 sdc_read(struct sdc_dev_t *dev, UDIF_U32 off)
{
	return udif_ioread32(dev->vaddr + off);
}

static void sdc_write(struct sdc_dev_t *dev, UDIF_U32 off, UDIF_U32 dat)
{
	udif_iowrite32(dat, dev->vaddr + off);
}

/* sdc_lock must be held */
static UDIF_U32 sdc_rmw(struct sdc_dev_t *dev, UDIF_U32 off,
			UDIF_U32 mask, UDIF_U32 shift, UDIF_U32 val)
{
	UDIF_U32 dat, new;

	dat = sdc_read(dev, off);
	new = dat & ~(mask << shift);  /* clear */
	new |= (val & mask) << shift;  /* set */
	sdc_write(dev, off, new);
	return dat;
}

/*--- utility functions ---*/
static int sdc_is_ddr2(struct sdc_dev_t *dev)
{
	UDIF_U32 dat;

	dat = sdc_read(dev, SDC_MEMORY);
	return (dat & SDC_MODE_DDR);
}

static int sdc_is_2rank(struct sdc_dev_t *dev)
{
	UDIF_U32 dat;

#ifdef CONFIG_CXD4132_DDR_RANK0_ABSENT
	return 0;
#else
	dat = sdc_read(dev, SDC_MEMORY);
	return (dat & SDC_MODE2RANK);
#endif
}

static UDIF_INT sdc_ddr2_wait(struct sdc_dev_t *dev)
{
	UDIF_U32 dat;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(TIMEOUT);
	while ((dat = sdc_read(dev, SDC_COMMAND2)) & SDC_DDR2_BUSY) {
		if (time_after(jiffies, timeout)) { /* timeout */
			dev->errors |= SDC_ERR_TIMEOUT;
			return -1;
		}
	}
	return 0;
}

void sdc_ddr2_mrw(struct sdc_dev_t *dev, UDIF_U32 cmd, UDIF_U32 ma,
		  UDIF_U32 op, UDIF_U32 tMRW)
{
	UDIF_U32 dat;

	udif_spin_lock(&dev->sdc_lock);

	if (sdc_ddr2_wait(dev) < 0) { /* timeout */
		goto out;
	}

	/* set tMRW */
	sdc_rmw(dev, SDC_PARA2, SDC_TMRW_MASK, SDC_TMRW_SHIFT, tMRW);
	/* issue */
	dat = cmd | (ma << SDC_DDR2_MA_SHIFT) | op;
	sdc_write(dev, SDC_COMMAND2, dat);
 out:
	udif_spin_unlock(&dev->sdc_lock);
}


#ifdef CONFIG_PROC_FS
static UDIF_SSIZE sdc_proc_read(UDIF_PROC_READ *proc)
{
	UDIF_INT i;
	struct sdc_dev_t *dev;
	UDIF_SSIZE cnt = 0;

	udif_mutex_lock(&sdc_devs_sem);
	for (i = 0, dev = sdc_devs; i < MAX_SDC_DEV; i++, dev++) {
		if (SDC_DEV_MAGIC != dev->magic)
			continue;
		cnt += udif_proc_setbuf(proc, cnt, "%d: %s %drank 0x%08x\n",
					i,
					dev->is_ddr2 ? "DDR2":"DDR1",
					dev->is_2rank ? 2:1,
					dev->errors);
		/* ZQ calibration */
		cnt += sdc_zqcal_proc_read(proc, cnt, dev);
	}
	udif_mutex_unlock(&sdc_devs_sem);
	return cnt;
}

static UDIF_PROC sdc_proc = {
	.name	= PROC_NAME,
	.read	= sdc_proc_read,
};
#endif /* CONFIG_PROC_FS */

#ifdef CONFIG_PM
static UDIF_ERR sdc_suspend(const UDIF_DEVICE *udev, UDIF_CH ch, UDIF_VP data)
{
	struct sdc_dev_t *dev;

	if ((dev = sdc_find_device(udev)) == NULL) {
		udif_printk(KERN_ERR DRV_NAME ":ERROR:sdc_suspend:NO EXIST\n");
		return UDIF_ERR_DEV;
	}
	/* ZQ calibration */
	sdc_zqcal_suspend(dev);
	return UDIF_ERR_OK;
}

static UDIF_ERR sdc_resume(const UDIF_DEVICE *udev, UDIF_CH ch, UDIF_VP data)
{
	struct sdc_dev_t *dev;

	if ((dev = sdc_find_device(udev)) == NULL) {
		udif_printk(KERN_ERR DRV_NAME ":ERROR:sdc_resume:NO EXIST\n");
		return UDIF_ERR_DEV;
	}
	/* ZQ calibration */
	sdc_zqcal_resume(dev);
	return UDIF_ERR_OK;
}
#endif /* CONFIG_PM */

static UDIF_ERR UDIF_INIT sdc_probe(const UDIF_DEVICE *udev, UDIF_CH ch,
				    UDIF_VP data)
{
	struct sdc_dev_t *dev;

	if ((dev = sdc_get_device()) == NULL) {
		return UDIF_ERR_DEV;
	}
	udif_spin_lock_init(&dev->sdc_lock);
	dev->udev = udev;
	dev->vaddr = udif_devio_virt(udev, ch);
	dev->errors = SDC_ERR_NONE;
	/* read config of SDC */
	dev->is_2rank = sdc_is_2rank(dev);
	dev->is_ddr2 = sdc_is_ddr2(dev);
	/* ZQ calibration */
	sdc_zqcal_register(dev);
	return UDIF_ERR_OK;
}

static UDIF_ERR UDIF_INIT sdc_remove(const UDIF_DEVICE *udev, UDIF_CH ch,
				     UDIF_VP data)
{
	struct sdc_dev_t *dev;

	if ((dev = sdc_find_device(udev)) == NULL) {
		udif_printk(KERN_ERR DRV_NAME ":ERROR:sdc_remove:NO EXIST\n");
		return UDIF_ERR_DEV;
	}
	/* ZQ calibration */
	sdc_zqcal_unregister(dev);
	sdc_put_device(dev);
	return UDIF_ERR_OK;
}

static UDIF_ERR UDIF_INIT sdc_init(UDIF_VP data)
{
#ifdef CONFIG_PROC_FS
	UDIF_ERR ret;
#endif

	udif_mutex_init(&sdc_devs_sem);
	sdc_zqcal_init();
#ifdef CONFIG_PROC_FS
	if ((ret = udif_create_proc(&sdc_proc)) != UDIF_ERR_OK) {
		udif_printk(KERN_ERR DRV_NAME ":WARNING: can not create proc. ret=%d\n", ret);
	}
#endif
	return UDIF_ERR_OK;
}

static UDIF_ERR sdc_exit(UDIF_VP data)
{
#ifdef CONFIG_PROC_FS
	udif_remove_proc(&sdc_proc);
#endif
	return UDIF_ERR_OK;
}

static UDIF_DRIVER_OPS sdc_ops = {
	.init		= sdc_init,
	.exit		= sdc_exit,
	.probe		= sdc_probe,
	.remove		= sdc_remove,
#ifdef CONFIG_PM
	.suspend	= sdc_suspend,
	.resume		= sdc_resume,
#endif /* CONFIG_PM */
};

UDIF_IDS(sdc_ids) = {
	UDIF_ID(UDIF_ID_SDC, UDIF_CH_MASK_DEFAULT),
};
UDIF_DEPS(sdc_deps) = {};

UDIF_MODULE(sdc, DRV_NAME, DRV_VERSION, sdc_ops, sdc_ids, sdc_deps, NULL);
