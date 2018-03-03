/*
 * arch/arm/plat-cxd41xx/clock.c
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
#include <linux/udif/types.h>
#include <linux/udif/macros.h>
#include <linux/udif/device.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/mutex.h>
#include <mach/platform.h>

static UDIF_DECLARE_SPINLOCK(udif_clock_lock);

UDIF_ERR __udif_devio_hclk(UDIF_CLOCK *clock, UDIF_U8 enable)
{
	UDIF_PARM_CHK(!clock, "invalid clock", UDIF_ERR_PAR);
#if 0
	UDIF_PARM_CHK(!clock->hclk.set, "invalid hclk", UDIF_ERR_PAR);
#else
	if (!clock->hclk.set) {
		UDIF_PERR("invalid hclk: IGNORED.\n");
		return UDIF_ERR_OK;
	}
#endif
	if (enable >= 2) {
		return UDIF_ERR_PAR;
	}

	if (enable) {
		udif_iowrite32(1<<clock->hclk.shift, clock->hclk.set);
	}
	else {
		udif_iowrite32(1<<clock->hclk.shift, clock->hclk.clr);
	}
	return UDIF_ERR_OK;
}


UDIF_ERR __udif_devio_devclk(UDIF_CLOCK *clock, UDIF_U8 enable, UDIF_U8 clksel)
{
	UDIF_U32 tmp;
	UDIF_ULONG flags;

	UDIF_PARM_CHK(!clock, "invalid clock", UDIF_ERR_PAR);
#if 0
	UDIF_PARM_CHK(!clock->dclk.set, "invalid devclk", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!clock->dclk.clr, "invalid devclk", UDIF_ERR_PAR);
#else
	if (!clock->dclk.set || !clock->dclk.clr) {
		UDIF_PERR("invalid devclk: IGNORED.\n");
		return UDIF_ERR_OK;
	}
#endif
	if (enable >= 2) {
		return UDIF_ERR_PAR;
	}

	if (enable) {
		if ((clock->dclksel.set) && !(clock->dclksel.clr)) {
			udif_spin_lock_irqsave(&udif_clock_lock, flags);
			tmp = udif_ioread32(clock->dclksel.set);
			tmp &= ~(0xff<<clock->dclksel.shift);
			tmp |= clksel<<clock->dclksel.shift;
			udif_iowrite32(tmp, clock->dclksel.set);
			udif_spin_unlock_irqrestore(&udif_clock_lock, flags);
		}
		else {
			if (clock->dclksel.clr) {
				udif_iowrite32(0xff<<clock->dclksel.shift,
					       clock->dclksel.clr);
			}
			if (clock->dclksel.set) {
				udif_iowrite32(clksel<<clock->dclksel.shift,
					       clock->dclksel.set);
			}
		}
		udif_iowrite32(1<<clock->dclk.shift, clock->dclk.set);
	}
	else {
		udif_iowrite32(1<<clock->dclk.shift, clock->dclk.clr);
	}
	return UDIF_ERR_OK;
}

#define DECLARE_DIVCLK(id, vaddr) \
	[UDIF_CLK_ID_##id] = { .base = (UDIF_VA)(vaddr), .mtx = UDIF_MUTEX_INIT(divclk[UDIF_CLK_ID_##id].mtx) }

struct divclk_t {
	UDIF_VA base;
	UDIF_MUTEX mtx;
};

static struct divclk_t divclk[UDIF_CLK_ID_NUM] = {
	DECLARE_DIVCLK(CRG11_00, VA_CLKCRG0(0)),
	DECLARE_DIVCLK(CRG11_01, VA_CLKCRG0(1)),
	DECLARE_DIVCLK(CRG11_10, VA_CLKCRG1(0)),
	DECLARE_DIVCLK(CRG11_11, VA_CLKCRG1(1)),
	DECLARE_DIVCLK(CRG11_20, VA_CLKCRG2(0)),
	DECLARE_DIVCLK(CRG11_21, VA_CLKCRG2(1)),
	DECLARE_DIVCLK(CRG11_3,  VA_CLKCRG3(0)),
	DECLARE_DIVCLK(CRG11_40, VA_CLKCRG4(0)),
	DECLARE_DIVCLK(CRG11_41, VA_CLKCRG4(1)),
};

UDIF_ERR udif_change_clk(UDIF_CLK_ID id, UDIF_CLK_CB cb, UDIF_VP data)
{
	struct divclk_t *clk;
	UDIF_ERR ret;

	if (id >= UDIF_CLK_ID_NUM) {
		UDIF_PERR("register_clk_cb: invalid id %d\n", id);
		return UDIF_ERR_PAR;
	}

	if (!cb) {
		UDIF_PERR("register_clk_cb: invalid cb(NULL)\n");
		return UDIF_ERR_PAR;
	}

	clk = &divclk[id];
	udif_mutex_lock(&clk->mtx);

	ret = cb(id, clk->base, data);

	udif_mutex_unlock(&clk->mtx);

	return ret;
}

EXPORT_SYMBOL(__udif_devio_hclk);
EXPORT_SYMBOL(__udif_devio_devclk);
EXPORT_SYMBOL(udif_change_clk);

