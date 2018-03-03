/* 2009-06-02: File added and changed by Sony Corporation */
/*
 *  Page Accounting
 */
/*
 * Copyright 2007-2009 Sony Corporation.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 * WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include	<linux/seq_file.h>
#include	<linux/kallsyms.h>
#include 	<linux/kernel.h>
#include 	<linux/spinlock.h>
#include	<linux/cache.h>

struct kma_caller {
	const void *caller;
	int total, net, slack, allocs, frees;
};

struct kma_list {
	int callerhash;
	const void *address;
};

#define MAX_CALLER_TABLE 128
#define MAX_ALLOC_TRACK 8192

#define kma_hash(address, size) (((u32)address / (u32)size) % size)

static struct kma_list kma_alloc[MAX_ALLOC_TRACK];
static struct kma_caller kma_caller[MAX_CALLER_TABLE];

static int kma_callers;
static int kma_lost_callers, kma_lost_allocs, kma_unknown_frees;
static int kma_total, kma_net, kma_slack, kma_allocs, kma_frees;
static DEFINE_SPINLOCK(kma_lock);
static unsigned long flags;
extern int is_free_initmem_running;

#ifdef CONFIG_SNSC_PAGES_ACCOUNTING_DEBUG
int n_pageacct_actual_alloc = 0;
#endif

int return_addr_hash(unsigned long addr)
{
	int i;

	for (i = 0 ; i < MAX_ALLOC_TRACK; i++)
		if (addr == (unsigned long) kma_alloc[i].address)
			return i;
	return -EINVAL;
}

void pages_account_remove_caller(unsigned long addr)
{
	int hasha, hashc;

	hasha = return_addr_hash(addr);
	hashc = kma_alloc[hasha].callerhash;

	spin_lock_irqsave(&kma_lock, flags);
	memset(&kma_caller[hashc], 0, sizeof(struct kma_caller));
	memset(&kma_alloc[hasha], 0, sizeof(struct kma_list));
	spin_unlock_irqrestore(&kma_lock, flags);
}

void pages_account(const void *caller, const void *addr, int size, int req)
{
	int i, hasha, hashc;
	extern int mem_init_state;

	if (!mem_init_state)
		return;
	if (is_free_initmem_running)
		return;

	spin_lock_irqsave(&kma_lock, flags);
	if(req >= 0) /* kmalloc */
	{
		/* find callers slot */
		hashc = kma_hash(caller, MAX_CALLER_TABLE);
		/* don't allow use of the 0th hash slot */
		/* because kma_alloc[i]->callerhash == 0 means empty entry */
		if (hashc == 0) {
			hashc++;
		}
		for (i = 0; i < MAX_CALLER_TABLE; i++) {
			if (!kma_caller[hashc].caller ||
			    kma_caller[hashc].caller == caller)
				break;
			hashc = (hashc + 1) % MAX_CALLER_TABLE;
			if (hashc == 0) {
				hashc++;
			}
		}

		if (!kma_caller[hashc].caller)
			kma_callers++;

		if (i < MAX_CALLER_TABLE) {
			/* update callers stats */
			kma_caller[hashc].caller = caller;
			kma_caller[hashc].total += size;
			kma_caller[hashc].net += size;
			kma_caller[hashc].slack += size - req;
			kma_caller[hashc].allocs++;

			/* add malloc to list */
			hasha = kma_hash(addr, MAX_ALLOC_TRACK);
			for (i = 0; i < MAX_ALLOC_TRACK; i++) {
				if (!kma_alloc[hasha].callerhash)
					break;
				hasha = (hasha + 1) % MAX_ALLOC_TRACK;
			}

			if(i < MAX_ALLOC_TRACK) {
				kma_alloc[hasha].callerhash = hashc;
				kma_alloc[hasha].address = addr;
			}
			else
				kma_lost_allocs++;
		}
		else {
			kma_lost_callers++;
			kma_lost_allocs++;
		}

		kma_total += size;
		kma_net += size;
		kma_slack += size - req;
		kma_allocs++;
	}
	else { /* kfree */
		hasha = kma_hash(addr, MAX_ALLOC_TRACK);
		for (i = 0; i < MAX_ALLOC_TRACK ; i++) {
			if (kma_alloc[hasha].address == addr)
				break;
			hasha = (hasha + 1) % MAX_ALLOC_TRACK;
		}

		if (i < MAX_ALLOC_TRACK) {
			hashc = kma_alloc[hasha].callerhash;
			kma_alloc[hasha].callerhash = 0;
			kma_caller[hashc].net -= size;
			kma_caller[hashc].frees++;
		}
		else {
#ifdef CONFIG_SNSC_PAGES_ACCOUNTING_DEBUG
			char *modname;
			const char *name;
			unsigned long offset = 0, s;
			char namebuf[128];

			name = kallsyms_lookup((u32)caller, &s, &offset, &modname, namebuf);
			printk(KERN_INFO "pageacct: %s: %8p %d\n", name, addr, size);
#endif
			kma_unknown_frees++;
		}

		kma_net -= size;
		kma_frees++;
	}
	spin_unlock_irqrestore(&kma_lock, flags);
}

