/*
 *  exception.c  - Exception Monitor: entered when exception occured.
 *
 * Copyright 2009,2010 Sony Corporation
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
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/console.h>
#include <linux/exception_monitor.h>
#include <linux/exception.h>
#include <linux/em_export.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define EM_CONSOLE_DELAY 400 /* msec */

#define EM_CONFIG_USERMODE_CALLBACK
#define EM_CONFIG_ATOMIC_CALLBACK

#define CONSOLE_IRQ   0		/* UART0 */
#define CONSOLE_PORT  0

#define SCNPRINTF(buf, size, fmt, ...) (((size) > 0) ? scnprintf(buf, size, fmt, ## __VA_ARGS__) : 0)

static int oom_exit;
module_param_named(oom_exit, oom_exit, int, S_IRUSR|S_IWUSR);
static int segv_exit;
module_param_named(segv_exit, segv_exit, int, S_IRUSR|S_IWUSR);
static int em_reboot_flag;
module_param_named(reboot, em_reboot_flag, int, S_IRUSR|S_IWUSR);

#define ALIGN4(x) (((x) + 0x3) & 0xfffffffc)

/*
 * MONITOR_MODE:
 * 1: don't show function name at callstack dump
 * 2: show function name at callstack dump
 * 3: show function name at callstack dump
 */
static int monitor_mode = 2;
struct pt_regs *em_regs;
int not_interrupt = 1;

static struct callstack *callstack;

/*
 * for dump byte/word/long
 */
static unsigned char *dump_point = (unsigned char *)0x10010000;
static int dump_size = 0x100;

extern int console_read(const unsigned char *buf, int count);
extern int console_write(const unsigned char *buf, int count);

#define LOG_BUF_SZ  128
static char log_buf[LOG_BUF_SZ];

#define WRITE_BUF_SZ  512

struct em_callback_node {
	struct list_head list;
	em_callback_t fun;
	void *arg;
};

#ifdef EM_CONFIG_USERMODE_CALLBACK
static DECLARE_RWSEM(em_usermode_callback_sem);
static LIST_HEAD(em_usermode_callback_head);

void *em_register_usermode_callback(em_callback_t fun, void *arg)
{
	struct em_callback_node *node;

	node = (struct em_callback_node *)kmalloc(sizeof(*node),
						  GFP_KERNEL);
	if (!node)
		return NULL;
	node->fun = fun;
	node->arg = arg;
	down_write(&em_usermode_callback_sem);
	list_add(&node->list, &em_usermode_callback_head);
	up_write(&em_usermode_callback_sem);
	return node;
}
EXPORT_SYMBOL(em_register_usermode_callback);

void em_unregister_usermode_callback(void *handle)
{
	if (!handle)
		return;
	down_write(&em_usermode_callback_sem);
	list_del(handle);
	up_write(&em_usermode_callback_sem);
	kfree(handle);
}
EXPORT_SYMBOL(em_unregister_usermode_callback);

#ifndef CONFIG_SUBSYSTEM
static void em_call_usermode_callback(struct pt_regs *regs)
{
	struct em_callback_node *node;

	down_read(&em_usermode_callback_sem);
	list_for_each_entry(node, &em_usermode_callback_head, list) {
		node->fun(regs, node->arg);
	}
	up_read(&em_usermode_callback_sem);
}
#endif /* CONFIG_SUBSYSTEM */
#else /* EM_CONFIG_USERMODE_CALLBACK */
inline static void em_call_usermode_callback(struct pt_regs *regs) {}
#endif /* EM_CONFIG_USERMODE_CALLBACK */

#ifdef EM_CONFIG_ATOMIC_CALLBACK
static DEFINE_RAW_SPINLOCK(em_atomic_callback_lock);
static LIST_HEAD(em_atomic_callback_head);

