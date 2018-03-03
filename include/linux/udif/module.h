/*
 * include/linux/udif/module.h
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
#ifndef __UDIF_MODULE_H__
#define __UDIF_MODULE_H__

#include <linux/module.h>
#include <linux/udif/driver.h>
#include <mach/udif/platform.h>

#define UDIF_MODULE_PARAM module_param

#ifndef UDIF_MODULE_DEFINE
# define UDIF_MODULE_DEFINE
#endif /* UDIF_MODULE_DEFINE */

#define UDIF_DEPS(name) \
	static UDIF_DRIVER *name[]

#if defined(MODULE) && defined(UDIF_UNIFY)
# define DEF_UDIF_INIT_EXIT(name)

# define UDIF_DECLARE_DEP(mod) \
	extern UDIF_DRIVER udif_##mod

# define UDIF_DEP(mod) \
	&udif_##mod

#else

# define UDIF_DECLARE_DEP(mod)
# define UDIF_DEP(mod) NULL

# define DEF_UDIF_INIT_EXIT(name) \
static int __init __udif_init_##name(void) \
{ \
	UDIF_DRIVER *drv = &name; \
	return udif_driver_register(&drv, 1); \
}; \
module_init(__udif_init_##name); \
static void __exit __udif_exit_##name(void) \
{ \
	UDIF_DRIVER *drv = &name; \
	udif_driver_unregister(&drv, 1); \
} \
module_exit(__udif_exit_##name)
#endif

#define UDIF_MODULE(name, string, ver, ops, ids, dps, data) \
	UDIF_DECLARE_DRIVER( \
			udif_##name, string, ver, &ops, \
			ids, dps, data); \
	DEF_UDIF_INIT_EXIT(udif_##name); \
	UDIF_MODULE_DEFINE

#endif /* __UDIF_MODULE_H__ */
