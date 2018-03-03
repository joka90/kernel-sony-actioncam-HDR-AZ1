/* 2012-07-26: File added by Sony Corporation */
/*
 *  Snapshot Boot Core - extra region handling
 *
 *  Copyright 2008-2009 Sony Corporation
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
#include <linux/pfn.h>
#include <linux/slab.h>
#include <linux/list.h>
#include "internal.h"

/* status of list entry */
typedef enum ssboot_exreg_stat {
	SSBOOT_EXREG_STAT_FREE,
	SSBOOT_EXREG_STAT_STATIC,
	SSBOOT_EXREG_STAT_DYNAMIC
} ssboot_exreg_stat_t;

/* extra region descriptor */
typedef struct ssboot_exreg_t {
	struct list_head    list;
	unsigned long       start_pfn;
	unsigned long       num_pages;
	unsigned long       attr;
	ssboot_exreg_stat_t stat;
} ssboot_exreg_t;

/* static entries */
static ssboot_exreg_t exreg_static[SSBOOT_EXREG_NUM_STATIC];

/* extra region list */
static LIST_HEAD(exreg_list);
static struct list_head *exreg_pos;

static ssboot_exreg_t*
exreg_alloc_entry(void)
{
	ssboot_exreg_t *reg;
	int i;

	/* static allocation before kmem_cache_init() */
	for (i = 0; i < SSBOOT_EXREG_NUM_STATIC; i++) {
		reg = &exreg_static[i];
		if (reg->stat == SSBOOT_EXREG_STAT_FREE) {
			reg->stat = SSBOOT_EXREG_STAT_STATIC;
			return reg;
		}
	}

	/* dynamic allocation after kmem_cache_init() */
	if (slab_is_available()) {
		reg = (ssboot_exreg_t*)kmalloc(sizeof(ssboot_exreg_t),
					       GFP_ATOMIC);
		if (reg != NULL) {
			reg->stat = SSBOOT_EXREG_STAT_DYNAMIC;
			return reg;
		}
	}
	return NULL;
}

static void
exreg_free_entry(ssboot_exreg_t *reg)
{
	/* free entry for dynamic allocation */
	if (reg->stat == SSBOOT_EXREG_STAT_DYNAMIC) {
		kfree(reg);
		return;
	}

	/* free entry for static allocation */
	reg->stat = SSBOOT_EXREG_STAT_FREE;
}

static ssboot_exreg_t*
exreg_find_entry(unsigned long attr, unsigned long pfn, unsigned long num)
{
	ssboot_exreg_t *reg;

	/* find entry with specified attribute */
	list_for_each_entry(reg, &exreg_list, list) {
		if ((reg->attr & attr) == 0) {
			continue;
		}
		if (pfn == reg->start_pfn && num == reg->num_pages) {
			return reg;
		}
	}
	return NULL;
}

static int
exreg_check_overlap(unsigned long attr, unsigned long pfn, unsigned long num)
{
	ssboot_exreg_t *reg;
	unsigned long new_start, reg_start;
	unsigned long new_end, reg_end;
	unsigned long new_attr, reg_attr;

	/* new region */
	new_start = pfn;
	new_end   = pfn + num - 1;
	new_attr  = attr;

	list_for_each_entry(reg, &exreg_list, list) {
		/* registered region */
		reg_start = reg->start_pfn;
		reg_end   = reg->start_pfn + reg->num_pages - 1;
		reg_attr  = reg->attr;

		/* check exact match */
		if (new_attr  == reg_attr  &&
		    new_start == reg_start && new_end == reg_end) {
			return -EEXIST;
		}
		/* check allowed pattern */
		if ((SSBOOT_EXREG_TYPE(new_attr) == SSBOOT_EXREG_DISCARD ||
		     SSBOOT_EXREG_TYPE(reg_attr) == SSBOOT_EXREG_DISCARD) ||
		    (SSBOOT_EXREG_TYPE(new_attr) != SSBOOT_EXREG_WORK &&
		     SSBOOT_EXREG_TYPE(reg_attr) != SSBOOT_EXREG_WORK)) {
			continue;
		}
		/* check region overlap */
		if (!(new_end < reg_start || reg_end < new_start)) {
			return -EINVAL;
		}
	}
	return 0;
}