void *em_register_atomic_callback(em_callback_t fun, void *arg)
{
	struct em_callback_node *node;
	unsigned long flags;

	node = (struct em_callback_node *)kmalloc(sizeof(*node),
						  GFP_KERNEL);
	if (!node)
		return NULL;
	node->fun = fun;
	node->arg = arg;
	raw_spin_lock_irqsave(&em_atomic_callback_lock, flags);
	list_add(&node->list, &em_atomic_callback_head);
	raw_spin_unlock_irqrestore(&em_atomic_callback_lock, flags);
	return node;
}
EXPORT_SYMBOL(em_register_atomic_callback);

void em_unregister_atomic_callback(void *handle)
{
	unsigned long flags;

	if (!handle)
		return;
	raw_spin_lock_irqsave(&em_atomic_callback_lock, flags);
	list_del(handle);
	raw_spin_unlock_irqrestore(&em_atomic_callback_lock, flags);
	kfree(handle);
}
EXPORT_SYMBOL(em_unregister_atomic_callback);

#ifndef CONFIG_SUBSYSTEM
static void em_call_atomic_callback(struct pt_regs *regs)
{
	struct em_callback_node *node;
	unsigned long flags;

	raw_spin_lock_irqsave(&em_atomic_callback_lock, flags);
	list_for_each_entry(node, &em_atomic_callback_head, list) {
		node->fun(regs, node->arg);
	}
	raw_spin_unlock_irqrestore(&em_atomic_callback_lock, flags);
}
#endif /* CONFIG_SUBSYSTEM */
#else  /* EM_CONFIG_ATOMIC_CALLBACK */
inline static void em_call_atomic_callback(struct pt_regs *regs) {}
#endif /* EM_CONFIG_ATOMIC_CALLBACK */

void em_dump_write(const char *format, ...)
{
	static char buf[WRITE_BUF_SZ];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, WRITE_BUF_SZ, format, args);
	va_end(args);
	buf[WRITE_BUF_SZ - 1] = '\0';

	console_write(buf, strlen(buf));
}

char *em_get_execname(void)
{
	char *name, *envp;
	char *name_base, *envp_base;

	if (current->mm == NULL) {
		return NULL;
	}
	if (current->mm->arg_start == 0) {
		return NULL;
	}
	name = (unsigned char *)current->mm->arg_start;
	envp = (unsigned char *)current->mm->env_start;
	while ((unsigned long)envp < current->mm->env_end) {
		if (strncmp(envp, "_=", 2) == 0) {
			name_base = (unsigned char *)name + strlen(name);
			while ((name_base >= name) && (*name_base != '/'))
				name_base--;
			name_base++;
			envp_base = (unsigned char *)envp + strlen(envp);
			while ((envp_base >= envp) && (*envp_base != '/'))
				envp_base--;
			envp_base++;
			if (strcmp(name_base, envp_base) == 0) {
				name = envp + 2;
			}
			break;
		}
		envp += strlen(envp) + 1;
	}
	/* if argv is NULL */
	if ((unsigned long)name == current->mm->arg_end) {
		struct vm_area_struct *vm = NULL;
		for (vm = current->mm->mmap; vm != NULL; vm = vm->vm_next) {
#ifdef CONFIG_PPC
			if (vm->vm_start != 0x10000000)
				continue;
#endif
#ifdef CONFIG_ARM
			if (vm->vm_start != 0x00008000)
				continue;
#endif
			if (vm->vm_file == NULL)
				continue;
			if (vm->vm_file->f_dentry)
				name = (char *)
					vm->vm_file->f_dentry->d_name.name;
		}
	}
	return name;
}

static char em_convert_char(unsigned long c)
{
	if (((c & 0xff) < 0x20) || ((c & 0xff) > 0x7e))
		return '.';
	else
		return c & 0xff;
}

