/*
 * drivers/udif/device.c
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
#include <linux/udif/device.h>
#include <linux/udif/mutex.h>
#include <linux/udif/proc.h>
#include <linux/udif/macros.h>
#include <mach/udif/platform.h>

struct udif_devices_t {
	UDIF_LIST list;
	UDIF_MUTEX mtx;
};

static struct udif_devices_t devices = {
	.list = UDIF_LIST_HEAD_INIT(devices.list),
	.mtx  = UDIF_MUTEX_INIT(devices.mtx),
};

static inline void udif_device_list_add(UDIF_DEVICE *dev, struct udif_devices_t *devs)
{
	udif_mutex_lock(&devs->mtx);
	udif_list_add_tail(&dev->list, &devs->list);
	udif_mutex_unlock(&devs->mtx);
}

static inline void udif_device_list_del(UDIF_DEVICE *dev, struct udif_devices_t *devs)
{
	udif_mutex_lock(&devs->mtx);
	udif_list_del(&dev->list);
	udif_mutex_unlock(&devs->mtx);
}

UDIF_ERR udif_device_register(UDIF_DEVICE *dev)
{
	UDIF_ERR ret = UDIF_ERR_OK;
	UDIF_UINT i;

	if (unlikely(!dev)) {
		UDIF_PERR("invalid device\n");
		return UDIF_ERR_PAR;
	}

	if (unlikely(dev->nr_ch > UDIF_MAX_NR_CH)) {
		UDIF_PERR("too many channels: nr_ch = %d\n", dev->nr_ch);
		return UDIF_ERR_PAR;
	}

	if (!dev->init) {
		goto end;
	}

	for (i = 0; i < dev->nr_ch; i++) {
		ret = dev->init(&dev->chs[i]);
		if (unlikely(ret)) {
			UDIF_PERR("%s error %d\n", dev->name, ret);
			break;
		}
	}

end:
	if (likely(!ret))
		udif_device_list_add(dev, &devices);

	return ret;
}

void udif_device_unregister(UDIF_DEVICE *dev)
{
	UDIF_UINT i;

	if (unlikely(!dev)) {
		UDIF_PERR("invalid device\n");
		return;
	}

	udif_device_list_del(dev, &devices);

	if (!dev->exit) {
		return;
	}

	i = dev->nr_ch;
	while (i--) {
		dev->exit(&dev->chs[i]);
	}
}

#define PROC_NAME "devices"

static UDIF_INT udif_read_proc(UDIF_PROC_READ *proc)
{
	struct udif_devices_t *devs = udif_proc_getdata(proc);
	UDIF_DEVICE *dev;
	int len = 0;

	len += udif_proc_setbuf(proc, len, "id name\tnch\n");
	len += udif_proc_setbuf(proc, len, "   ch          iomem          irq\n");

	udif_mutex_lock(&devs->mtx);
	udif_list_for_each_entry(dev, &devs->list, list) {
		UDIF_CH ch;
		len += udif_proc_setbuf(proc, len, "%02d %s\t%d\n", udif_dev_id(dev), dev->name, dev->nr_ch);
		for (ch = 0; ch < dev->nr_ch; ch++) {
			len += udif_proc_setbuf(proc, len, "   %2d 0x%08lx - 0x%08lx %3d\n",
						ch,
						udif_devio_phys(dev, ch),
						udif_devio_phys(dev, ch) + udif_devio_size(dev, ch) - 1,
						udif_devint_irq(dev, ch));
		}
	}
	udif_mutex_unlock(&devs->mtx);

	udif_proc_setend(proc);

	return len;
}

static UDIF_PROC proc = {
	.name = PROC_NAME,
	.read = udif_read_proc,
	.data = &devices,
};

static int __init udif_module_init(void)
{
	return udif_create_proc(&proc);
}

postcore_initcall(udif_module_init);
