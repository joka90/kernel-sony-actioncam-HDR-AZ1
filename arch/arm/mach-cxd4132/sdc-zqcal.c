/*
 * arch/arm/mach-cxd4132/sdc-zqcal.c
 *
 * SDC ZQ calibration driver
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
#include <linux/udif/macros.h>
#ifdef CONFIG_SNSC_BOOT_TIME
#include <linux/snsc_boot_time.h>
#endif
#include <mach/regs-dmc.h>
#include "sdc.h"
#include "sdc-zqcal-api.h"

#define DEFAULT_PERIOD  100 /* ms */

static UDIF_UINT zqcal_period = DEFAULT_PERIOD;
module_param_named(period, zqcal_period, uint, S_IRUSR|S_IWUSR);

/* mode */
#define ZQCAL_LONG	1
#define ZQCAL_SHORT	2
static void zqcal_exec(struct zqcal_ctl *zq, UDIF_INT mode)
{
	struct sdc_dev_t *dev = container_of(zq, struct sdc_dev_t, zqcal);
	UDIF_U32 cmd, op, tMRW;

	switch (mode) {
	case ZQCAL_LONG:
		op = LPDDR2_ZQLONG;
		tMRW = SDC_ZQLONG_PERIOD;
		break;
	case ZQCAL_SHORT:
		op = LPDDR2_ZQSHORT;
		tMRW = SDC_ZQSHORT_PERIOD;
		break;
	default:
		return;
	}

	if (0 == zq->cur_rank) {
		cmd = SDC_DDR2_MRW_R0;  /* RANK 0 */
	} else {
		cmd = SDC_DDR2_MRW_R1;  /* RANK 1 */
	}

	/* execute */
	sdc_ddr2_mrw(dev, cmd, LPDDR2_ZQREG, op, tMRW);
}

static unsigned long zqcal_rel_jiffies(struct zqcal_ctl *zq)
{
	UDIF_ULONG t;
	UDIF_UINT period;

	if (zq->period != zqcal_period) { /* value changed */
		period = zqcal_period;
		if (zq->n_rank > 1) {
			period /= zq->n_rank;
		}

		if ((t = msecs_to_jiffies(period)) == 0) {
			t = 1;
		}
		/* cache */
		zq->period = zqcal_period;
		zq->ticks = t;
	}
	return zq->ticks;
}

static void zqcal_timer_handler(unsigned long data)
{
	struct zqcal_ctl *zq = (struct zqcal_ctl *)data;
	struct zqcal_rank *rankp;
	UDIF_U32 t2, dt;

	if (!zq->enable) {
		return;
	}
	if (0 == zqcal_period) {
		return;
	}
	/* current RANK */
	rankp = &zq->ranks[zq->cur_rank];
	/* statistics */
	t2 = mach_read_cycles();
	if (rankp->flags & ZQCAL_STARTUP) {
		rankp->flags &= ~ZQCAL_STARTUP;
	} else {
		dt = t2 - rankp->t1;
		if (dt < zq->t_min) {
			zq->t_min = dt;
		}
		if (dt > zq->t_max) {
			zq->t_max = dt;
		}
	}
	rankp->t1 = t2; /* save */
	zq->t_last = jiffies;

	/* Do ZQ calibration */
	if (rankp->flags & ZQCAL_NEEDLONG) {
		rankp->flags &= ~ZQCAL_NEEDLONG; /* one shot */
		zqcal_exec(zq, ZQCAL_LONG);
#ifdef CONFIG_SNSC_BOOT_TIME
		BOOT_TIME_ADD();
#endif
	} else {
		zqcal_exec(zq, ZQCAL_SHORT);
	}

	/* goto next RANK */
	if (zq->n_rank > 1) {
		if (++zq->cur_rank >= zq->n_rank) {
			zq->cur_rank = 0;
		}
	}

	mod_timer(&zq->timer, jiffies + zqcal_rel_jiffies(zq));
}

static void zqcal_start(struct zqcal_ctl *zq)
{
	struct timer_list *timer = &zq->timer;
	int i;

	zq->enable = 1;
#ifdef CONFIG_CXD4132_DDR_RANK0_ABSENT
	zq->cur_rank = 1;
#else
	zq->cur_rank = 0;
#endif
	for (i = 0; i < SDC_MAX_RANK; i++) {
		zq->ranks[i].flags |= ZQCAL_STARTUP;
	}
	init_timer(timer);
	timer->function = zqcal_timer_handler;
	timer->data = (unsigned long)zq;
	timer->expires = jiffies + 1;
	add_timer(timer);
}

static void zqcal_finish(struct zqcal_ctl *zq)
{
	zq->enable = 0;
	del_timer_sync(&zq->timer);
}

/*----------------- API -----------------*/
#ifdef CONFIG_PM
void sdc_zqcal_suspend(struct sdc_dev_t *dev)
{
	struct zqcal_ctl *zq = &dev->zqcal;

	zqcal_finish(zq);
}

void sdc_zqcal_resume(struct sdc_dev_t *dev)
{
	struct zqcal_ctl *zq = &dev->zqcal;

	zqcal_start(zq);
}
#endif /* CONFIG_PM */

#ifdef CONFIG_PROC_FS
UDIF_SIZE sdc_zqcal_proc_read(UDIF_PROC_READ *proc, UDIF_SIZE off,
			      struct sdc_dev_t *dev)
{
	struct zqcal_ctl *zq = &dev->zqcal;
	UDIF_SIZE cnt = 0;

	cnt += udif_proc_setbuf(proc, off+cnt, "  min:%7d[us]\n",
				mach_cycles_to_usecs(zq->t_min));
	cnt += udif_proc_setbuf(proc, off+cnt, "  max:%7d[us]\n",
				mach_cycles_to_usecs(zq->t_max));
	cnt += udif_proc_setbuf(proc, off+cnt, "  cur_rank:%d\n",
				zq->cur_rank);
	cnt += udif_proc_setbuf(proc, off+cnt, "  last:%lu\n",
				zq->t_last);
	return cnt;
}
#endif /* CONFIG_PROC_FS */

void sdc_zqcal_register(struct sdc_dev_t *dev)
{
	struct zqcal_ctl *zq = &dev->zqcal;
	int i;

	if (!ZQCAL_REQUIRED(dev)) {
		return;
	}
	/* initialize */
	zq->enable = 0;
	zq->n_rank = (dev->is_2rank) ? 2 : 1;
	for (i = 0; i < SDC_MAX_RANK; i++) {
		zq->ranks[i].flags = ZQCAL_NEEDLONG; /* ZQLONG at 1st time */;
	}
	zq->t_min = (UDIF_U32)(-1); /* UINT32 MAX */
	zq->t_max = 0;
	zq->period = 0;
	if (zqcal_period > 0) {
		zqcal_start(zq);
	}
}

void sdc_zqcal_unregister(struct sdc_dev_t *dev)
{
	struct zqcal_ctl *zq = &dev->zqcal;

	if (!ZQCAL_REQUIRED(dev)) {
		return;
	}
	/* stop */
	zqcal_finish(zq);
}

void sdc_zqcal_init(void)
{
}
