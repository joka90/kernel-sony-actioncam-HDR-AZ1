/* 2011-03-01: File added and changed by Sony Corporation */
/*
 *  NBLArgs
 *
 *  Copyright 2002,2003,2013 Sony Corporation
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED	  ``AS	IS'' AND   ANY	EXPRESS OR IMPLIED
 *  WARRANTIES,	  INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO	EVENT  SHALL   THE AUTHOR  BE	 LIABLE FOR ANY	  DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED	  TO, PROCUREMENT OF  SUBSTITUTE GOODS	OR SERVICES; LOSS OF
 *  USE, DATA,	OR PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN	 CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/page-flags.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/string.h>
#include <asm/uaccess.h>

#include <linux/snsc_nblargs.h>

#define NAME "NBLArgs: "

#define DEBUG_LVL 1

#if DEBUG_LVL > 0
#  define printk1(args...) printk(args)
#else
#  define printk1(args...)
#endif

#if DEBUG_LVL > 1
#  define printk2(args...) printk(args)
#else
#  define printk2(args...)
#endif

#define ASSERT(x) if (!(x)) {printk(NAME "assertion ("#x ") failed at " __FILE__ ":%d\n", __LINE__);}

static struct nblargs_entry entry[NBLARGS_NUM_TABLE];
static int entry_cnt = 0;

static int    _parse_nblargs_memory(u_int32_t addr, int len);

int
nblargs_add(struct nblargs_entry *e, int cnt)
{
	int i;
	for (i=0; i<cnt; i++) {
		memcpy(entry+entry_cnt, e+i, sizeof(struct nblargs_entry));
		entry_cnt++;
	}
	return entry_cnt;
}

/*
 * _parse_nblargs_memory():
 *
 *   Parse memory region for NBLArgs.
 *
 * Returns:
 *  >=0 : A memory region that seems valid found.
 *        Return value is number of NBLArgs found.
 *   -1 : No valid NBLArgs was found.
 */
static int
_parse_nblargs_memory(u_int32_t addr, int len)
{
	struct nblargs       *pn;
	struct nblargs_entry *pe;

	printk1(NAME "searching for nblargs on memory(0x%x@0x%x).\n",
		addr, len);

	if (addr == 0)
		return -1;

	/* translate to virtual address */
	pn = (struct nblargs *)nbl_to_va(addr);

	/* check for MAGIC */
	if (pn->magic != NBLARGS_MAGIC) {
		printk1(NAME "No valid NBLArgs region found in 0x%08lx(0x%p)\n", nbl_to_pa(addr), pn);
		return -1;
	}
	printk1(NAME "Valid NBLArgs region found in 0x%08lx(0x%p)\n", nbl_to_pa(addr), pn);

	pe = nbl_to_va(pn->pHead);
	if (pe == nbl_to_va(pn->pNext))
		return 0;
	do {
		struct nblargs_entry na;
		if (pe->keylen != 0) {
			if (nblargs_get_key(pe->key, &na) == 0) {
				printk(NAME "Skippig duplicated nblargs key = %s\n", pe->key);
				goto next;
			}
			printk2("Key[%d]     = %s\n", entry_cnt, pe->key);
			printk2("KeyLen[%d]  = %d\n", entry_cnt, pe->keylen);
			printk2("Size[%d]    = %u bytes\n", entry_cnt, pe->size);
			printk2("Addr[%d]    = 0x%lx\n", entry_cnt, nbl_to_pa(pe->addr));
			printk2("Param[%d]   = %s\n", entry_cnt, pe->param);
			printk2("Paramlen[%d]= %d \n", entry_cnt, pe->paramlen);
			nblargs_add(pe, 1);
		}
	next:
		pe++;
	} while (pe != nbl_to_va(pn->pMax));

	return entry_cnt;
}

/*
 * int reserve_bootmem_common(int n)
 */
static
int reserve_bootmem_common(int n)
{
	struct nblargs_entry *e = entry+n;
	int err;

	if (!e->keylen)
		return -1;

#if DEBUG_LVL > 0
	printk1(NAME "%s(): key=%s, 0x%x@0x%lx, param=->%s<-\n",
		__FUNCTION__, e->key, e->size, nbl_to_pa(e->addr), e->param);
#endif

	if (e->size != 0) {
		err = reserve_bootmem(nbl_to_pa(e->addr), e->size,
				      BOOTMEM_EXCLUSIVE);
		if (err) {
			printk(KERN_ERR NAME
			       "cannot reserve memory at 0x%08lx size 0x%x "
			       "it is already reserved\n",
			       nbl_to_pa(e->addr), e->size);
			return err;
		}
	}

	return 0;
}