static void em_dump_current_task(int argc, char **argv)
{
	em_dump_write("\n[current task]\n");
	em_dump_write("program: %s(%s) (pid: %d, cpu:%d, tid: %d, stackpage: 0x%08x)\n",
		      em_get_execname(), current->comm, current->pid,
		      raw_smp_processor_id(),
#ifdef CONFIG_SUBSYSTEM
	              current->itron_tid,
#else
		      -1,
#endif /* CONFIG_SUBSYSTEM */
		      (unsigned int)current);
#ifdef CONFIG_PPC
	em_dump_write("last syscall: %ld\n", current->thread.last_syscall);
#endif
#ifdef CONFIG_ARM
	em_dump_write("address: %08x, trap_no: %08x, error_code: %08x\n",
		      current->thread.address,
		      current->thread.trap_no,
		      current->thread.error_code);
#endif
}

#ifdef CONFIG_MODULES
static void em_dump_modules(int argc, char **argv)
{
#ifdef MODULE
	struct module *mod_head = THIS_MODULE;
#else
	struct module *mod_head = __module_address(MODULES_VADDR);
#endif
	struct module *mod = mod_head;

	em_dump_write("\n[modules]\n");

	em_dump_write("%10s %8s   %s\n", "Address", "Size", "Module");

	if (!mod)
		return;

	while((unsigned long)mod >= MODULES_VADDR && (unsigned long)mod <= MODULES_END) {
		em_dump_write("0x%8p %8lu   %s [%8p]\n",
			      mod->module_core,
			      mod->init_size + mod->core_size,
			      mod->name,
			      mod);

		mod = list_entry(mod->list.next, struct module, list);
	}

	mod = list_entry(mod_head->list.prev, struct module, list);
	while((unsigned long)mod >= MODULES_VADDR && (unsigned long)mod <= MODULES_END) {
		em_dump_write("0x%8p %8lu   %s [%8p]\n",
			      mod->module_core,
			      mod->init_size + mod->core_size,
			      mod->name,
			      mod);

		mod = list_entry(mod->list.prev, struct module, list);
	}
}
#endif /* CONFIG_MODULES */

static void em_dump_system_maps(int argc, char **argv)
{
	unsigned long page_size = 0x00001000;
	struct vm_area_struct *vm;

	em_dump_write("\n[system maps]\n");

	if (!current || !current->mm || !current->mm->mmap) {
		em_dump_write("current->mm->mmap is NULL");
		return;
	}

	em_dump_write("start    end      flg offset     name\n");

	for (vm = current->mm->mmap; vm; vm = vm->vm_next) {
		em_dump_write("%08x-%08x ", vm->vm_start, vm->vm_end);
		em_dump_write("%c", (vm->vm_flags & VM_READ) ? 'r' : '-');
		em_dump_write("%c", (vm->vm_flags & VM_WRITE) ? 'w' : '-');
		em_dump_write("%c ", (vm->vm_flags & VM_EXEC) ? 'x' : '-');

		em_dump_write("0x%08x ", vm->vm_pgoff * page_size);

		if (vm->vm_file && vm->vm_file->f_dentry &&
		    vm->vm_file->f_dentry->d_name.name)
			em_dump_write("%s\n",
				      vm->vm_file->f_dentry->d_name.name);
		else
			em_dump_write("\n");
	}
}

