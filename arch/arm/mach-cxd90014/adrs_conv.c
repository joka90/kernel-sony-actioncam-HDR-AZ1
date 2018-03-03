/*
 * arch/arm/mach-cxd90014/adrs_conv.c
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
#include <linux/init.h>
#include <asm/system.h>
#include <asm/memory.h>
#include <mach/noncache.h>

/* physical address */
#define ESRAM_BASE	UL(CXD90014_ESRAM_BASE)
#define ESRAM_SIZE	UL(CXD90014_ESRAM_SIZE)
#define ESRAM_END	(ESRAM_BASE + ESRAM_SIZE)

/* unified memory map */
#define UC_MIN	UL(DDR_BASE)
#define UC_MAX	ESRAM_END
#ifdef CONFIG_SUBSYSTEM
extern unsigned long subsys_virt, subsys_phys, subsys_size, subsys_offs;
#endif /* CONFIG_SUBSYSTEM */

/* separate memory map */
/* DDR uncacheable */
#define U_DDR_MIN	UL(DDR_BASE)
#define U_DDR_MAX	(U_DDR_MIN + DDR_SIZE)
/* DDR cacheable */
#ifdef CONFIG_CXD90014_DDR512MB
#define C_DDR_MIN	0x60000000UL
#else
#define C_DDR_MIN	0x40000000UL
#endif /* CONFIG_CXD90014_DDR512MB */
#define C_DDR_MAX	(C_DDR_MIN + DDR_SIZE)
#define DDR_OFFSET	(UC_MIN - C_DDR_MIN)
/* eSRAM uncacheable */
#define U_ESR_MIN	ESRAM_BASE
#define U_ESR_MAX	(U_ESR_MIN + ESRAM_SIZE)
/* eSRAM cacheable */
#define C_ESR_MIN	UC_MAX
#define C_ESR_MAX	(C_ESR_MIN + ESRAM_SIZE)
#define ESRAM_OFFSET	(C_ESR_MAX - UC_MAX)

static inline int pa_range_ok(unsigned long pa)
{
	return (DDR_BASE <= pa  &&  pa < ESRAM_END);
}

unsigned long arch_va_to_phys(unsigned long va)
{
#if CONFIG_PAGE_OFFSET == UC_MIN
	/* unified memory map */
	if (UC_MIN <= va  &&  va < UC_MAX) {
		return va;
	}
# ifdef CONFIG_SUBSYSTEM
	if (subsys_offs && subsys_virt <= va && va < subsys_virt+subsys_size) {
		return va - subsys_offs;
	}
# endif /* CONFIG_SUBSYSTEM */
#elif CONFIG_PAGE_OFFSET == C_DDR_MIN
	/* separate memory map */
	if (UC_MIN <= va  &&  va < UC_MAX) {
		/* DDR,eSRAM uncacheable */
		return va;
	} else if (C_DDR_MIN <= va  &&  va < C_DDR_MAX) {
		/* DDR cacheable */
		return (va + DDR_OFFSET);
	} else if (C_ESR_MIN <= va  &&  va < C_ESR_MAX) {
		/* eSRAM cacheable */
		return (va - ESRAM_OFFSET);
	}
#else
# error "Unsupported CONFIG_PAGE_OFFSET"
#endif

	return PA2NC_ERR;
}

unsigned long arch_va_to_noncache(unsigned long va)
{
	return arch_va_to_phys(va);
}

unsigned long arch_va_to_cache(unsigned long va)
{
#if CONFIG_PAGE_OFFSET == UC_MIN
	/* unified memory map */
	if (UC_MIN <= va  &&  va < UC_MAX) {
# ifdef CONFIG_SUBSYSTEM
		if (subsys_offs && subsys_phys <= va && va < subsys_phys+subsys_size) {
			return va + subsys_offs;
		}
# endif /* CONFIG_SUBSYSTEM */
		return va;
	}
#elif CONFIG_PAGE_OFFSET == C_DDR_MIN
	/* separate memory map */
	if (U_DDR_MIN <= va  &&  va < U_DDR_MAX) {
		/* DDR uncacheable */
		return (va - DDR_OFFSET);
	} else if (U_ESR_MIN <= va  &&  va < U_ESR_MAX) {
		/* eSRAM uncacheable */
		return (va + ESRAM_OFFSET);
	} else if (C_DDR_MIN <= va  &&  va < C_DDR_MAX) {
		/* DDR cacheable */
		return va;
	} else if (C_ESR_MIN <= va  &&  va < C_ESR_MAX) {
		/* eSRAM cacheable */
		return va;
	}
#else
# error "Unsupported CONFIG_PAGE_OFFSET"
#endif

	return PA2NC_ERR;
}

static inline unsigned long arch_any_to_noncache(unsigned long x)
{
	if (pa_range_ok(x))
		return x;
	return arch_va_to_noncache(x);
}

unsigned long arch_phys_to_noncache(unsigned long pa)
{
#ifdef CONFIG_CXD90014_ADRSCONV_RELAX
	return arch_any_to_noncache(pa);
#else
	return (pa_range_ok(pa)) ? pa : PA2NC_ERR;
#endif /* CONFIG_CXD90014_ADRSCONV_RELAX */
}

unsigned long arch_phys_to_cache(unsigned long pa)
{
	return (pa_range_ok(pa)) ? arch_va_to_cache(pa) : PA2NC_ERR;
	/* because phys == uncacheable */
}

EXPORT_SYMBOL(arch_phys_to_noncache);
EXPORT_SYMBOL(arch_phys_to_cache);
EXPORT_SYMBOL(arch_va_to_phys);
EXPORT_SYMBOL(arch_va_to_noncache);
EXPORT_SYMBOL(arch_va_to_cache);