static void *as_start(struct seq_file *m, loff_t *pos)
{
	int i;
	loff_t n = *pos;

	if (!n) {
		seq_printf(m, "total bytes allocated: %8d\n", kma_total);
		seq_printf(m, "slack bytes allocated: %8d\n", kma_slack);
		seq_printf(m, "net bytes allocated:   %8d\n", kma_net);
		seq_printf(m, "number of allocs:      %8d\n", kma_allocs);
#ifdef CONFIG_SNSC_PAGES_ACCOUNTING_DEBUG
		/*
		 * Number of allocs and actual allocs should be equal, or
		 * else there are bugs in pageacct.
		 */
		seq_printf(m, "number of actual allocs: %8d\n", n_pageacct_actual_alloc);
#endif
		seq_printf(m, "number of frees:       %8d\n", kma_frees);
		seq_printf(m, "number of callers:     %8d\n", kma_callers);
		seq_printf(m, "lost callers:          %8d\n",
			   kma_lost_callers);
		seq_printf(m, "lost allocs:           %8d\n",
			   kma_lost_allocs);
		seq_printf(m, "unknown frees:         %8d\n",
			   kma_unknown_frees);
		seq_puts(m, "\n   total    slack      net alloc/free  caller\n");
	}

	for (i = 0; i < MAX_CALLER_TABLE; i++) {
		if(kma_caller[i].caller)
			n--;
		if(n < 0)
			return (void *)(i+1);
	}

	return 0;
}

static void *as_next(struct seq_file *m, void *p, loff_t *pos)
{
	int n = (int)p-1, i;
	++*pos;

	for (i = n + 1; i < MAX_CALLER_TABLE; i++)
		if(kma_caller[i].caller)
			return (void *)(i+1);

	return 0;
}

static void as_stop(struct seq_file *m, void *p)
{
}

static int as_show(struct seq_file *m, void *p)
{
	int n = (int)p-1;
	struct kma_caller *c;
#ifdef CONFIG_KALLSYMS
	char *modname;
	const char *name;
	unsigned long offset = 0, size;
	char namebuf[128];

	c = &kma_caller[n];
	name = kallsyms_lookup((int)c->caller, &size, &offset, &modname,
			       namebuf);
	seq_printf(m, "%8d %8d %8d %5d/%-5d %s+0x%lx\n",
		   c->total, c->slack, c->net, c->allocs, c->frees,
		   name, offset);
#else
	c = &kma_caller[n];
	seq_printf(m, "%8d %8d %8d %5d/%-5d %p\n",
		   c->total, c->slack, c->net, c->allocs, c->frees, c->caller);
#endif

	return 0;
}

struct seq_operations pages_account_op = {
	.start	= as_start,
	.next	= as_next,
	.stop	= as_stop,
	.show	= as_show,
};

