/*
 * noncache.h
 *
 * Physical address / Virtual address converter
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

#ifndef __MACH_CXD90014_INCLUDE_MACH_NONCACHE_H
#define __MACH_CXD90014_INCLUDE_MACH_NONCACHE_H
/*
 * NAME
 * 	PHYS_TO_NONCACHE, PHYS_TO_CACHE - Convert physical address to
 *	kernel address
 *
 * SYNOPSIS
 *	#include <mach/noncache.h>
 *	unsigned long PHYS_TO_NONCACHE(unsigned long);
 *	unsigned long PHYS_TO_CACHE(unsigned long);
 * INPUT
 *	Physical address of DDR, eSRAM.
 * RETURN VALUE
 *	PHYS_TO_NONCACHE: Noncacheable kernel address
 *	PHYS_TO_CACHE:    Cacheable kernel address
 *
 *	If input is out of range, PA2NC_ERR shall be returned.
 *
 * note
 *   Prefix PA2NC_ means Physical Address TO NonCacheable address.
 */
/*
 * NAME
 * 	VA_TO_PHYS - Convert virtual address to physical address
 *
 * SYNOPSIS
 *	#include <mach/noncache.h>
 *	unsigned long VA_TO_PHYS(unsigned long);
 * INPUT
 *	Kernel address of DDR, eSRAM.
 * RETURN VALUE
 *	Physical address
 *
 *	If input is out of range, PA2NC_ERR shall be returned.
 *
 */
/*
 * NAME
 * 	VA_TO_NONCACHE, VA_TO_CACHE - Convert virtual address to
 *      virtual address
 *
 * SYNOPSIS
 *	#include <mach/noncache.h>
 *	unsigned long VA_TO_NONCACHE(unsigned long);
 *	unsigned long VA_TO_CACHE(unsigned long);
 * INPUT
 *	Kernel address of DDR, eSRAM.
 * RETURN VALUE
 *	VA_TO_NONCACHE: Noncacheable kernel address
 *	VA_TO_CACHE:    Cacheable kernel address
 *
 *	If input is out of range, PA2NC_ERR shall be returned.
 *
 */

#define PHYS_TO_NONCACHE(x)	arch_phys_to_noncache(x)
#define PHYS_TO_CACHE(x)	arch_phys_to_cache(x)
#define VA_TO_PHYS(x)		arch_va_to_phys(x)
#define VA_TO_NONCACHE(x)	arch_va_to_noncache(x)
#define VA_TO_CACHE(x)		arch_va_to_cache(x)

#define PA2NC_ERR (0)

#ifdef __cplusplus
extern "C" {
#endif

	extern unsigned long arch_phys_to_noncache(unsigned long pa);
	extern unsigned long arch_phys_to_cache(unsigned long pa);
	extern unsigned long arch_va_to_phys(unsigned long va);
	extern unsigned long arch_va_to_noncache(unsigned long va);
	extern unsigned long arch_va_to_cache(unsigned long va);

#ifdef __cplusplus
}
#endif

#endif /* __MACH_CXD90014_INCLUDE_MACH_NONCACHE_H */
