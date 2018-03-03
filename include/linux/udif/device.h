/*
 * include/linux/udif/device.h
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
#ifndef __UDIF_DEVICE_H__
#define __UDIF_DEVICE_H__

#include <linux/udif/types.h>
#include <linux/udif/io.h>
#include <linux/udif/dmac.h>
#include <linux/udif/irq.h>
#include <linux/udif/list.h>
#include <mach/udif/clock.h>

#define NARRAY(x)	(sizeof(x)/sizeof((x)[0]))
#define UDIF_MAX_NR_CH	31

typedef struct UDIF_CHANNELS {
	UDIF_IOMEM	iomem;
	UDIF_INTRPT	intr;
	UDIF_CLOCK	clock;
} UDIF_CHANNELS;

typedef struct UDIF_DEVICE {
	const UDIF_U8		name[8];
	__UDIF_DMAC		dmac;
	const UDIF_UINT		nr_ch;
	UDIF_CHANNELS		*chs;
	UDIF_ERR 		(*init)(UDIF_CHANNELS *);
	void 			(*exit)(UDIF_CHANNELS *);
	UDIF_LIST		list;
} UDIF_DEVICE;

UDIF_ERR udif_device_register(UDIF_DEVICE *);
void udif_device_unregister(UDIF_DEVICE *);

#define	UDIF_IDS(name) \
	static const UDIF_DEVICES name[]

#define udif_devio_virt(devs,ch)	((devs)?udif_iomem_virt(&(devs)->chs[ch].iomem):0)
#define udif_devio_phys(devs,ch)	((devs)?udif_iomem_phys(&(devs)->chs[ch].iomem):0)
#define udif_devio_size(devs,ch)	((devs)?udif_iomem_size(&(devs)->chs[ch].iomem):0)
#define udif_devio_flags(devs,ch)	((devs)?udif_iomem_flags(&(devs)->chs[ch].iomem):(-1UL))

#define udif_devio_hclk(devs,ch,ena)		((devs)?__udif_devio_hclk(&(devs)->chs[ch].clock, ena):UDIF_ERR_PAR)
#define udif_devio_devclk(devs,ch,ena,sel)	((devs)?__udif_devio_devclk(&(devs)->chs[ch].clock,ena,sel):UDIF_ERR_PAR)

#define udif_devint_irq(devs,ch)	((devs)?udif_intr_irq(&(devs)->chs[ch].intr):0)
#define udif_devint_flags(devs,ch)	((devs)?udif_intr_flags(&(devs)->chs[ch].intr):(-1UL))

#define udif_request_irq(devs,ch,hndl,data)	((devs)?__udif_request_irq(&(devs)->chs[ch].intr,hndl,data):UDIF_ERR_PAR)
#define udif_free_irq(devs,ch)			((devs)?__udif_free_irq(&(devs)->chs[ch].intr):UDIF_ERR_PAR)
#define udif_enable_irq(irq)		__udif_enable_irq(irq)
#define udif_disable_irq(irq)		__udif_disable_irq(irq)
#define udif_disable_irq_nosync(irq)	__udif_disable_irq_nosync(irq)

#define udif_dmac_num(devs)		__udif_dmac_num(&(devs)->dmac)
#define udif_dmac_ch(devs,i)		__udif_dmac_ch(&(devs)->dmac,i)
#define udif_dmac_rqid(devs,i)		__udif_dmac_rqid(&(devs)->dmac,i)
#define udif_dmac_irq(devs,i)		__udif_dmac_irq(&(devs)->dmac,i)
#define udif_dmac_flags(devs,i)		__udif_dmac_flags(&(devs)->dmac,i)

#define udif_dmac_request_irq(devs,i,hndl,data)	((devs)?__udif_dmac_request_irq(&(devs)->dmac,i,hndl,data):UDIF_ERR_PAR)
#define udif_dmac_free_irq(devs,i)		((devs)?__udif_dmac_free_irq(&(devs)->dmac,i):UDIF_ERR_PAR)

#define udif_dmastop(devs,i)		__udif_dmastop(&(devs)->dmac,i)
#define udif_dmastart(devs,i,p,c)	__udif_dmastart(&(devs)->dmac,i,p,c)
#define udif_dmaclear(devs,i)		__udif_dmaclear(&(devs)->dmac,i)
#define udif_dmainttc_status(devs,i)	__udif_dmainttc_status(&(devs)->dmac,i)

#define udif_clearirq(devs,ch)	__udif_clearirq(&(devs)->chs[ch].intr)
#define udif_maskirq(devs,ch,e)	__udif_maskirq(&(devs)->chs[ch].intr,e)
#define udif_pollirq(devs,ch)	__udif_pollirq(&(devs)->chs[ch].intr)

#endif /* __UDIF_DEVICE_H__ */
