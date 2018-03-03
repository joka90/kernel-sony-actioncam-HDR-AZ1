/*
 * arch/arm/plat-cxd41xx/clock.c
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
#include <linux/module.h>
#include <linux/udif/types.h>
#include <linux/udif/macros.h>
#include <linux/udif/device.h>
#include <linux/udif/spinlock.h>

static UDIF_DECLARE_SPINLOCK(udif_clock_lock);

UDIF_ERR __udif_devio_hclk(UDIF_CLOCK *clock, UDIF_U8 enable)
{
	UDIF_PARM_CHK(!clock, "invalid clock", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!clock->hclk.set, "invalid hclk", UDIF_ERR_PAR);
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
	UDIF_U32 tmp, flags;

	UDIF_PARM_CHK(!clock, "invalid clock", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!clock->dclk.set, "invalid devclk", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!clock->dclk.clr, "invalid devclk", UDIF_ERR_PAR);
	if (enable >= 2) {
		return UDIF_ERR_PAR;
	}

	if (enable) {
		if ((clock->dclksel.set) && !(clock->dclksel.clr)) {
			udif_spin_lock_irqsave(&udif_clock_lock, flags);
			tmp = udif_ioread32(clock->dclksel.set);
			tmp &= ~(0xf<<clock->dclksel.shift);
			tmp |= clksel<<clock->dclksel.shift;
			udif_iowrite32(tmp, clock->dclksel.set);
			udif_spin_unlock_irqrestore(&udif_clock_lock, flags);
		}
		else {
			if (clock->dclksel.clr) {
				udif_iowrite32(0xf<<clock->dclksel.shift,
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


EXPORT_SYMBOL(__udif_devio_hclk);
EXPORT_SYMBOL(__udif_devio_devclk);