/*
 * Initialization routine of NBLArgs. (full memory version)
 *
 * (1) Search memory address and size provided in parameter.
 *
 * Returns:
 *   -. Number of NBLArgs entries stored in internal table.
 */
int __init
nblargs_init(void)
{
	int i;
	int retval;
	int len = CONFIG_SNSC_NBLARGS_STD_LEN;
#ifdef CONFIG_SNSC_NBLARGS_RESERVE_ENTRY_REGION
	int err;
#endif /* CONFIG_SNSC_NBLARGS_RESERVE_ENTRY_REGION */
	u_int32_t addr = CONFIG_SNSC_NBLARGS_STD_ADDR;

	/*
	 * if memory address is specified, search for it.
	 */
	retval = _parse_nblargs_memory(addr, len);

#ifdef CONFIG_SNSC_NBLARGS_RESERVE_ENTRY_REGION
	if (retval >= 0) {
		err = reserve_bootmem(nbl_to_pa(addr), len, BOOTMEM_EXCLUSIVE);
		if (err) {
			printk(KERN_ERR NAME "cannot reserve memory at "
			       "0x%08lx size 0x%x "
			       "it is already reserved\n",
			       nbl_to_pa(addr), len);
		} else {
			printk1(NAME "reserved entry region (0x%08lx@0x%x).\n",
				nbl_to_pa(addr), len);
		}
	}
#endif /* CONFIG_SNSC_NBLARGS_RESERVE_ENTRY_REGION */

	/*
	 * call reserve_bootmem
	 */
	for (i=0; i<retval; i++) {
		reserve_bootmem_common(i);
	}

	return retval;
}


/*
 * Arguments:
 *  *key : null-terminated key string.
 *  *p   : pointer to memory region
 *  *l   : length of *p in bytes
 *  *param: null-terminated list of parameters.
 *
 * Returns:
 *   0 : an entry matched given key, values returned in arguments.
 *  -1 : no entry matched given key.
 */
int
nblargs_get_key(char *key, struct nblargs_entry *na)
{
	int i;
	int retval = -1;

	ASSERT(key != NULL);
	ASSERT(na  != NULL);

	for (i=0; i<NBLARGS_NUM_TABLE; i++) {
		struct nblargs_entry *e = entry+i;

		if (!e->keylen)
			continue;

		if (!strncmp(key, e->key, NBLARGS_KEY_LEN)) {
			memcpy(na, e, sizeof(struct nblargs_entry));
			retval = 0;
			break;	/* found */
		}
	}

	return retval;
}
EXPORT_SYMBOL(nblargs_get_key);


void free_nblargs_mem(unsigned long start, unsigned long end)
{
	extern unsigned long totalram_pages;

	printk (NAME "%ldk freed\n", (end - start) >> 10);
	for (; start < end; start += PAGE_SIZE) {
		ClearPageReserved(virt_to_page((void*)start));
		init_page_count(virt_to_page((void*)start));
		free_page(start);
		totalram_pages++;
	}
}

/*
 * - frees reserved memory arer for 'key'.
 * - deletes nblargs entry by setting keylen to 0x0.
 */
void nblargs_free_key(char *key)
{
	int i;

	ASSERT(key != NULL);

	for (i=0; i<NBLARGS_NUM_TABLE; i++) {
		struct nblargs_entry *e = entry+i;

		if (!e->keylen)
			continue;

		if (!strncmp(key, e->key, NBLARGS_KEY_LEN)) {
			printk(NAME "freeing key=%s, 0x%x@0x%lx\n",
			       e->key, e->size, nbl_to_pa(e->addr));

			/* 0 means not valid entry*/
			e->keylen = 0;
			free_nblargs_mem((unsigned long)nbl_to_va(e->addr),
					 (unsigned long)nbl_to_va(e->addr) + e->size);
			break;
		}
	}
	return ;
}
EXPORT_SYMBOL(nblargs_free_key);

/*
 * This is for debug purpose
 */
int
nblargs_get_num(int num, char *key)
{
	if (num < NBLARGS_NUM_TABLE) {
		if (entry[num].keylen) {
			strncpy(key, entry[num].key, entry[num].keylen);
			return 0;
		}
	}
	return -1;
}


