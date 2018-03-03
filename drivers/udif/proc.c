/*
 * drivers/udif/proc.c
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
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/udif/proc.h>
#include <linux/udif/macros.h>

#define DIR_NAME "udm"

#define UDIF_DECLARE_PROC_READ(name, b, c, p, e, d) \
struct UDIF_PROC_READ name = { \
	.kaddr	= b, \
	.count	= c, \
	.pos	= p, \
	.eof	= e, \
	.data	= d, \
}

#define UDIF_DECLARE_PROC_WRITE(name, b, c, d) \
struct UDIF_PROC_WRITE name = { \
	.uaddr	= b, \
	.count	= c, \
	.data	= d, \
}

static struct proc_dir_entry *udif_proc_dir = NULL;

static int udif_read_proc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	off_t pos = off;
	UDIF_PROC *proc = data;
	int ret = 0;

	if (likely(proc) && proc->read) {
		UDIF_DECLARE_PROC_READ(rd, page, count, &pos, eof, proc->data);

		ret = proc->read(&rd);

		*start = (char *)(pos - off);
		if (unlikely(*start >= page)) {
			UDIF_PERR("too large pos = 0x%08lx, off = 0x%08lx, (page = 0x%p)\n",
				   pos, off, page);
			*start = NULL;
			ret = -EFAULT;
			*eof = 1;
		}
	}

	return ret;
}

static int udif_write_proc(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{
	UDIF_PROC *proc = data;
	int ret = count;

	if (likely(proc) && proc->write) {
		UDIF_DECLARE_PROC_WRITE(wr, buffer, count, proc->data);
		ret = proc->write(&wr);
	}

	return ret;
}

UDIF_ERR udif_create_proc(UDIF_PROC *proc)
{
	struct proc_dir_entry *ent;

	UDIF_PARM_CHK(!proc, "proc is NULL", UDIF_ERR_PAR);

	if (unlikely(!udif_proc_dir)) {
		UDIF_PERR("not initialize udif_proc_dir\n");
		return UDIF_ERR_IO;
	}

	ent = create_proc_entry(proc->name, S_IRUGO, udif_proc_dir);
	if (unlikely(!ent)) {
		UDIF_PERR("cannot create proc entry %s\n", proc->name);
		return UDIF_ERR_NOMEM;
	}

	ent->read_proc = udif_read_proc;
	ent->write_proc = udif_write_proc;
	ent->data = proc;

	return UDIF_ERR_OK;
}

UDIF_ERR udif_remove_proc(UDIF_PROC *proc)
{
	UDIF_PARM_CHK(!proc, "proc is NULL", UDIF_ERR_PAR);

	if (unlikely(!udif_proc_dir)) {
		UDIF_PERR("not initialize udif_proc_dir\n");
		return UDIF_ERR_IO;
	}

	remove_proc_entry(proc->name, udif_proc_dir);

	return UDIF_ERR_OK;
}

EXPORT_SYMBOL(udif_create_proc);
EXPORT_SYMBOL(udif_remove_proc);

static int __init udif_init_proc(void)
{
	udif_proc_dir = proc_mkdir(DIR_NAME, NULL);
	if (unlikely(!udif_proc_dir)) {
		UDIF_PERR("cannot proc_mkdir\n");
		return -ENOMEM;
	}

	return 0;
}

postcore_initcall(udif_init_proc);

#if 0
static void __exit udif_exit_proc(void)
{
	remove_proc_entry(DIR_NAME, NULL);
	udif_proc_dir = NULL;
}
module_exit(udif_exit_proc);
#endif
