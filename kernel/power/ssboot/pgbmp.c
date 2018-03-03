/* 2012-07-26: File added by Sony Corporation */
/*
 *  Snapshot Boot Core - page bitmap handling
 *
 *  Copyright 2008,2009,2010 Sony Corporation
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
#include <linux/slab.h>
#include <linux/pfn.h>
#include "internal.h"

static unsigned long
pgbmp_entry_size(ssboot_pgbmp_entry_t *entry)
{
	return BITS_TO_LONGS(entry->num_pages) * sizeof(unsigned long);
}

static int
pgbmp_entry_pfn_valid(ssboot_pgbmp_entry_t *entry, unsigned long pfn)
{
	if (pfn >= entry->start_pfn &&
	    pfn <  entry->start_pfn + entry->num_pages) {
		return 1;
	}
	return 0;
}

static int
pgbmp_entry_set(ssboot_pgbmp_entry_t *entry, unsigned long pfn)
{
	unsigned long *data;
	unsigned long pos, mask;

	pos  = pfn - entry->start_pfn;
	data = entry->bitmap + (pos / BITS_PER_LONG);
	mask = 1 << (pos & (BITS_PER_LONG - 1));
	if (!(*data & mask)) {
		*data |= mask;
		return 1;
	}
	return 0;
}

static int
pgbmp_entry_clear(ssboot_pgbmp_entry_t *entry, unsigned long pfn)
{
	unsigned long *data;
	unsigned long pos, mask;

	pos  = pfn - entry->start_pfn;
	data = entry->bitmap + (pos / BITS_PER_LONG);
	mask = 1 << (pos & (BITS_PER_LONG - 1));
	if (*data & mask) {
		*data &= ~mask;
		return 1;
	}
	return 0;
}

static void
pgbmp_entry_find(ssboot_pgbmp_entry_t *entry, unsigned long start_pfn,
		 unsigned long *pfn)
{
	unsigned long data;
	unsigned long off, pos = 0;
	int bit;

	/* find first set bit */
	start_pfn -= entry->start_pfn;
	for (pos = start_pfn / BITS_PER_LONG;
	     pos < entry->num_pages / BITS_PER_LONG; pos++) {
		data = *(entry->bitmap + pos);
		while ((bit = ffs(data))) {
			off = pos * BITS_PER_LONG + (bit - 1);
			if (off >= start_pfn) {
				*pfn = entry->start_pfn + off;
				return;
			}
			data &= ~(1 << (bit - 1));
		}
	}
	*pfn = SSBOOT_PFN_NONE;
}

unsigned long
ssboot_pgbmp_size(ssboot_pgbmp_t *pgbmp)
{
	ssboot_pgbmp_entry_t *entry;
	unsigned long size = 0;

	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		size += pgbmp_entry_size(entry);
	}
	return size;
}

ssboot_pgbmp_t*
ssboot_pgbmp_alloc(ssboot_memmap_t *memmap)
{
	ssboot_pgbmp_t *pgbmp;
	ssboot_pgbmp_entry_t *entry;
	int i;

	/* allocate page bitmap descriptor */
	pgbmp = kmalloc(sizeof(ssboot_pgbmp_t), GFP_KERNEL);
	if (pgbmp == NULL) {
		return NULL;
	}
	INIT_LIST_HEAD(&pgbmp->bmp_list);
	pgbmp->curr_pfn = SSBOOT_PFN_NONE;
	pgbmp->num_set = 0;

	/* allocate page bitmap */
	for (i = 0; i < memmap->num_region; i++) {
		entry = kmalloc(sizeof(ssboot_pgbmp_entry_t), GFP_KERNEL);
		if (entry == NULL) {
			goto err;
		}
		entry->start_pfn = (unsigned long)
				   PFN_DOWN(memmap->region[i].phys_addr);
		entry->num_pages = PFN_DOWN(memmap->region[i].len);
		entry->bitmap = kmalloc(pgbmp_entry_size(entry), GFP_KERNEL);
		if (entry->bitmap == NULL) {
			kfree(entry);
			goto err;
		}
		list_add_tail(&entry->list, &pgbmp->bmp_list);
	}
	return pgbmp;
 err:
	ssboot_pgbmp_free(pgbmp);
	return NULL;
}

void
ssboot_pgbmp_free(ssboot_pgbmp_t *pgbmp)
{
	ssboot_pgbmp_entry_t *entry, *temp;

	/* free page bitmap */
	list_for_each_entry_safe(entry, temp, &pgbmp->bmp_list, list) {
		list_del(&entry->list);
		kfree(entry->bitmap);
		kfree(entry);
	}

	/* free page bitmap descriptor */
	kfree(pgbmp);
}

