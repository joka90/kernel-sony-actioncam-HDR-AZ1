/*
 * arch/arm/mach-cxd4132/fcs.c
 *
 * FCS driver
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/pm_speed.h>
#include <mach/regs-clk.h>
#include <asm/io.h>

static irqreturn_t fcs_intr(int irq, void *dev_id)
{
	u32 stat;

	stat = readl(VA_FCS(FCS_MASKED_STATUS));
	writel(stat, VA_FCS(FCS_MASKED_STATUS));

	pm_speed_notify();
	return IRQ_HANDLED;
}

static void fcs_setup(int irq)
{
	irq_set_affinity(irq, cpumask_of(0));
	writel(FCS_INT_FIN, VA_FCS(FCS_INT_ENABLE));
}

static int __devinit fcs_probe(struct platform_device *pdev)
{
	struct resource *fcs_irq;
	int irq, ret;

	fcs_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!fcs_irq) {
		printk(KERN_ERR "ERROR:FCS: can not allocate irq resource.\n");
		return -ENODEV;
	}
	irq = fcs_irq->start;

	fcs_setup(irq);
	ret = request_irq(irq, fcs_intr, IRQF_NO_THREAD|IRQF_DISABLED,
			  "fcs", NULL);
	if (ret) {
		printk(KERN_ERR "ERROR:FCS: request_irq %d failed: %d\n",
		       irq, ret);
	}
	platform_set_drvdata(pdev, (void *)irq);
	return ret;
}

static int __devexit fcs_remove(struct platform_device *pdev)
{
	int irq;

	irq = (int)platform_get_drvdata(pdev);
	free_irq(irq, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int fcs_suspend(struct platform_device *dev, pm_message_t state)
{
	int irq;

	irq = (int)platform_get_drvdata(dev);
	disable_irq(irq);
	return 0;
}

static int fcs_resume(struct platform_device *dev)
{
	int irq = (int)platform_get_drvdata(dev);

	fcs_setup(irq);
	enable_irq(irq);
	return 0;
}
#endif /* CONFIG_PM */

static struct platform_driver fcs_driver = {
	.probe = fcs_probe,
	.remove = fcs_remove,
#ifdef CONFIG_PM
	.suspend = fcs_suspend,
	.resume	 = fcs_resume,
#endif
	.driver = {
		.name = "fcs",
	},
};

static int __init fcs_init(void)
{
	return platform_driver_register(&fcs_driver);
}

static void __exit fcs_cleanup(void)
{
	platform_driver_unregister(&fcs_driver);
}

module_init(fcs_init);
module_exit(fcs_cleanup);