static void em_dump_callstack(int argc, char **argv)
{
	int i = 0;
	struct _elf_info *elfinfo ;
	struct callstack *cs;

	cs = &callstack[i];
	elfinfo = cs->elf_info;

	em_dump_write("\n[call stack]\n");

	if (callstack[0].caller == 0x0) {
		em_dump_write("Could not get callstack info\n");
		return;
	}

	switch (monitor_mode) {
	case 1:
		em_dump_write
		    ("    function entry : 0x%08x [ factor : 0x%08x ]\n",
		     cs->entry, cs->caller);
		for (i = 1; callstack[i].caller != 0x0; i++) {
			cs = &callstack[i];
			em_dump_write
			    (" <- function entry : 0x%08x [ caller : 0x%08x ]\n",
			     cs->entry, cs->caller);
		}
		break;

	case 2:
	case 3:
		em_dump_write("########### FP callstack #################\n");
		em_dump_write("    function entry : 0x%08x "
			      "[ factor : 0x%08x ] -- %s @ %s : 0x%lx\n",
			      cs->entry,
			      cs->caller,
			      cs->entry_str,
			      elfinfo?elfinfo->filename:"NULL",
			      cs->caller - (elfinfo?elfinfo->addr_offset:0));
		for (i = 1; callstack[i].caller != CALLER_END; i++) {
			cs = &callstack[i];
			elfinfo = cs->elf_info;

			if (cs->caller == CALLER_SP) {
				em_dump_write("########### SP callstack #################\n");
				continue;
			}

			if (cs->caller == CALLER_USER) {
				em_dump_write("\n########### FP user callstack ############\n");
				continue;
			}
			em_dump_write(" <- function entry : 0x%08x "
				      "[ caller : 0x%08x ] -- %s @ %s : 0x%lx\n",
				      cs->entry,
				      cs->caller,
				      cs->entry_str,
				      elfinfo?elfinfo->filename:"NULL",
				      cs->caller - (elfinfo?elfinfo->addr_offset:0));
		}
		break;

	default:
		em_dump_write("invalid mode for exception monitor\n");
		break;
	}

	em_dump_write("\n");
}

static void em_dump_byte(int argc, char **argv)
{
	int i;
	char buf[17];
	int n = 0;
	unsigned long insn;
	unsigned char c = 0;
	unsigned char *point = (unsigned char *)dump_point;
	int size = dump_size;

	switch (argc) {
	case 3:
		if ((argv[2][0] == '0') && (toupper(argv[2][1]) == 'X')) {
			argv[2] = &argv[2][2];
		}
		size = simple_strtoul(argv[2], NULL, 16);
	case 2:
		if ((argv[1][0] == '0') && (toupper(argv[1][1]) == 'X')) {
			argv[1] = &argv[1][2];
		}
		point = (unsigned char *)simple_strtoul(argv[1], NULL, 16);
		break;
	case 1:
		break;
	default:
		return;
	}

	buf[16] = 0;
	while (n < size) {
		em_dump_write("%08x :", point);
		for (i = 0; i < 16; i++) {
			if (n < size) {
				if (__get_user(insn, point)) {
					em_dump_write(" (Bad data address)\n");
					return;
				}
				c = *point++;
				buf[i] = em_convert_char(c);
				em_dump_write(" %02x", c);
				n++;
			} else {
				buf[i] = ' ';
				em_dump_write("   ");
			}
		}
		em_dump_write(" : %s\n", buf);
	}
	dump_point = point;
	dump_size = size;
}

static void em_dump_word(int argc, char **argv)
{
	int i;
	char buf[17];
	int n = 0;
	unsigned long insn;
	unsigned short c = 0;
	unsigned short *point = (unsigned short *)dump_point;
	int size = dump_size;

	switch (argc) {
	case 3:
		if ((argv[2][0] == '0') && (toupper(argv[2][1]) == 'X')) {
			argv[2] = &argv[2][2];
		}
		size = simple_strtoul(argv[2], NULL, 16);
	case 2:
		if ((argv[1][0] == '0') && (toupper(argv[1][1]) == 'X')) {
			argv[1] = &argv[1][2];
		}
		point = (unsigned short *)simple_strtoul(argv[1], NULL, 16);
		break;
	case 1:
		break;
	default:
		return;
	}

	buf[16] = 0;
	while (n < (size / 2)) {
		em_dump_write("%08x :", point);
		for (i = 0; i < 8; i++) {
			if (n < size) {
				if (__get_user(insn, point)) {
					em_dump_write(" (Bad data address)\n");
					return;
				}
				c = *point++;
				buf[i * 2] = em_convert_char(c >> 8);
				buf[i * 2 + 1] = em_convert_char(c);
				em_dump_write(" %04x", c);
				n++;
			} else {
				buf[i] = ' ';
				em_dump_write("   ");
			}
		}
		em_dump_write(" : %s\n", buf);
	}
	dump_point = (unsigned char *)point;
	dump_size = size;
}

