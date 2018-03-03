/*
 * arch/arm/mach-cxd4132/sdc.h
 *
 * SDC driver header
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

#ifndef __ARCH_CXD4132_SDC_H__
#define __ARCH_CXD4132_SDC_H__

#include <linux/udif/device.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/macros.h>

#define SDC_MAX_RANK	2

#include "sdc-zqcal.h"

struct sdc_dev_t {
	UDIF_U32	magic;
	const UDIF_DEVICE	*udev;

	UDIF_SPINLOCK	sdc_lock;
	UDIF_VA		vaddr;
	UDIF_INT	is_2rank;
	UDIF_INT	is_ddr2;
	UDIF_U32	errors;
	/* ZQcal stuff */
	struct zqcal_ctl zqcal;
};
/* magic */
#define SDC_DEV_MAGIC 0x00434453
#define SDC_DEV_FREE  0
/* errors */
#define SDC_ERR_NONE	0
#define SDC_ERR_TIMEOUT	0x00000001

extern void sdc_ddr2_mrw(struct sdc_dev_t *dev, UDIF_U32 cmd,
			 UDIF_U32 ma, UDIF_U32 op, UDIF_U32 tMRW);

#endif /* __ARCH_CXD4132_SDC_H__ */
