/*
 * arch/arm/mach-cxd90014/include/mach/pl310.h
 *
 * PL310 settings
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
#ifndef __MACH_PL310_H__
#define __MACH_PL310_H__

#include <asm/hardware/cache-l2x0.h>

#define PL310_AUX_WAY	(1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT)

#define TAGRAM_LATENCY	0x000	/* 2/2/2 */
#define DATARAM_LATENCY	0x000	/* 2/2/2 */

#define AUX_MASK	0x000f0000

#ifdef CONFIG_EXCLUSIVE_CACHE
# define AUX_VAL		0x02401000 /* RR, ignore Sh.attr, Excl */
#else
# define AUX_VAL		0x02400000 /* RR, ignore Sh.attr */
#endif /* CONFIG_EXCLUSIVE_CACHE */

#endif /* __MACH_PL310_H__ */