static void em_dump_long(int argc, char **argv)
{
	int i;
	char buf[17];
	int n = 0;
	unsigned long insn;
	unsigned long c = 0;
	unsigned long *point = (unsigned long *)dump_point;
	int size = dump_size;

	switch (argc) {
	case 3:
		if ((argv[2][0] == '0') && (toupper(argv[2][1]) == 'X')) {
			argv[2] = &argv[2][2];
		}
		size = simple_strtoul(argv[2], NULL, 16);
	case 2:
		if ((argv[1][0] == '0') && (toupper(argv[1][1]) == 'X')) {
			argv[1] = &argv[1][2];
		}
		point = (unsigned long *)simple_strtoul(argv[1], NULL, 16);
		break;
	case 1:
		break;
	default:
		return;
	}

	buf[16] = 0;
	while (n < (size / 4)) {
		em_dump_write("%08x :", point);
		for (i = 0; i < 4; i++) {
			if (n < size) {
				if (__get_user(insn, point)) {
					em_dump_write(" (Bad data address)\n");
					return;
				}
				c = *point++;
				buf[i * 4] = em_convert_char(c >> 24);
				buf[i * 4 + 1] = em_convert_char(c >> 16);
				buf[i * 4 + 2] = em_convert_char(c >> 8);
				buf[i * 4 + 3] = em_convert_char(c);
				em_dump_write(" %08x", c);
				n++;
			} else {
				buf[i] = ' ';
				em_dump_write("   ");
			}
		}
		em_dump_write(" : %s\n", buf);
	}
	dump_point = (unsigned char *)point;
	dump_size = size;
}

static void em_write_byte(int argc, char **argv)
{
	char buf[17];
	unsigned char datum;
	unsigned char *point;
	int i;
	unsigned long insn;
	unsigned char c = 0;

	switch (argc) {
	case 3:
		if ((argv[2][0] == '0') && (toupper(argv[2][1]) == 'X')) {
			argv[2] = &argv[2][2];
		}
		datum = (unsigned char)simple_strtoul(argv[2], NULL, 16);
		point = (unsigned char *)simple_strtoul(argv[1], NULL, 16);
		break;
	case 2:
	case 1:
	default:
		return;
	}

	if (copy_to_user(point, &datum, sizeof(*point))) {
		em_dump_write(" (Bad data address)\n");
		return;
	}

	em_dump_write("%08x: ", point);
	for (i = 0; i < 16; i++) {
		if (__get_user(insn, point)) {
			em_dump_write(" (Bad data address)\n");
			return;
		}
		c = *point++;
		buf[i] = em_convert_char(c);
		em_dump_write(" %02x", c);
	}
	buf[16] = 0;
	em_dump_write(" : %s\n", buf);
}

static void em_write_word(int argc, char **argv)
{
	char buf[17];
	unsigned short datum;
	unsigned short *point;
	int i;
	unsigned long insn;
	unsigned short c = 0;

	switch (argc) {
	case 3:
		if ((argv[2][0] == '0') && (toupper(argv[2][1]) == 'X')) {
			argv[2] = &argv[2][2];
		}
		datum = (unsigned short)simple_strtoul(argv[2], NULL, 16);
		point = (unsigned short *)simple_strtoul(argv[1], NULL, 16);
		break;
	case 2:
	case 1:
	default:
		return;
	}

	if (copy_to_user(point, &datum, sizeof(*point))) {
		em_dump_write(" (Bad data address)\n");
		return;
	}

	em_dump_write("%08x: ", point);
	for (i = 0; i < 8; i++) {
		if (__get_user(insn, point)) {
			em_dump_write(" (Bad data address)\n");
			return;
		}
		c = *point++;
		buf[i * 2] = em_convert_char(c >> 8);
		buf[i * 2 + 1] = em_convert_char(c);
		em_dump_write(" %04x", c);
	}
	buf[16] = 0;
	em_dump_write(" : %s\n", buf);
}

