/*
 * arch/arm/mach-cxd90014/include/mach/vectors-layout.h
 *
 * Exception vector page layout
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
#define FIQ_VEC_OFFS		0x0000001c
#define FIQ_VEC_END		0x00000200
#define MAX_FIQ_VEC		(FIQ_VEC_END - FIQ_VEC_OFFS)

#define CXD90014_FIQ_REGSAVE	0x00000600
# define CXD90014_FIQ_N_REGS	24
# define CXD90014_FIQ_REGSIZE	(4 * CXD90014_FIQ_N_REGS)
# define CXD90014_MAX_CPUS	3
# define CXD90014_FIQ_REG_ALL	(CXD90014_FIQ_REGSIZE * CXD90014_MAX_CPUS)

#define CXD90014_FIQ_DEBUG	0x00000800
#define CXD90014_FIQ_DEBUG_END	0x00000f00
#define MAX_FIQ_DEBUG		(CXD90014_FIQ_DEBUG_END - CXD90014_FIQ_DEBUG)

#define VA_VECTORS_REGSAVE	(CONFIG_VECTORS_BASE + CXD90014_FIQ_REGSAVE)
#define VA_VECTORS_DEBUG	(CONFIG_VECTORS_BASE + CXD90014_FIQ_DEBUG)

#if CXD90014_FIQ_REGSAVE + CXD90014_FIQ_REG_ALL >= CXD90014_FIQ_DEBUG
# error "CXD90014_FIQ_N_REGS too big"
#endif

#ifndef __ASSEMBLY__
extern char fiq_vec[], fiq_vec_end[];
extern char fiq_debug[], fiq_debug_end[];
extern void mach_trap_init(unsigned long);
extern void cxd90014_fiq_init(void);
#endif /* !__ASSEMBLY__ */

#ifdef VMLINUX_SYMBOL
/* include from vmlinux.lds.S */
ASSERT((fiq_vec_end - fiq_vec <= MAX_FIQ_VEC), "FIQ VECTOR too big")
ASSERT((fiq_debug_end - fiq_debug <= MAX_FIQ_DEBUG), "FIQ DEBUG too big")
#endif
