/*
 * drivers/udif/driver.c
 *
 * UDM
 *
 * Copyright 2012,2013 Sony Corporation
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
#include <linux/udif/mutex.h>
#include <linux/udif/print.h>
#include <linux/udif/list.h>
#include <linux/udif/proc.h>
#include <linux/udif/boottime.h>
#include <linux/udif/macros.h>
#include <linux/ssboot.h>

#define UDM_SID 0x008E

struct udif_drivers_t {
	UDIF_LIST list;
	UDIF_MUTEX mtx;
	UDIF_DRIVER *mindrv_tail;
};

static struct udif_drivers_t drivers = {
	.list = UDIF_LIST_HEAD_INIT(drivers.list),
	.mtx  = UDIF_MUTEX_INIT(drivers.mtx),
	.mindrv_tail = NULL,
};

static inline void udif_driver_list_add(UDIF_DRIVER *drv, struct udif_drivers_t *drvs)
{
	udif_mutex_lock(&drvs->mtx);
	udif_list_add_tail(&drv->list, &drvs->list);
	udif_mutex_unlock(&drvs->mtx);
}

static inline void udif_driver_list_del(UDIF_DRIVER *drv, struct udif_drivers_t *drvs)
{
	udif_mutex_lock(&drvs->mtx);
	udif_list_del(&drv->list);
	udif_mutex_unlock(&drvs->mtx);
}

void udif_driver_add(UDIF_DRIVER *drv)
{
	udif_driver_list_add(drv, &drivers);
}

void udif_driver_del(UDIF_DRIVER *drv)
{
	udif_driver_list_del(drv, &drivers);
}

static UDIF_ERR udif_driver_call_fn(UDIF_DRIVER *drv, UDIF_MODULE_CB fn, UDIF_UINT *ndev)
{
	UDIF_ERR ret = UDIF_ERR_OK;
	UDIF_CH ch;

	if (!fn)
		goto end;

	if (!drv->nr_devs) {
		/* virtural driver which has no devices */
		ret = fn(0, 0, drv->data);
		goto end;
	}

	for (*ndev = 0; *ndev < drv->nr_devs; (*ndev)++) {
		const UDIF_DEVICE *dev = drv->devs[*ndev].dev;
		if (unlikely(!dev)) {
			UDIF_PERR("%s, %s no such num of device = %d\n", drv->name, dev->name, *ndev);
			ret = UDIF_ERR_NOENT;
			goto end;
		}
		for (ch = 0; ch < dev->nr_ch; ch++) {
			UDIF_CHANNELS *chs = &dev->chs[ch];

			if (unlikely(!chs)) {
				UDIF_PERR("%s, %s no such num of ch = %d\n", drv->name, dev->name, ch);
				ret = UDIF_ERR_NOENT;
				goto end;
			}

			if (!(drv->devs[*ndev].ch_mask & (1UL << ch)))
				continue;

			ret = fn(dev, ch, drv->data);
			if (unlikely(ret))
				goto end;
		}
	}

end:
	if (unlikely(ret))
		UDIF_PERR("%s failed ret = %d\n", drv->name, ret);

	return ret;
}

static UDIF_ERR udif_driver_call_fn_reverse(UDIF_DRIVER *drv, UDIF_MODULE_CB fn, UDIF_UINT ndev)
{
	UDIF_ERR ret = UDIF_ERR_OK;

	if (!fn)
		goto end;

	if (!drv->nr_devs) {
		/* virtural driver which has no devices */
		ret = fn(0, 0, drv->data);
		if (unlikely(ret))
			UDIF_PERR("%s error %d\n", drv->name, ret);
		goto end;
	}

	while (ndev--) {
		const UDIF_DEVICE *dev = drv->devs[ndev].dev;
		UDIF_CH ch = dev->nr_ch;
		while (ch--) {
			UDIF_CHANNELS *chs = &dev->chs[ch];

			if (unlikely(!chs)) {
				UDIF_PERR("%s, %s no such num of ch = %d\n", drv->name, dev->name, ch);
				ret = UDIF_ERR_NOENT;
				goto end;
			}

			if (!(drv->devs[ndev].ch_mask & (1UL << ch)))
				continue;

			ret = fn(dev, ch, drv->data);
			if (unlikely(ret))
				goto end;
		}
	}

end:
	if (unlikely(ret))
		UDIF_PERR("%s failed ret = %d\n", drv->name, ret);

	return ret;
}

