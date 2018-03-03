/*
 * drivers/udif/irq.c
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/udif/types.h>
#include <linux/udif/irq.h>
#include <linux/udif/macros.h>

static bool udif_nosoftirq = false;
module_param_named(udif_nosoftirq, udif_nosoftirq, bool, S_IRUSR);

#define UDIF_DECLARE_INTR(name, i, d) \
UDIF_INTR name = { \
	.irq	= i, \
	.data	= d, \
}

static irqreturn_t udif_handler(int irq, void *dev_id)
{
	UDIF_INTRPT *intr = dev_id;
	UDIF_DECLARE_INTR(hndl, irq, intr->data);
	int ret;

	if (unlikely(!intr)) {
		UDIF_PERR("invalid interrupt irq = %d\n", irq);
		return IRQ_NONE;
	}

	if (unlikely(intr->irq != irq)) {
		UDIF_PERR("%s: Wrong irq = %d, expected irq = %d\n", intr->name, irq, intr->irq);
		return IRQ_NONE;
	}

	ret = intr->hndl(&hndl);
	if (unlikely(ret != UDIF_ERR_OK)) {
		UDIF_PERR("%s: failed handler irq = %d\n", intr->name, irq);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

#define UDIF_IRQF_DEFAULT (IRQF_DISABLED|((udif_nosoftirq)?IRQF_NO_SOFTIRQ_CALL:0))

UDIF_ERR __udif_request_irq(UDIF_INTRPT *intr, UDIF_INT_CB hndl, UDIF_VP data)
{
	UDIF_ERR ret;

	UDIF_PARM_CHK(!intr, "invalid intr", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!intr->irq, intr->name, UDIF_ERR_PAR);
	UDIF_PARM_CHK(intr->hndl, intr->name, UDIF_ERR_PAR);

	intr->hndl = hndl;
	intr->data = data;

	ret = request_irq(intr->irq, udif_handler, intr->flags | UDIF_IRQF_DEFAULT, intr->name, intr);
	if (unlikely(ret)) {
		UDIF_PERR("failed request_irq(), %s: irq = %d, flags = %08x\n", intr->name, intr->irq, intr->flags | UDIF_IRQF_DEFAULT);
		intr->data = 0;
		intr->hndl = 0;
	}

	return ret;
}

UDIF_ERR __udif_free_irq(UDIF_INTRPT *intr)
{
	UDIF_PARM_CHK(!intr, "invalid intr", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!intr->irq, intr->name, UDIF_ERR_PAR);
	UDIF_PARM_CHK(!intr->hndl, intr->name, UDIF_ERR_PAR);

	free_irq(intr->irq, intr);

	intr->data = 0;
	intr->hndl = 0;

	return UDIF_ERR_OK;
}

UDIF_ERR __udif_enable_irq(UDIF_IRQ irq)
{
	enable_irq((unsigned int)irq);
	return UDIF_ERR_OK;
}

UDIF_ERR __udif_disable_irq(UDIF_IRQ irq)
{
	disable_irq((unsigned int)irq);
	return UDIF_ERR_OK;
}

UDIF_ERR __udif_disable_irq_nosync(UDIF_IRQ irq)
{
	disable_irq_nosync((unsigned int)irq);
	return UDIF_ERR_OK;
}

EXPORT_SYMBOL(__udif_request_irq);
EXPORT_SYMBOL(__udif_free_irq);
EXPORT_SYMBOL(__udif_enable_irq);
EXPORT_SYMBOL(__udif_disable_irq);
EXPORT_SYMBOL(__udif_disable_irq_nosync);
