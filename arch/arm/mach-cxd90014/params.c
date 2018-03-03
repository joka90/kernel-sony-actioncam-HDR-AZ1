/*
 * arch/arm/plat-cxd41xx/params.c
 *
 * module parameter
 *
 * Copyright 2009,2010 Sony Corporation
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
#include <linux/moduleparam.h>
#include <mach/moduleparam.h>

/*
 * Port parameter: PORT,BIT
 */
static void port_help(const char *p)
{
	printk(KERN_ERR "Usage: %s=PORT,BIT\n", p);
}

int param_set_port(const char *val, const struct kernel_param *kp)
{
	int ret, data;
	char ctrl[2];

	if (!val) {
		port_help(kp->name);
		return -EINVAL;
	}
	/* 1st param (PORT) */
	ret = get_option((char **)&val, &data);
	if (2 != ret) {
		port_help(kp->name);
		return -EINVAL;
	}
	ctrl[0] = data;
	/* 2nd param (BIT) */
	ret = get_option((char **)&val, &data);
	if (1 != ret) {
		port_help(kp->name);
		return -EINVAL;
	}
	ctrl[1] = data;

	((char *)kp->arg)[0] = ctrl[0];
	((char *)kp->arg)[1] = ctrl[1];
	return 0;
}

int param_get_port(char *buffer, const struct kernel_param *kp)
{
	char ctrl[2];

	ctrl[0] = ((char *)kp->arg)[0];
	ctrl[1] = ((char *)kp->arg)[1];
	return snprintf(buffer, 16, "%d,%d", ctrl[0], ctrl[1]);
}

struct kernel_param_ops param_ops_port = {
	.set = param_set_port,
	.get = param_get_port,
};
EXPORT_SYMBOL(param_ops_port);

#ifdef CONFIG_PM
#ifndef CONFIG_CXD90014_SIMPLE_SUSPEND
#include <mach/hardware.h>
#include <mach/regs-ddmc.h>
/*
 * DDR power down control parameters
 *
 *  {no|sref|dpd}
 *
 *    no:   Do not control  (default)
 *    sref: SelfRefresh
 *    dpd:  DeepPowerDown
 */
static void ddrctrl_help(const char *p)
{
	printk(KERN_ERR "Usage: %s={no|sref|dpd}\n", p);
}

static const char *ddr_ctrl_param(const char *p, char *result)
{
#if 0
	char ctrl;

	if (strncmp(p, "sref", 4) == 0) {
		ctrl = DMC_DDRCTRL_SREF;
		p += 4;
	} else if (strncmp(p, "dpd", 3) == 0) {
		ctrl = DMC_DDRCTRL_DPD;
		p += 3;
	} else if (!strncmp(p, "no", 2)) {
		ctrl = DMC_DDRCTRL_NO;
		p += 2;
	} else {
		return NULL;
	}
	/* delimiter ? */
	if (*p == ',' || *p == ' ' || *p == '\0' || *p == '\n') {
		*result = ctrl;
		return p;
	}
#endif
	return NULL;
}

int param_set_ddrctrl(const char *val, const struct kernel_param *kp)
{
	char ctrl[1];

	if (!val) {
		ddrctrl_help(kp->name);
		return -EINVAL;
	}
	/* 1st param */
	val = ddr_ctrl_param(val, &ctrl[0]);
	if (!val) {
		ddrctrl_help(kp->name);
		return -EINVAL;
	}
	((char *)kp->arg)[0] = ctrl[0];
	return 0;
}

static char *param_ddrctrl_str(int ctrl)
{
	char *str;

	switch (ctrl) {
#if 0
	case DMC_DDRCTRL_SREF:
		str = "sref";
		break;
	case DMC_DDRCTRL_DPD:
		str = "dpd";
		break;
#endif
	default:
		str = "no";
		break;
	}
	return str;
}

int param_get_ddrctrl(char *buffer, const struct kernel_param *kp)
{
	char ctrl[1];

	ctrl[0] = ((char *)kp->arg)[0];
	return snprintf(buffer, 16, "%s",
			param_ddrctrl_str(ctrl[0]));
}

struct kernel_param_ops param_ops_ddrctrl = {
	.set = param_set_ddrctrl,
	.get = param_get_ddrctrl,
};
EXPORT_SYMBOL(param_ops_ddrctrl);
#endif /* !CONFIG_CXD90014_SIMPLE_SUSPEND */
#endif /* CONFIG_PM */
