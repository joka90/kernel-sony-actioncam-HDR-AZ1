/*
 * include/linux/udif/driver.h
 *
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
#ifndef __UDIF_DRIVER_H__
#define __UDIF_DRIVER_H__

#include <linux/udif/device.h>
#ifdef CONFIG_OSAL_UDIF_THREAD
#include <linux/completion.h>
#endif

typedef	UDIF_ERR (*UDIF_MODULE_INIT_CB)(UDIF_VP);
typedef	UDIF_ERR (*UDIF_MODULE_CB)(const UDIF_DEVICE *, UDIF_CH, UDIF_VP);

typedef struct UDIF_DRIVER_OPS {
	const UDIF_MODULE_INIT_CB init;
	const UDIF_MODULE_INIT_CB exit;
	const UDIF_MODULE_CB	probe;
	const UDIF_MODULE_CB	remove;
	const UDIF_MODULE_CB	suspend;
	const UDIF_MODULE_CB	resume;
	const UDIF_MODULE_CB	shutdown;
} UDIF_DRIVER_OPS;

#define UDIF_CH_MASK_DEFAULT (-1UL)

typedef struct UDIF_DEVICES {
	const UDIF_DEVICE	*dev;
	const UDIF_U32		ch_mask;
} UDIF_DEVICES;

typedef struct UDIF_DRIVER UDIF_DRIVER;

struct UDIF_DRIVER {
	UDIF_LIST		list;
	const UDIF_U8		name[12];
	const UDIF_U8		ver[12];
	const UDIF_DEVICES	*devs;
	UDIF_UINT		nr_devs;
	UDIF_DRIVER_OPS		*ops;
	const UDIF_VP		data;
#ifdef CONFIG_OSAL_UDIF_THREAD
	UDIF_VP			th;
	struct completion	com;
	UDIF_DRIVER		**depends;
	UDIF_UINT		nr_depends;
#endif
};

#ifdef CONFIG_OSAL_UDIF_THREAD
# define UDIF_DECLARE_DRIVER(drv, st, v, op, ids, dps, dt) \
UDIF_DRIVER drv = { \
	.name	= st, \
	.ver	= v, \
	.devs	= ids, \
	.nr_devs = NARRAY(ids), \
	.ops	= op, \
	.data	= dt, \
	.com	= COMPLETION_INITIALIZER(drv.com), \
	.depends = dps, \
	.nr_depends = NARRAY(dps), \
}; \
EXPORT_SYMBOL(drv)
#else
# define UDIF_DECLARE_DRIVER(drv, st, v, op, ids, dps, dt) \
UDIF_DRIVER drv = { \
	.name	= st, \
	.ver	= v, \
	.devs	= ids, \
	.nr_devs = NARRAY(ids), \
	.ops	= op, \
	.data	= dt, \
}
#endif /* CONFIG_OSAL_UDIF_THREAD */

int  udif_driver_register(UDIF_DRIVER **, UDIF_U32);
void udif_driver_unregister(UDIF_DRIVER **, UDIF_U32);

#endif /* __UDIF_DRIVER_H__ */