static void __udif_driver_exit(UDIF_DRIVER *drv, UDIF_UINT nr_devs)
{
	UDIF_ERR ret;

	if (drv->ops) {
		ret = udif_driver_call_fn_reverse(drv, drv->ops->remove, nr_devs);
		if (unlikely(ret))
			return;

		if (drv->ops->exit) {
			ret = drv->ops->exit(drv->data);
			if (unlikely(ret)) {
				UDIF_PERR("%s exit error %d\n", drv->name, ret);
				return;
			}
		}
	}
}

static inline void udif_driver_exit(UDIF_DRIVER *drv, UDIF_UINT nr_devs)
{
	__udif_driver_exit(drv, nr_devs);
	udif_driver_list_del(drv, &drivers);
}

static UDIF_ERR __udif_driver_init(UDIF_DRIVER *drv)
{
	UDIF_ERR ret = UDIF_ERR_OK;
	UDIF_UINT success;;

	if (!drv->devs)
		drv->nr_devs = 0;

	if (drv->ops) {
		udif_timetag(UDM_SID, UDIF_TAGID_BEGIN, drv->name);
		if (drv->ops->init) {
			ret = drv->ops->init(drv->data);
			if (unlikely(ret)) {
				UDIF_PERR("%s init error %d\n", drv->name, ret);
				goto end;
			}
		}

		ret = udif_driver_call_fn(drv, drv->ops->probe, &success);
		if (unlikely(ret))
			__udif_driver_exit(drv, success);

		udif_timetag(UDM_SID, UDIF_TAGID_END, drv->name);
	}

end:
	return ret;
}

static inline UDIF_ERR udif_driver_init(UDIF_DRIVER *drv)
{
	UDIF_ERR ret;

	udif_driver_list_add(drv, &drivers);
	ret = __udif_driver_init(drv);
	if (unlikely(ret))
		udif_driver_list_del(drv, &drivers);

	return ret;
}

static int udif_driver_is_list_added(UDIF_DRIVER *drv)
{
	UDIF_DRIVER *ldrv;
	int ret = 0;

	udif_mutex_lock(&drivers.mtx);
	udif_list_for_each_entry(ldrv, &drivers.list, list) {
		if (ldrv == drv) {
			ret = 1;
			break;
		}
	}
	udif_mutex_unlock(&drivers.mtx);

	return ret;
}

void udif_driver_unregister(UDIF_DRIVER **drv, UDIF_U32 n)
{
	while (n--) {
		if (unlikely(!drv[n])) {
			UDIF_PERR("invalid driver n = %d\n", n);
			continue;
		}

		if (udif_driver_is_list_added(drv[n]))
			udif_driver_exit(drv[n], drv[n]->nr_devs);
	}
}

EXPORT_SYMBOL(udif_driver_unregister);

#ifdef CONFIG_OSAL_UDIF_THREAD
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/udif/atomic.h>

static UDIF_ERR udif_wait_depends(UDIF_DRIVER *drv)
{
	int i, ret;

	if (!drv->depends || !drv->depends[0])
		drv->nr_depends = 0;

	for (i = 0; i < drv->nr_depends; i++) {
		ret = wait_for_completion_timeout(&drv->depends[i]->com, 30*HZ); /* 30 sec */
		if (unlikely(!ret)) {
			UDIF_PERR("%s: timeout wait for %s init\n", drv->name, drv->depends[i]->name);
			return UDIF_ERR_TIMEOUT;
		}
	}

	return UDIF_ERR_OK;
}

static void udif_wakeup_depends(UDIF_DRIVER *drv)
{
	complete_all(&drv->com);
}

struct udif_thread_t {
	struct completion com;
	struct sched_param prio;
	UDIF_ATOMIC num;
	UDIF_ERR result;
};

static int udif_thread(void *data)
{
	UDIF_ERR ret;
	UDIF_DRIVER *drv = data;
	struct udif_thread_t *th = drv->th;

	daemonize("udm_%s", drv->name);

	ret = sched_setscheduler(current, SCHED_FIFO, &th->prio);
	if (unlikely(ret)) {
		UDIF_PERR("failed sched_setscheduler() ret = %d\n", ret);
		/* fall through */
	}

	ret = udif_wait_depends(drv);
	if (unlikely(ret)) {
		th->result = ret;
		goto end;
	}

	ret = udif_driver_init(drv);

	if (unlikely(ret)) {
		th->result = ret;
		/* not nesessary atomic becase of error seaquense */
	}

end:
	udif_wakeup_depends(drv);

	if (udif_atomic_dec_and_test(&th->num))
		complete(&th->com);

	return 0;
}