void
ssboot_pgbmp_init(ssboot_pgbmp_t *pgbmp)
{
	ssboot_pgbmp_entry_t *entry;

	/* reset all bits */
	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		memset(entry->bitmap, 0, pgbmp_entry_size(entry));
	}

	/* reset counter */
	pgbmp->num_set = 0;
}

void
ssboot_pgbmp_set(ssboot_pgbmp_t *pgbmp, unsigned long pfn)
{
	ssboot_pgbmp_entry_t *entry;

	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		if (pgbmp_entry_pfn_valid(entry, pfn)) {
			if (pgbmp_entry_set(entry, pfn)) {
				pgbmp->num_set++;
			}
			return;
		}
	}
	BUG();
}

void
ssboot_pgbmp_clear(ssboot_pgbmp_t *pgbmp, unsigned long pfn)
{
	ssboot_pgbmp_entry_t *entry;

	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		if (pgbmp_entry_pfn_valid(entry, pfn)) {
			if (pgbmp_entry_clear(entry, pfn)) {
				pgbmp->num_set--;
			}
			return;
		}
	}
	BUG();
}

int
ssboot_pgbmp_test(ssboot_pgbmp_t *pgbmp, unsigned long pfn)
{
	ssboot_pgbmp_entry_t *entry;
	unsigned long *data;
	unsigned long mask;

	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		if (pgbmp_entry_pfn_valid(entry, pfn)) {
			pfn -= entry->start_pfn;
			data = entry->bitmap + (pfn / BITS_PER_LONG);
			mask = 1 << (pfn & (BITS_PER_LONG - 1));
			return (*data & mask) ? 1 : 0;
		}
	}
	return 0;
}

void
ssboot_pgbmp_set_region(ssboot_pgbmp_t *pgbmp, unsigned long pfn,
			unsigned long num)
{
	ssboot_pgbmp_entry_t *entry;
	unsigned long max, pos = pfn;

	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		if (pgbmp_entry_pfn_valid(entry, pos)) {
			max = entry->start_pfn + entry->num_pages - pos;
			max = num < max ? num : max;
			for (; max > 0; max--, num--, pos++) {
				if (pgbmp_entry_set(entry, pos)) {
					pgbmp->num_set++;
				}
			}
			if (num == 0) {
				return;
			}
		}
	}
	BUG();
}

void
ssboot_pgbmp_clear_region(ssboot_pgbmp_t *pgbmp, unsigned long pfn,
			  unsigned long num)
{
	ssboot_pgbmp_entry_t *entry;
	unsigned long max, pos = pfn;

	list_for_each_entry(entry, &pgbmp->bmp_list, list) {
		if (pgbmp_entry_pfn_valid(entry, pos)) {
			max = entry->start_pfn + entry->num_pages - pos;
			max = num < max ? num : max;
			for (; max > 0; max--, num--, pos++) {
				if (pgbmp_entry_clear(entry, pos)) {
					pgbmp->num_set--;
				}
			}
			if (num == 0) {
				return;
			}
		}
	}
	BUG();
}

void
ssboot_pgbmp_find_next(ssboot_pgbmp_t *pgbmp, unsigned long *pfn)
{
	ssboot_pgbmp_entry_t *entry = pgbmp->curr_entry;

	if (pgbmp->curr_pfn == SSBOOT_PFN_NONE) {
		*pfn = SSBOOT_PFN_NONE;
		return;
	}

	/* search in current entry */
	pgbmp_entry_find(entry, pgbmp->curr_pfn, pfn);
	if (*pfn != SSBOOT_PFN_NONE) {
		goto out;
	}

	/* search next entry */
	list_for_each_entry_continue(entry, &pgbmp->bmp_list, list) {
		pgbmp_entry_find(entry, entry->start_pfn, pfn);
		if (*pfn != SSBOOT_PFN_NONE) {
			goto out;
		}
	}
 out:
	pgbmp->curr_entry = entry;
	pgbmp->curr_pfn   = *pfn + 1;
}

void
ssboot_pgbmp_find_first(ssboot_pgbmp_t *pgbmp, unsigned long *pfn)
{
	ssboot_pgbmp_entry_t *entry;

	entry = list_first_entry(&pgbmp->bmp_list, ssboot_pgbmp_entry_t, list);
	pgbmp->curr_entry = entry;
	pgbmp->curr_pfn   = entry->start_pfn;

	ssboot_pgbmp_find_next(pgbmp, pfn);
}

unsigned long
ssboot_pgbmp_num_set(ssboot_pgbmp_t *pgbmp)
{
	return pgbmp->num_set;
}
