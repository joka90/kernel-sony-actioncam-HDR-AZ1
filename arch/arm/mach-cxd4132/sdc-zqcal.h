/*
 * arch/arm/mach-cxd4132/sdc-zqcal.h
 *
 * SDC ZQ calibration header
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

#ifndef __ARCH_CXD4132_SDC_ZQCAL_H__
#define __ARCH_CXD4132_SDC_ZQCAL_H__

#include <linux/udif/macros.h>
#include <linux/udif/proc.h>

struct zqcal_rank {
	UDIF_U32 flags;
	UDIF_U32 t1;
};
/* flags */
#define ZQCAL_STARTUP	0x00000002
#define ZQCAL_NEEDLONG	0x00000004

struct zqcal_ctl {
	struct timer_list timer;
	UDIF_UINT enable;
	/* RANK ctrl */
	UDIF_UINT n_rank, cur_rank;
	struct zqcal_rank ranks[SDC_MAX_RANK];
	/* calibration period */
	UDIF_UINT period;
	UDIF_ULONG ticks;
	/* statistics */
	UDIF_U32 t_min;
	UDIF_U32 t_max;
	ulong t_last;
};

#define ZQCAL_REQUIRED(dev)	((dev)->is_ddr2)

/* LPDDR2 definitions */
#define LPDDR2_ZQREG	10	/* MR10 */
#define LPDDR2_ZQINIT	0xff
#define LPDDR2_ZQLONG	0xab
#define LPDDR2_ZQSHORT	0x56
#define LPDDR2_ZQRESET	0xc3
#define LPDDR2_TZQCL	360 /* ns */
#define LPDDR2_TZQCS	90  /* ns */

#define SDC_LPDDR2_CLK	312 /* MHz */
#define SDC_ZQLONG_PERIOD  DIV_ROUND_UP(LPDDR2_TZQCL*SDC_LPDDR2_CLK, 1000)
#define SDC_ZQSHORT_PERIOD DIV_ROUND_UP(LPDDR2_TZQCS*SDC_LPDDR2_CLK, 1000)

#endif /* __ARCH_CXD4132_SDC_ZQCAL_H__ */