static void em_write_long(int argc, char **argv)
{
	char buf[17];
	unsigned long datum;
	unsigned long *point;
	int i;
	unsigned long insn;
	unsigned long c = 0;

	switch (argc) {
	case 3:
		if ((argv[2][0] == '0') && (toupper(argv[2][1]) == 'X')) {
			argv[2] = &argv[2][2];
		}
		datum = (unsigned long)simple_strtoul(argv[2], NULL, 16);
		point = (unsigned long *)simple_strtoul(argv[1], NULL, 16);
		break;
	case 2:
	case 1:
	default:
		return;
	}

	if (copy_to_user(point, &datum, sizeof(*point))) {
		em_dump_write(" (Bad data address)\n");
		return;
	}

	em_dump_write("%08x: ", point);
	for (i = 0; i < 4; i++) {
		if (__get_user(insn, point)) {
			em_dump_write(" (Bad data address)\n");
			return;
		}
		c = *point++;
		buf[i * 4] = em_convert_char(c >> 24);
		buf[i * 4 + 1] = em_convert_char(c >> 16);
		buf[i * 4 + 2] = em_convert_char(c >> 8);
		buf[i * 4 + 3] = em_convert_char(c);
		em_dump_write(" %08x", c);
	}
	buf[16] = 0;
	em_dump_write(" : %s\n", buf);
}

static void em_dump_exception(int argc, char **argv)
{
	unsigned long *point = (unsigned long *)instruction_pointer(em_regs);

	point -= 8; /* point at 8 word before */
	em_dump_write
	    ("==============================================================================");
	em_dump_regs(1, NULL);
	em_dump_stack(1, NULL);
	em_dump_callstack(1, NULL);
#ifdef CONFIG_MODULES
	em_dump_modules(1, NULL);
#endif /* CONFIG_MODULES */
	em_dump_system_maps(1, NULL);
	em_dump_current_task(1, NULL);
	em_show_syndrome();
	em_dump_write
	    ("==============================================================================\n");
}

#ifdef EM_UM_SHELL
static void em_user_shell(int argc, char **argv)
{
	char *arg[] = {
		"/bin/sh",
		NULL
	};
	char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin:/usr/bin:/devel/sbin:/devel/bin:/devel/usr/bin:/devel/usr/sbin",
		NULL
	};
	int error;
	if((error=call_usermodehelper_ex("/bin/sh", arg, envp, 1, 1)))
		em_dump_write("call_usermodehelper error: %d!\n", error);
}
#else
static void em_user_shell(int argc, char **argv)
{
	em_dump_write("shell is disabled!\n");
	em_dump_write("please set EM_DEBUG to yes during compilation:\n");
	em_dump_write("   eg. EM_DEBUG=y make\n");
}
#endif

void __attribute__((weak)) em_reboot(int force)
{
	machine_restart(NULL);
}

void em_cond_reboot(void)
{
	if (em_reboot_flag) {
		em_reboot(0);
	}
}

static void em_cmd_reboot(int argc, char **argv)
{
	em_reboot(1);
}

struct command {
	char name[32];
	void (*func) (int, char **);
};

