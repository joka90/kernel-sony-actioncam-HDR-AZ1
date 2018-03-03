/*
 * drivers/udif/malloc.c
 *
 * UDM
 *
 * Copyright 2012,2013 Sony Corporation
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
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/udif/types.h>
#include <linux/udif/malloc.h>
#include <linux/udif/print.h>
#include <linux/udif/proc.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/string.h>
#include <linux/udif/macros.h>

unsigned long udif_km_debug = 0;
module_param_named(debug, udif_km_debug, ulong, S_IRUSR);

struct udif_drivers_t {
	UDIF_SPINLOCK lock;
	struct list_head list;
};

static struct udif_drivers_t drivers = {
	.lock = UDIF_SPINLOCK_INIT(drivers.lock),
	.list = LIST_HEAD_INIT(drivers.list),
};

struct udif_kmalloc_t {
	char name[16];
	unsigned int size;
	unsigned long flags;
	struct list_head list;
	char user[0];
};

UDIF_VP __udif_kmalloc(const UDIF_U8 *name, UDIF_UINT size, UDIF_ULONG flags)
{
	struct udif_kmalloc_t *ent;
	unsigned long lflgs = 0;

	UDIF_PARM_CHK(!size, name, NULL);

	size += sizeof(struct udif_kmalloc_t);

	ent = kmalloc(size, flags);

	UDIF_PARM_CHK(!ent, name, NULL);

	__strncpy(ent->name, name, sizeof(ent->name));
	ent->size = size;
	ent->flags = flags;

	udif_spin_lock_irqsave(&drivers.lock, lflgs);
	list_add_tail(&ent->list, &drivers.list);
	udif_spin_unlock_irqrestore(&drivers.lock, lflgs);

	return ent->user;
}

UDIF_ERR __udif_kfree(UDIF_VP p)
{
	struct udif_kmalloc_t *ent;
	unsigned long flags = 0;

	UDIF_PARM_CHK(!p, "invalid pointer", UDIF_ERR_PAR);

	ent = container_of(p, struct udif_kmalloc_t, user);

	UDIF_PARM_CHK(!ent, "invalid entry", UDIF_ERR_PAR);

	udif_spin_lock_irqsave(&drivers.lock, flags);
	list_del(&ent->list);
	udif_spin_unlock_irqrestore(&drivers.lock, flags);

	kfree(ent);

	return UDIF_ERR_OK;
}

EXPORT_SYMBOL(__udif_kmalloc);
EXPORT_SYMBOL(__udif_kfree);
EXPORT_SYMBOL(udif_km_debug);

#define PROC_NAME "kmalloc"

static UDIF_INT udif_read_proc(UDIF_PROC_READ *proc)
{
	struct udif_drivers_t *drvs = udif_proc_getdata(proc);
	static struct udif_kmalloc_t *ent;
	unsigned long flags = 0;
	int len = 0;
	int done = 1;

	udif_spin_lock_irqsave(&drvs->lock, flags);

	if (udif_proc_pos(proc) == 0) {
		len += udif_proc_setbuf(proc, len, "strings\t\tsize\t\tflags\n");
		ent = list_first_entry(&drvs->list, struct udif_kmalloc_t, list);
	}

	list_for_each_entry_from(ent, &drvs->list, list) {
		int sz = strnlen(ent->name, 15) + 1 + 8 + 1 + 2 + 8 + 2;
					/*  %s   \t %08d  \t 0x %08lx \n\0 */

		if (len + sz > udif_proc_bufsize(proc)) {
			done = 0; /* buffer shortage, read at next request */
			break;
		}

		len += udif_proc_setbuf(proc, len, "%s\t%08d\t0x%08lx\n",
					ent->name,
					ent->size - sizeof(struct udif_kmalloc_t),
					ent->flags & ~KMALLOC_FLAGS);
	}

	udif_spin_unlock_irqrestore(&drvs->lock, flags);

	udif_proc_pos_inc(proc, len);

	if (done)
		udif_proc_setend(proc);

	return len;
}

static UDIF_PROC proc = {
	.name = PROC_NAME,
	.read = udif_read_proc,
	.data = &drivers
};

static int __init udif_kmalloc_init(void)
{
	if (udif_km_debug)
		return udif_create_proc(&proc);
	else
		return 0;
}

postcore_initcall(udif_kmalloc_init);

#if 0
static void __exit udif_kmalloc_exit(void)
{
	udif_remove_proc(&proc);
}

module_exit(udif_kmalloc_exit);
#endif
