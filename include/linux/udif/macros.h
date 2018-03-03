/*
 * include/linux/udif/macros.h
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
#ifndef __UDIF_MACROS_H__
#define __UDIF_MACROS_H__

#include <linux/kernel.h>

#ifdef CONFIG_OSAL_UDIF_ERROR_PRINT
# define UDIF_PERR(fmt, args...) \
	printk(KERN_ERR "UDIF: %s() Line %d: " fmt, __func__, __LINE__, ##args)
#else
# define UDIF_PERR(fmt, args...) do {} while(0)
#endif /* CONFIG_OSAL_UDIF_ERROR_PRINT */

#define UDIF_PINFO(fmt, args...) \
	printk(KERN_INFO "UDIF: " fmt, ##args)

#define UDIF_CALL_FN(dev, fn, type, ival, args...) \
({ \
	type ret = (ival); \
	if (likely((dev)->ops) && (dev)->ops->fn) { \
		ret = (dev)->ops->fn(args); \
		if (unlikely(ret < 0)) \
			UDIF_PERR("%s: warning ret = %d\n", (dev)->node->name, (int)ret); \
	} \
	ret; \
})

#define UDIF_DECLARE_FILE(name, no, dt) \
UDIF_FILE name = { \
	.devno = no, \
	.data = dt, \
}

#define UDIF_DECLARE_IOCTL(name, c, a) \
UDIF_IOCTL name = { \
	.cmd = c, \
	.arg = a, \
	.disk_change = 0, \
}

#define UDIF_PARM_CHK(c, msg, err) \
do { \
	if (unlikely(c)) { \
		UDIF_PERR("Parm Error: %s: " #c "\n", msg); \
		return (err); \
	} \
} while (0)

#if 0
# define UDIF_BUGON(c) \
do { \
	if (unlikely(c)) { \
		UDIF_PERR("BUG\n"); \
		*(int *) = 0; \
	} \
} while (0)

# define UDIF_WARN(c, fmt, args...) \
do { \
	if (unlikely(c)) { \
		UDIF_PERR("Warning: " fmt, ##args); \
	} \
} while (0)

# define UDIF_PARM_CHK_FN(c, fn, args...) \
({ \
	UDIF_ERR ret = UDIF_ERR_OK; \
	if (likely(c)) { \
		fn(args); \
	} else { \
		UDIF_PERR("invalid pointer\n"); \
		ret = UDIF_ERR_PAR; \
	} \
	ret; \
})
#else
  /* to avoid warning "addresing xxx will always evaluate as true" */
# define UDIF_PARM_CHK_FN(c, fn, args...) fn(args)
#endif

#endif /* __UDIF_MACROS_H__ */

