/*
 * arch/arm/mach-cxd90014/include/mach/udif/platform.h
 *
 *
 * Copyright 2012 Sony Corporation
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
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
#ifndef __UDIF_PLATFORM_H__
#define __UDIF_PLATFORM_H__

/* DEVICE ID */
typedef enum {
	UDIF_ID_HWTIMER = 0,
	UDIF_ID_GPIO,
	UDIF_ID_NAND,
	UDIF_ID_DMAC,
	UDIF_ID_LDEC,
	UDIF_ID_MMC,
	UDIF_ID_SIRCS,
	UDIF_ID_I2C,
	UDIF_ID_MS,
	UDIF_ID_HDMI,
	UDIF_ID_CEC,
	UDIF_ID_FUSB_OTG,
        UDIF_ID_SIO,
	UDIF_ID_SATA,
	UDIF_ID_NUM,
} UDIF_ID;

#define UDIF_NR_HWTIMER	4
#define UDIF_NR_GPIO	18U
#define UDIF_NR_DMAC    8U
#define UDIF_NR_NAND    5U
#define UDIF_NR_LDEC    1U
#define UDIF_NR_MMC	3
#define UDIF_NR_SIRCS	2
#define UDIF_NR_I2C	1
#define UDIF_NR_MS	1
#define UDIF_NR_HDMI	1
#define UDIF_NR_CEC	1
#define UDIF_NR_FUSB    8U
#define UDIF_NR_SIO     5
#define UDIF_NR_SATA    0

#define UDIF_SIRCS_RX		0
#define UDIF_SIRCS_OVERRUN	1

#ifdef __KERNEL__
#include <linux/udif/device.h>

extern UDIF_DEVICE udif_devices[UDIF_ID_NUM];

#define udif_get_device(id)	((id) < UDIF_ID_NUM ? &udif_devices[(id)]: NULL)
#define udif_dev_id(dev)	((dev) ? ((dev) - udif_devices): -1)

#define	UDIF_ID(id, msk) \
	{ .dev = &udif_devices[(id)], .ch_mask = msk }
#endif

#endif /* __UDIF_PLATFORM_H__ */