unsigned long
ssboot_exreg_find_next(unsigned long attr, unsigned long *pfn,
		       unsigned long *num)
{
	ssboot_exreg_t *reg;

	/* find next extra region */
	reg = list_entry(exreg_pos, ssboot_exreg_t, list);
	list_for_each_entry_continue(reg, &exreg_list, list) {
		if ((reg->attr & attr) == attr) {
			*pfn = reg->start_pfn;
			*num = reg->num_pages;
			exreg_pos = &reg->list;
			return reg->attr;
		}
	}

	/* not found */
	*pfn = SSBOOT_PFN_NONE;

	return 0;
}

unsigned long
ssboot_exreg_find_first(unsigned long attr, unsigned long *pfn,
			unsigned long *num)
{
	/* find first extra region */
	exreg_pos = &exreg_list;

	return ssboot_exreg_find_next(attr, pfn, num);
}

int
ssboot_exreg_register(unsigned long attr, unsigned long pfn,
		      unsigned long num)
{
	ssboot_exreg_t *reg;
	int ret;

	/* ignore zero size */
	if (num == 0) {
		return 0;
	}

	/* check context */
	switch (ssboot.state) {
	case SSBOOT_STATE_PREPARE:
		if (SSBOOT_EXREG_TYPE(attr) == SSBOOT_EXREG_NORMAL ||
		    SSBOOT_EXREG_TYPE(attr) == SSBOOT_EXREG_DISCARD) {
			break;
		}
		/* fall through */
	case SSBOOT_STATE_WRITING:
		ssboot_err("cannot register region (invalid context): "
			   "0x%08llx-0x%08llx\n",
			   (u_int64_t)PFN_PHYS(pfn),
			   (u_int64_t)PFN_PHYS(pfn + num) - 1);
		return -EBUSY;
	default:
		break;
	}

	/* check pfn range */
	if (!ssboot_pfn_valid_range(pfn, num)) {
		ssboot_err("cannot register region (invalid range): "
			   "0x%08llx-0x%08llx\n",
			   (u_int64_t)PFN_PHYS(pfn),
			   (u_int64_t)PFN_PHYS(pfn + num) - 1);
		return -EINVAL;
	}

	/* check region overlap */
	ret = exreg_check_overlap(attr, pfn, num);
	if (ret < 0) {
		/* ignore exact match */
		if (ret == -EEXIST) {
			return 0;
		}
		ssboot_err("cannot register region (overlapped): "
			   "0x%08llx-0x%08llx\n",
			   (u_int64_t)PFN_PHYS(pfn),
			   (u_int64_t)PFN_PHYS(pfn + num) - 1);
		return ret;
	}

	/* create new entry */
	reg = exreg_alloc_entry();
	if (reg == NULL) {
		return -ENOMEM;
	}
	reg->start_pfn = pfn;
	reg->num_pages = num;
	reg->attr = attr;

	/* add to extra region list */
	list_add_tail(&reg->list, &exreg_list);

	ssboot_dbg("registered region: 0x%08llx-0x%08llx (attr=0x%02lx)\n",
		   (u_int64_t)PFN_PHYS(pfn),
		   (u_int64_t)PFN_PHYS(pfn + num) - 1, attr);

	return 0;
}

int
ssboot_exreg_unregister(unsigned long attr, unsigned long pfn,
			unsigned long num)
{
	ssboot_exreg_t *reg;
	int cnt = 0;

	/* ignore zero size */
	if (num == 0) {
		return 0;
	}

	for (;;) {
		/* find entry in extra region list */
		reg = exreg_find_entry(attr, pfn, num);
		if (reg == NULL) {
			break;
		}

		/* remove from extra region list */
		list_del(&reg->list);

		ssboot_dbg("unregistered region: 0x%08llx-0x%08llx "
			   "(attr=0x%02lx)\n",
			   (u_int64_t)PFN_PHYS(pfn),
			   (u_int64_t)PFN_PHYS(pfn + num) - 1,
			   reg->attr);

		/* free entry */
		exreg_free_entry(reg);

		cnt++;
	}

	/* check if no entry is freed */
	if (cnt == 0) {
		ssboot_err("cannot unregister region (unknown): "
			   "0x%08llx-0x%08llx\n",
			   (u_int64_t)PFN_PHYS(pfn),
			   (u_int64_t)PFN_PHYS(pfn + num) - 1);
		return -ENOENT;
	}
	return 0;
}