#define UDIF_REGISTER_PRIO (MAX_RT_PRIO - 70)

int udif_driver_register(UDIF_DRIVER **drv, UDIF_U32 n)
{
	UDIF_U32 i;
	int ret;
	struct udif_thread_t th = {
		.com = COMPLETION_INITIALIZER(th.com),
		.prio = { .sched_priority = UDIF_REGISTER_PRIO },
		.num = UDIF_ATOMIC_INIT(n),
		.result = UDIF_ERR_OK,
	};

	udif_timetag(UDM_SID, UDIF_TAGID_BEGIN, "unified register thread");

	for (i = 0; i < n; i++) {
		if (unlikely(!drv[i])) {
			UDIF_PERR("invalid driver i = %d\n", i);
			return -EINVAL;
		}

		drv[i]->th = &th;
		kernel_thread(udif_thread, drv[i], 0);
	}

	ret = wait_for_completion_timeout(&th.com, 5*60*HZ); /* 5 min */
	if (unlikely(!ret)) {
		UDIF_PERR("timeout: driver register\n");
		th.result = -ETIMEDOUT;
	}

	if (unlikely(th.result))
		udif_driver_unregister(drv, n);

	udif_timetag(UDM_SID, UDIF_TAGID_END, "unified register thread");

	return th.result;
}
#else /* CONFIG_OSAL_UDIF_THREAD */

int udif_driver_register(UDIF_DRIVER **drv, UDIF_U32 n)
{
	UDIF_U32 i;
	UDIF_ERR ret = UDIF_ERR_OK;

	udif_timetag(UDM_SID, UDIF_TAGID_BEGIN, "unified register");

	for (i = 0; i < n; i++) {
		if (unlikely(!drv[i])) {
			UDIF_PERR("invalid driver i = %d\n", i);
			return -EINVAL;
		}

		ret = udif_driver_init(drv[i]);
		if (unlikely(ret))
			break;
	}

	if (unlikely(ret))
		udif_driver_unregister(drv, i);

	udif_timetag(UDM_SID, UDIF_TAGID_END, "unified register");

	return ret;
}
#endif /* CONFIG_OSAL_UDIF_THREAD */

EXPORT_SYMBOL(udif_driver_register);

static void udif_print_both(unsigned short tagid, const char *prefix, const char *msg)
{
	UDIF_PINFO("%s%s\n", prefix, msg);
	udif_timetag(UDM_SID, tagid, msg);
}

int udif_driver_suspend(void)
{
	UDIF_DRIVER *drv;
	int ret = 0;
	int skip = 0;

	udif_print_both(UDIF_TAGID_BEGIN, "suspend start: ", "unified drivers");

	udif_mutex_lock(&drivers.mtx);
	if (ssboot_is_rewriting() && drivers.mindrv_tail)
		skip = 1;

	udif_list_for_each_entry_reverse(drv, &drivers.list, list) {
		if (skip) {
			if (drv == drivers.mindrv_tail) {
				skip = 0;
			}
			else {
				udif_print_both(UDIF_TAGID_BEGIN, "suspend skipped: ", drv->name);
				continue ;
			}
		}

		udif_print_both(UDIF_TAGID_BEGIN, "suspend start: ", drv->name);

		if (drv->ops) {
			ret = udif_driver_call_fn_reverse(drv, drv->ops->suspend, drv->nr_devs);
			if (unlikely(ret))
				break;
		}

		udif_print_both(UDIF_TAGID_END, "suspend end:   ", drv->name);

	}
	udif_mutex_unlock(&drivers.mtx);

	udif_print_both(UDIF_TAGID_END, "suspend end:   ", "unified drivers");

	return ret;
}

int udif_driver_resume(void)
{
	int ret = 0;
	UDIF_DRIVER *drv;
	UDIF_UINT success;;

	udif_print_both(UDIF_TAGID_BEGIN, "resume start: ", "unified drivers");

	udif_mutex_lock(&drivers.mtx);
	udif_list_for_each_entry(drv, &drivers.list, list) {

		udif_print_both(UDIF_TAGID_BEGIN, "resume start: ", drv->name);

		if (drv->ops) {
			ret = udif_driver_call_fn(drv, drv->ops->resume, &success);
			if (unlikely(ret)) {
				udif_driver_call_fn_reverse(drv, drv->ops->suspend, success);
				break;
			}
		}

		udif_print_both(UDIF_TAGID_END,   "resume end: ", drv->name);

		if (ssboot_is_rewriting() && (drv == drivers.mindrv_tail)) {
			break ;
		}

	}
	udif_mutex_unlock(&drivers.mtx);

	udif_print_both(UDIF_TAGID_END, "resume end:   ", "unified drivers");

	return ret;
}

