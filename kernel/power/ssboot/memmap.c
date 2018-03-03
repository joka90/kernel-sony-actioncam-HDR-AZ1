/* 2012-07-26: File added by Sony Corporation */
/*
 *  Snapshot Boot Core - memory map handling
 *
 *  Copyright 2010 Sony Corporation
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
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/pfn.h>
#include "internal.h"

static unsigned long
memmap_pfn_valid(unsigned long pfn)
{
	ssboot_memmap_t *memmap = &ssboot.memmap;
	unsigned long start, end;
	int i;

	/* allow any PFN if not initialized */
	if (memmap->num_region == 0) {
		return -1UL;
	}

	/* check current memory map */
	for (i = 0; i < memmap->num_region; i++) {
		start = PFN_DOWN(memmap->region[i].phys_addr);
		end   = start + PFN_DOWN(memmap->region[i].len) - 1;
		if (pfn >= start && pfn <= end) {
			return end - pfn + 1;
		}
	}
	return 0;
}

/*
 * Exported functions
 */
int
ssboot_pfn_valid(unsigned long pfn)
{
	return memmap_pfn_valid(pfn) ? 1 : 0;
}
EXPORT_SYMBOL(ssboot_pfn_valid);

int
ssboot_pfn_valid_range(unsigned long pfn, unsigned long num)
{
	unsigned long ret;

	while (num > 0) {
		ret = memmap_pfn_valid(pfn);
		if (ret == 0) {
			return 0;
		} else if (ret > num) {
			break;
		}
		num -= ret;
		pfn += ret;
	}
	return 1;
}
EXPORT_SYMBOL(ssboot_pfn_valid_range);

void*
ssboot_pfn_to_virt(unsigned long pfn)
{
	ssboot_memmap_t *memmap = &ssboot.memmap;
	unsigned long start, end, virt;
	int i;

	for (i = 0; i < memmap->num_region; i++) {
		start = PFN_DOWN(memmap->region[i].phys_addr);
		end   = start + PFN_DOWN(memmap->region[i].len) - 1;
		if (pfn >= start && pfn <= end) {
			virt = (unsigned long)((u_int64_t)PFN_PHYS(pfn) -
					       memmap->region[i].phys_addr +
					       memmap->region[i].virt_addr);
			return (void *)virt;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(ssboot_pfn_to_virt);

unsigned long
ssboot_virt_to_pfn(void *virt_addr)
{
	ssboot_memmap_t *memmap = &ssboot.memmap;
	unsigned long start, end, virt, pfn;
	int i;

	virt = (unsigned long)virt_addr;
	for (i = 0; i < memmap->num_region; i++) {
		start = memmap->region[i].virt_addr;
		end   = start + memmap->region[i].len - 1;
		if (virt >= start && virt <= end) {
			pfn = PFN_DOWN((u_int64_t)virt -
				       memmap->region[i].virt_addr +
				       memmap->region[i].phys_addr);
			return pfn;
		}
	}
	return SSBOOT_PFN_NONE;
}
EXPORT_SYMBOL(ssboot_virt_to_pfn);

int
ssboot_memmap_register(u_int64_t phys_addr, void *virt_addr, size_t len)
{
	ssboot_memmap_t *memmap = &ssboot.memmap;
	unsigned long new_vstart, new_vend;
	unsigned long old_vstart, old_vend;
	u_int64_t new_pstart, new_pend;
	u_int64_t old_pstart, old_pend;
	int idx, i;

	/* check zero size */
	if (len == 0) {
		return 0;
	}

	/* check alignment */
	if ((phys_addr | (unsigned long)virt_addr | len) & ~PAGE_MASK) {
		ssboot_err("cannot register memory map (invalid align): "
			   "0x%08x @ 0x%08llx\n",
			   len, phys_addr);
		return -EINVAL;
	}

	/* check free entry */
	idx = memmap->num_region;
	if (idx >= SSBOOT_MEMMAP_NUM) {
		ssboot_err("cannot register memory map (no space): "
			   "0x%08x @ 0x%08llx\n",
			   len, phys_addr);
		return -ENOMEM;
	}

	/* check overlap */
	new_vstart = (unsigned long)virt_addr;
	new_vend   = new_vstart + len - 1;
	new_pstart = phys_addr;
	new_pend   = new_pstart + len - 1;
	for (i = 0; i < idx; i++) {
		old_vstart = memmap->region[i].virt_addr;
		old_vend   = old_vstart + memmap->region[i].len - 1;
		old_pstart = memmap->region[i].phys_addr;
		old_pend   = old_pstart + memmap->region[i].len - 1;
		if (!(new_vend < old_vstart || old_vend < new_vstart) ||
		    !(new_pend < old_pstart || old_pend < new_pstart)) {
			ssboot_err("cannot register memory map (overlapped): "
				   "0x%08x @ 0x%08llx\n",
				   len, phys_addr);
			return -EINVAL;
		}
	}

	/* register new memory map */
	memmap->region[idx].virt_addr = (unsigned long)virt_addr;
	memmap->region[idx].phys_addr = phys_addr;
	memmap->region[idx].len       = len;
	memmap->num_region++;

	ssboot_dbg("memory map: 0x%08x @ 0x%08llx (0x%08lx)\n",
		   len, phys_addr, (unsigned long)virt_addr);

	return 0;
}
EXPORT_SYMBOL(ssboot_memmap_register);

/*
 * Initialization
 */
int __init
ssboot_memmap_init(void)
{
	u_int64_t phys;
	void *virt;
	size_t len;
	int ret;

	/* all lowmem region */
	phys = (u_int64_t)PFN_PHYS(min_low_pfn) + PHYS_OFFSET;
	virt = phys_to_virt(phys);
	len  = PFN_PHYS(max_low_pfn - min_low_pfn);

	/* register lowmem region */
	ret = ssboot_memmap_register(phys, virt, len);
	if (ret < 0) {
		ssboot_err("cannot register initial memory map: %d\n", ret);
		return ret;
	}
	return 0;
}