static void em_help(int argc, char **argv)
{
	em_dump_write("\n[Exception monitor commands]\n");
	em_dump_write(" show                       : show exception message\n");
	em_dump_write(" reg                        : show registers\n");
	em_dump_write(" stack                      : stack dump\n");
	em_dump_write(" call                       : call stack dump\n");
	em_dump_write(" map                        : show memory map\n");
#ifdef CONFIG_MODULES
	em_dump_write(" modules                    : show modules\n");
#endif /* CONFIG_MODULES */
	em_dump_write(" task                       : show current task info\n");
	em_dump_write(" d[b] [<addr>] [<size>]     : dump byte-access\n");
	em_dump_write(" dw [<addr>] [<size>]       : dump word-access\n");
	em_dump_write(" dl [<addr>] [<size>]       : dump long-access\n");
	em_dump_write(" w[b] <addr> <value>        : write byte-access\n");
	em_dump_write(" ww <addr> <value>          : write word-access\n");
	em_dump_write(" wl <addr> <value>          : write long-access\n");
	em_dump_write(" shell                      : enter shell(/bin/sh)\n");
	em_dump_write(" v2p <addr>                 : convert <addr> to physical address\n");
	em_dump_write(" help                       : show this message\n");
	em_dump_write(" reboot                     : reboot\n");
	em_dump_write(" exit                       : exit exception monitor\n\n");

}

static const struct command command[] = {
	{"show", &em_dump_exception},
	{"reg", &em_dump_regs},
	{"regd", &em_dump_regs_detail},
	{"stack", &em_dump_stack},
	{"call", &em_dump_callstack},
	{"map", &em_dump_system_maps},
#ifdef CONFIG_MODULES
	{"modules", &em_dump_modules},
#endif /* CONFIG_MODULES */
	{"task", &em_dump_current_task},
	{"d", &em_dump_byte},
	{"db", &em_dump_byte},
	{"dw", &em_dump_word},
	{"dl", &em_dump_long},
	{"w", &em_write_byte},
	{"wb", &em_write_byte},
	{"ww", &em_write_word},
	{"wl", &em_write_long},
	{"shell", &em_user_shell},
	{"reboot", &em_cmd_reboot},
	{"v2p", &em_cmd_v2p},
	{"help", &em_help}
};

static int em_execute_command(char *buf)
{
	int i;
	char *argv[8];
	int argc;
	int word = 0;

	argc = 0;
	argv[0] = NULL;
	for (i = 0; i < LOG_BUF_SZ; i++) {
		if (buf[i] == '\0')
			break;
		if (buf[i] == ' ') {
			buf[i] = '\0';
			word = 0;
			continue;
		}
		if (word == 0) {
			argv[argc++] = &buf[i];
			word = 1;
		}
	}

	for (i = 0; i < sizeof(command) / sizeof(*command); i++) {
		if (strncmp(buf, command[i].name, LOG_BUF_SZ) == 0) {
			(*command[i].func) (argc, argv);
			return 0;
		}
	}

	return -1;
}

static void em_disable_irq(void)
{
}
static void em_enable_irq(void)
{
}

void (*em_hook)(struct pt_regs *regs) = NULL;
int em_use_console;

static DEFINE_RAW_SPINLOCK(em_lock);