int udif_driver_shutdown(void)
{
	int ret = 0;
	UDIF_DRIVER *drv;

	udif_print_both(UDIF_TAGID_BEGIN, "shutdown start: ", "unified drivers");

	udif_mutex_lock(&drivers.mtx);
	udif_list_for_each_entry_reverse(drv, &drivers.list, list) {

		udif_print_both(UDIF_TAGID_BEGIN, "shutdown start: ", drv->name);

		if (drv->ops) {
			ret = udif_driver_call_fn_reverse(drv, drv->ops->shutdown, drv->nr_devs);
			if (unlikely(ret))
				break;
		}

		udif_print_both(UDIF_TAGID_END,   "shudown end: ", drv->name);
	}
	udif_mutex_unlock(&drivers.mtx);

	udif_print_both(UDIF_TAGID_END, "shutdown end:   ", "unified drivers");

	return ret;
}

#define PROC_NAME "drivers"

static UDIF_INT udif_read_proc(UDIF_PROC_READ *proc)
{
	struct udif_drivers_t *drvs = udif_proc_getdata(proc);
	UDIF_DRIVER *drv;
	int len = 0;

#ifdef CONFIG_OSAL_UDIF_THREAD
	len += udif_proc_setbuf(proc, len, "name\t\tversion\t\tdepends\n");
#else
	len += udif_proc_setbuf(proc, len, "name\t\tversion\n");
#endif /* CONFIG_OSAL_UDIF_THREAD */

	udif_mutex_lock(&drvs->mtx);
	udif_list_for_each_entry(drv, &drvs->list, list) {
#ifdef CONFIG_OSAL_UDIF_THREAD
		int i;
		len += udif_proc_setbuf(proc, len, "%s\t\t%s\t\t", drv->name, drv->ver);

		for (i = 0; i < drv->nr_depends; i++) {
			len += udif_proc_setbuf(proc, len, "%s ", drv->depends[i]->name);
		}

		len += udif_proc_setbuf(proc, len, "\n");
#else /* CONFIG_OSAL_UDIF_THREAD */
		len += udif_proc_setbuf(proc, len, "%s\t\t%s\n", drv->name, drv->ver);
#endif /* CONFIG_OSAL_UDIF_THREAD */
	}
	udif_mutex_unlock(&drvs->mtx);

	udif_proc_setend(proc);

	return len;
}

#define UDIF_DRV_PROC_BUFSIZE 8
static UDIF_INT udif_write_proc(UDIF_PROC_WRITE *proc)
{
	struct udif_drivers_t *drvs = udif_proc_getdata(proc);
	UDIF_DRIVER *drv;
	UDIF_U8 buf[UDIF_DRV_PROC_BUFSIZE];

	if(udif_proc_getbuf(proc, buf, UDIF_DRV_PROC_BUFSIZE) == UDIF_DRV_PROC_BUFSIZE){
		udif_printk("Fail to udif_proc_getbuf()\n");
		return UDIF_ERR_FAULT;
	}

	udif_mutex_lock(&drvs->mtx);

	if (!strncmp(buf, "set", 3)) {
		udif_list_for_each_entry_reverse(drv, &drivers.list, list) break;
		drvs->mindrv_tail = drv;
		udif_printk("Set MinDrv: %s\n", drv->name);
	} else if (!strncmp(buf, "reset", 5)) {
		drvs->mindrv_tail = NULL;
		udif_printk("Reset MinDrv\n");
	} else {
		udif_printk("invalid args\n");
	}

	udif_mutex_unlock(&drvs->mtx);

	return proc->count;
}

static UDIF_PROC proc = {
	.name = PROC_NAME,
	.read = udif_read_proc,
	.write= udif_write_proc,
	.data = &drivers,
};

static int __init udif_module_init(void)
{
	return udif_create_proc(&proc);
}

postcore_initcall(udif_module_init);

#if 0
static void __exit udif_module_exit(void)
{
	udif_remove_proc(&proc);
}

module_exit(udif_module_exit);
#endif