void em_exception_monitor(int mode, struct pt_regs *registers)
{
	mm_segment_t fs = get_fs();
	char *buf;
        int irq_disabled = 0;
	int p_count = 0;
	unsigned long flags;

#ifdef CONFIG_THREAD_MONITOR
	trace_cpu = 0;
#endif
	raw_spin_lock_irqsave(&em_lock, flags);

	if (!exception_check) {
		raw_spin_unlock_irqrestore(&em_lock, flags);
		return ;
	}
	exception_check = NULL;
	em_use_console = 1;

	raw_spin_unlock_irqrestore(&em_lock, flags);

	/*
	 * We already set em_use_console flag, but there is a possibility of
	 * that another CPU is executing call_console_drivers() now.
	 * So, we should wait for certain minutes.
	 */
	mdelay(EM_CONSOLE_DELAY);

	set_fs(KERNEL_DS);
#ifdef ARCH_HAS_SET_IOPRIV
	set_iopriv(KERNEL_DS);
#endif /* ARCH_HAS_SET_IOPRIV */
	/* disable file parsing in NFS environement */
#ifndef CONFIG_ROOT_NFS
	if (irqs_disabled()) {
		not_interrupt = 0;
	} else {
		not_interrupt = 1;
	}
#else
	not_interrupt = 0;
#endif

	if ((mode > 0) && (mode < 4)) {
		monitor_mode = mode;
	} else {
		monitor_mode = 3;
	}

#ifndef CONFIG_EJ_LOG_ALWAYS_DUMPABLE
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	if (current->mm && !current->mm->dumpable) {
#else
	if (current->mm && !(current->mm->flags & 0x3)) {
#endif
 			em_dump_write
 			    ("exception monitor: log not dumpable, return.\n");
 			goto end2;
	}
#endif /* CONFIG_EJ_LOG_ALWAYS_DUMPABLE */

	/*
	 * group_stop_count is non_zero if do_coredump() is called as
	 * a result of sending signal (not as a result of a CPU
	 * exception). In which case we should explicitly clear
	 * group_stop_count and TIF_SIGPENDING in order to access
	 * files, as do_coredump() does before coredumping.
	 */
	if (user_mode(registers)) {
		spin_lock_irqsave(&current->sighand->siglock, flags);
		current->signal->group_stop_count = 0;
		clear_thread_flag(TIF_SIGPENDING);
		spin_unlock_irqrestore(&current->sighand->siglock, flags);
	}

#ifndef CONFIG_SUBSYSTEM
	em_dump_write("calling callbacks\n");

	if (user_mode(registers))
		em_call_usermode_callback(registers);
	em_call_atomic_callback(registers);
#endif

	if (oom_exit && test_tsk_thread_flag(current, TIF_MEMDIE)) {
		em_dump_write("OMM Killer disabled -  Quit!\n");
		if (em_reboot_flag) {
			em_reboot(0);
		}
		goto end2;
	}

#if 0
	/*
	 * Flush serial buffer first
	 */
	if (!in_interrupt()) {
		acquire_console_sem();
		release_console_sem();
	}
#endif

	/*
	 * Disable interrupt requests
	 */
	em_disable_irq();

	/*
	 * Do some initialization stuff
	 */
	if (registers != NULL) {
		em_regs = registers;
	} else {
		em_dump_write("pt_regs is NULL\nreturn\n");
		goto end;
	}

	if (em_hook) {
		(*em_hook)(registers);
	}
	if (em_reboot_flag) {
		em_reboot(0);
	}

	callstack = em_get_callstack();

	em_dump_exception(0, NULL);

	em_dump_write("\n\nEntering exception monitor.\n");
	em_dump_write("Type `help' to show commands.\n");
	em_dump_write("Type `exit' to exit.\n\n");

	if (segv_exit && user_mode(em_regs) && !test_tsk_thread_flag(current, TIF_MEMDIE)) {
		em_dump_write("SEGV disabled -  Quit!\n");
		goto end;
	}

	preempt_disable();
	while (1) {
		em_dump_write("Exception> ");
		while (console_read(log_buf, LOG_BUF_SZ) < 0) ;
		buf = log_buf;

		if (buf == NULL)
			continue;
		if (strcmp(buf, "exit") == 0)
			break;

		if ((buf[0] != '\0') && (em_execute_command(buf) == -1)) {
			em_dump_write("%s: Command not found.\n", buf);
		}
	}
	em_dump_write("\nGood Bye.\n");
	preempt_enable();

 end:
	/*
	 * Enable interrupt requests
	 */
	em_enable_irq();

 end2:
	em_use_console = 0;
	wmb();
	exception_check = em_exception_monitor;
	if(irq_disabled) {
		local_irq_disable();
		add_preempt_count(p_count);
	}
	set_fs(fs);
#ifdef ARCH_HAS_SET_IOPRIV
	set_iopriv(fs);
#endif /* ARCH_HAS_SET_IOPRIV */
}


static int __init em_module_init(void)
{
	exception_check = em_exception_monitor;
	return 0;
}

static void __exit em_module_exit(void)
{
	exception_check = NULL;
}

module_init(em_module_init);
module_exit(em_module_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
