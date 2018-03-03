/*  exception-arm.c  - arm specific part of Exception Monitor
 *
 * Copyright 2009 Sony Corporation
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
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/exception.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

#define EM_SP_CALLSTACK

#ifdef DEBUG
#define dbg(fmt, argv...) em_dump_write(fmt, ##argv)
#else
#define dbg(fmt, argv...) do{}while(0)
#endif

#define ARRAY_NUM(x) (sizeof(x)/sizeof((x)[0]))

extern struct pt_regs *em_regs;
extern int not_interrupt;

/*
 * indicate if fp supported
 */
static int fp_valid;
/*
 * for callstack
 */
#define LIB_MAX 100
#define LIB_NAME_SIZE 64
static char libname[LIB_MAX][LIB_NAME_SIZE];

//#define ELF_INFO_MAX (1 + ARRAY_NUM(target_module) + LIB_MAX)
#define ELF_INFO_MAX (LIB_MAX + 2)
static struct _elf_info elf_info[ELF_INFO_MAX];


#define MAX_CALLSTACK    100
struct callstack callstack[MAX_CALLSTACK + 1];


static void em_get_entry_name(struct file *fp, Elf32_Off offset, char *str)
{
	fp->f_op->llseek(fp, offset, 0);
	fp->f_op->read(fp, str, CALLSTACK_STR_SZ, &fp->f_pos);
	str[CALLSTACK_STR_SZ] = '\0';
}

static int
em_get_entry_near(unsigned int index, Elf32_Shdr * sym, Elf32_Shdr * str,
		  struct callstack *cs)
{
	int n, j;
	struct file *fp;
	Elf32_Sym ent;
	Elf32_Off offset = 0x0;
	Elf32_Addr value = 0x0;
	Elf32_Word name = 0x0;
	unsigned long addr_base;
	int flag = 0;

	fp = elf_info[index].file;
	n = sym->sh_size / sizeof(Elf32_Sym);
	addr_base = (index) ? elf_info[index].addr_offset : 0;

	if (fp->f_op->llseek(fp, sym->sh_offset, 0) < 0)
		return 0;
	for (j = 0; j < n; j++) {
		if (fp->f_op->llseek(fp, sym->sh_offset + j*sizeof(Elf32_Sym), 0) < 0)
			break;
		if (fp->f_op->read(fp, (char *)&ent, sizeof(Elf32_Sym), &fp->f_pos) != sizeof(Elf32_Sym))
			break;
		if ((ELF32_ST_TYPE(ent.st_info) == STT_FUNC) &&
		    ((ent.st_value + addr_base) <= cs->caller) &&
		    ((ent.st_value + addr_base) >= value)) {
			cs->elf_info = & elf_info[index];
			cs->entry = ent.st_value + addr_base;
			cs->size = ent.st_size;

			offset = str->sh_offset;
			name = ent.st_name;
			em_get_entry_name(fp, offset + name, cs->entry_str);
#ifndef EM_SP_CALLSTACK
			if(cs->entry_str[0] != '$'){
#else
			if(1){
#endif
				value = ent.st_value + addr_base;
				flag = 1;
			}
		}
	}

	if (!flag) {
		return 0;
	}

	return 1;
}

static void em_find_symbol_near(int elf_cnt, struct callstack *cs)
{
	int i;
	int len ;

	if (cs->caller == 0x0)
		return;

	for (i = 0; i < elf_cnt; i++) {
		if (elf_info[i].file == NULL)
			continue;

		if (elf_info[i].addr_offset > cs->caller ||
		    elf_info[i].addr_end < cs->caller) {
			continue;
		}
#ifndef EM_SP_CALLSTACK
		if (elf_info[i].strip) {
			strncpy(cs->entry_str, "## Stripped", sizeof(cs->entry_str)/sizeof(cs->entry_str[0]) - 1);
			cs->entry_str[sizeof(cs->entry_str)/sizeof(cs->entry_str[0]) - 1]='\0';
			return;
		}
#endif
		if (em_get_entry_near
		    (i, &elf_info[i].sh_symtab, &elf_info[i].sh_strtab, cs)) {
			goto found;
		}

		if (em_get_entry_near
		    (i, &elf_info[i].sh_dynsym, &elf_info[i].sh_dynstr, cs)) {
			goto found;
		}

	}

	/* cs->entry = 0x0; */
	strncpy(cs->entry_str, "## Unknown", sizeof(cs->entry_str)/sizeof(cs->entry_str[0]) - 1);
	cs->entry_str[sizeof(cs->entry_str)/sizeof(cs->entry_str[0]) - 1] = '\0';
	return;

      found:
	len = CALLSTACK_STR_SZ - strlen(cs->entry_str);
	if (fp_valid)
		strncat(cs->entry_str, "  ## GUESS", len);
}

static int
em_get_entry(unsigned int index, Elf32_Shdr * sym, Elf32_Shdr * str,
	     struct callstack *cs)
{
	int n, j;
	struct file *fp;
	Elf32_Sym ent;
	Elf32_Off offset = 0x0;
	Elf32_Word name = 0x0;
	unsigned long addr_base;

	fp = elf_info[index].file;
	n = sym->sh_size / sizeof(Elf32_Sym);
	addr_base = (index) ? elf_info[index].addr_offset : 0;

	if (fp->f_op->llseek(fp, sym->sh_offset, 0) < 0)
		return 0;
	for (j = 0; j < n; j++) {
		if (fp->f_op->llseek(fp, sym->sh_offset + j*sizeof(Elf32_Sym), 0) < 0)
			break;
		if (fp->f_op->read(fp, (char *)&ent, sizeof(Elf32_Sym), &fp->f_pos) != sizeof(Elf32_Sym))
			break;

		if (ELF32_ST_TYPE(ent.st_info) == STT_FUNC) {
			if((ent.st_value + addr_base) == cs->entry){
				offset = str->sh_offset;
				name = ent.st_name;
				em_get_entry_name(fp, offset + name, cs->entry_str);
				if(cs->entry_str[0] != '$') {
					cs->elf_info = &elf_info[index];
					goto found;
				}
			}
		}
	}
	return 0;

      found:
	return 1;
}

static int get_kernel_entry(unsigned long pc, struct callstack *cs);
static void em_get_entry_str(int elf_cnt, struct callstack *cs)
{
	int i;

	if (cs->entry == 0x0)
		goto fuzzy;

	if(get_kernel_entry(cs->entry, cs))
		return;

	for (i = 0; i < elf_cnt; i++) {
		if (elf_info[i].file == NULL)
			continue;

		if (elf_info[i].addr_offset > cs->caller ||
		    elf_info[i].addr_end < cs->caller) {
			continue;
		}

		if (elf_info[i].addr_offset > cs->entry ||
		    elf_info[i].addr_end < cs->entry) {
			continue;
		}

		if (em_get_entry
		    (i, &elf_info[i].sh_symtab, &elf_info[i].sh_strtab, cs)) {
			return;
		}
		if (em_get_entry
		    (i, &elf_info[i].sh_dynsym, &elf_info[i].sh_dynstr, cs)) {
			return;
		}
	}

fuzzy:
	/* did not find entry */
	fp_valid = 0;
	em_find_symbol_near(elf_cnt, cs);
	return;
}

static void em_close_kernel_nm(void)
{
}

static void em_open_kernel_nm(void)
{
}

static int get_kernel_entry(unsigned long pc, struct callstack *cs)
{
	return 0;
}

static void em_open_modules(int *elf_cnt)
{
	*elf_cnt = 1;
}

static void em_close_elffile(unsigned int index)
{
	if (elf_info[index].file) {
		filp_close(elf_info[index].file, NULL);
		elf_info[index].file = NULL;
	}
}

static void em_close_elffiles(int elf_cnt)
{
	int i;

	for (i = 0; i < elf_cnt; i++) {
		em_close_elffile(i);
	}

	em_close_kernel_nm();
}

static int em_open_elffile(unsigned int index)
{
	int i;
	int strip = 2;
	Elf32_Ehdr ehdr;
	Elf32_Shdr shdr;
	Elf32_Shdr sh_shstrtab;
	char *shstrtab;
	struct file *fp;

	/*
	 * open elf file
	 */
	elf_info[index].file =
	    filp_open(elf_info[index].filename, O_RDONLY, 0444);

	if (IS_ERR(elf_info[index].file)) {
		elf_info[index].file = NULL;
		goto fail;
	}
	fp = elf_info[index].file;

	if (!fp->f_op || !fp->f_op->llseek || !fp->f_op->read)
		goto close_fail;

	/*
	 * read elf header
	 */
	fp->f_op->read(fp, (char *)&ehdr, sizeof(Elf32_Ehdr), &fp->f_pos);
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0)
		goto close_fail;
	if (!elf_check_arch(&ehdr))
		goto close_fail;

	/*
	 * read section header table
	 */
	fp->f_op->llseek(fp,
			 ehdr.e_shoff + sizeof(Elf32_Shdr) * ehdr.e_shstrndx,
			 0);
	fp->f_op->read(fp, (char *)&sh_shstrtab, sizeof(Elf32_Shdr),
		       &fp->f_pos);

	shstrtab = (char *)kmalloc(sh_shstrtab.sh_size, GFP_ATOMIC);
	if(shstrtab == NULL){
		goto fail;
	}

	fp->f_op->llseek(fp, sh_shstrtab.sh_offset, 0);
	fp->f_op->read(fp, shstrtab, sh_shstrtab.sh_size, &fp->f_pos);

	/*
	 * read shsymtab
	 */
	fp->f_op->llseek(fp, ehdr.e_shoff, 0);
	for (i = 0; i < ehdr.e_shnum; i++) {
		fp->f_op->read(fp, (char *)&shdr, sizeof(Elf32_Shdr),
			       &fp->f_pos);
		if (strcmp(&shstrtab[shdr.sh_name], ".dynsym") == 0)
			elf_info[index].sh_dynsym = shdr;
		else if (strcmp(&shstrtab[shdr.sh_name], ".dynstr") == 0)
			elf_info[index].sh_dynstr = shdr;
		else if (strcmp(&shstrtab[shdr.sh_name], ".symtab") == 0) {
			elf_info[index].sh_symtab = shdr;
			strip--;
		} else if (strcmp(&shstrtab[shdr.sh_name], ".strtab") == 0) {
			elf_info[index].sh_strtab = shdr;
			strip--;
		}
	}

	if (!strip)
		elf_info[index].strip = strip;

	kfree(shstrtab);
	return 0;

      close_fail:
	em_close_elffile(index);
      fail:
	return -1;
}

static void init_struct_callstack(void)
{
	int i;

	for (i = 0; i < MAX_CALLSTACK; i++) {
		callstack[i].entry = 0x0;
		callstack[i].caller = 0x0;
		callstack[i].size = 0x0;
		callstack[i].elf_info = 0x0;
		callstack[i].entry_str[0] = '\0';
	}
}

static void init_struct_elfinfo(void)
{
	int i;

	for (i = 0; i < ELF_INFO_MAX; i++) {
		elf_info[i].filename = 0;
		elf_info[i].sh_dynsym.sh_size = 0;
		elf_info[i].sh_dynstr.sh_size = 0;
		elf_info[i].sh_symtab.sh_size = 0;
		elf_info[i].sh_strtab.sh_size = 0;
		elf_info[i].addr_offset = 0;
		elf_info[i].addr_end = 0;
		elf_info[i].strip = 1;
	}

}

static int em_open_elffiles(void)
{
	char *path;
	static char buf[LIB_NAME_SIZE];
	struct vm_area_struct *vm = NULL;
	char *short_name;
	int elf_cnt = 0;
	int i;

	/*
	 * initialize
	 */
	init_struct_callstack();
	init_struct_elfinfo();

	if (!not_interrupt)
		goto out;

	if (test_tsk_thread_flag(current, TIF_MEMDIE))
		goto out;

	em_open_kernel_nm();

	/*
	 * set elf_info
	 */
	elf_info[0].filename = em_get_execname();
	short_name = elf_info[0].filename;
	for (i = 0; elf_info[0].filename[i]; i++)
		if (elf_info[0].filename[i] == '/')
			short_name = &elf_info[0].filename[i + 1];
	if (current->mm != NULL)
		vm = current->mm->mmap;
	for (; vm != NULL; vm = vm->vm_next) {
		if (vm->vm_flags & VM_WRITE)
			continue;
		if (vm->vm_file == NULL)
			continue;
		if (vm->vm_file->f_dentry) {
			if (strcmp
			    (vm->vm_file->f_dentry->d_name.name,
			     short_name) == 0) {
				elf_info[0].addr_offset = vm->vm_start;
				elf_info[0].addr_end = vm->vm_end;
			}
		}
	}

	em_open_modules(&elf_cnt);

	if (current->mm != NULL)
		vm = current->mm->mmap;
	for (i = 0; i < ARRAY_NUM(libname) && vm != NULL; vm = vm->vm_next) {
		if ((vm->vm_flags & (VM_READ | VM_EXEC)) != (VM_READ | VM_EXEC))
			continue;
		if (vm->vm_flags & VM_WRITE)	/* assume text is r-x and text
						   seg addr is base addr */
			continue;
		if (vm->vm_flags & VM_EXECUTABLE)
			continue;
		if (vm->vm_file == NULL)
			continue;

		path = d_path(&vm->vm_file->f_path, buf, sizeof(buf));
		buf[sizeof(buf) - 1] = '\0';

		if (strcmp(path, "/lib/ld-linux.so.2") == 0)
			continue;
		if (strcmp(path, "/devel/lib/ld-linux.so.2") == 0)
			continue;
		if (strcmp(path, "/lib/sonyld.so") == 0)
			continue;

		strncpy(libname[i], path, LIB_NAME_SIZE);
		libname[i][LIB_NAME_SIZE - 1] = 0;

		elf_info[elf_cnt].filename = libname[i];
		elf_info[elf_cnt].addr_offset = vm->vm_start;
		elf_info[elf_cnt].addr_end = vm->vm_end;
		elf_cnt++;
		i++;
	}

	for (i = 0; i < elf_cnt; i++) {
		if (em_open_elffile(i) == -1)
			em_dump_write("\n\tWARNING: file not found: %s\n",
				      elf_info[i].filename);

		dbg("file : %s (%08lx %08lx)\n",
			 elf_info[i].filename, elf_info[i].addr_offset, elf_info[i].addr_end);
	}

out:
	return elf_cnt;
}

static long arm_pc_adjustment(void){
	static long ad;
	long tmp1,tmp2;

	if (ad)
		return ad;

	__asm__ __volatile__ (
		"1: mov %2, pc\n"
		"   adr %1, 1b\n"
		"   sub %0, %2, %1\n"
		: "=r"(ad),"=r"(tmp1),"=r"(tmp2)
		);
	return ad;
}

static inline ulong em_get_user(ulong *p)
{
	ulong v;

	if (__get_user(v, p))
		v = 0;

	return v;
}

static inline ulong em_put_user(ulong v, ulong *p)
{
	return __put_user(v, p);
}

static inline ulong arch_stack_pointer(ulong *frame)
{
	if (!frame)
		return 0UL;
	return em_get_user(frame - 2);
}

static inline ulong arch_entry_address(ulong *frame)
{
	ulong val;

	if (!frame)
		return 0UL;
	val = em_get_user(frame);
	if (val)
		val -= (arm_pc_adjustment() + 4);
	return val;
}

static inline ulong arch_caller_address(ulong *frame)
{
	if (!frame)
		return 0UL;
	return em_get_user(frame - 1);
}

static inline ulong *arch_prev_frame(ulong *frame)
{
	if (!frame)
		return 0UL;
	return (ulong *)em_get_user(frame - 3);
}

#ifdef EM_SP_CALLSTACK
#include "emulator.c"

static int em_get_callstack_sp(int elf_cnt, int no)
{
	int i=0;
	static struct pt_regs __regs;
	struct pt_regs *regs = &__regs;

	__regs = *em_regs;

	dbg("*******%s: fuzzy mode**********\n");

	for (i = 0; i < 16; i ++)
		dbg("%s: %08lx\n", REGs[i], regs->uregs[i]);

	i = no;
	while (i < MAX_CALLSTACK) {
		struct _elf_info *elfinfo ;
		struct callstack *cs;
		ulong addr, start, end;
		ulong lr_orig;
		static struct pt_regs __regs_f;

		 __regs_f = __regs;

		callstack[i].caller = regs->ARM_pc?:regs->ARM_lr;
		em_find_symbol_near(elf_cnt, &callstack[i]);

		cs = &callstack[i];
		elfinfo = cs->elf_info;

		dbg("*%s: %08lx ~ %08lx ~ %08lx* %s:%08lx*  sp:%08lx\n",
		    __func__,
		    cs->entry,
		    cs->caller,
		    cs->entry + cs->size,
		    elfinfo?elfinfo->filename:"NULL",
		    elfinfo? (cs->caller - elfinfo->addr_offset):0,
		    regs->ARM_sp);

		dbg("============backward=============\n");
		R_M = 0;
		start = cs->entry;
		end = cs->caller;
		lr_orig = i==no?0:regs->ARM_lr;
		for (addr = end - 4; addr >= start; addr -= 4) {
			ulong op = get_op((ulong *)addr);
			if (ERR_OP(op))
				break;

			emu_one_ins(op, regs);
		}

		if (regs->ARM_lr != lr_orig) {
			regs->ARM_pc = regs->ARM_lr - 4;
			goto success;
		}

		__regs = __regs_f;

		dbg("============forward=============\n");
		R_M = 1;
		start = cs->caller;
		end = cs->caller + (cs->size?:0x1000);
		for (addr = start; addr < end; addr += 4) {
			ulong pc_orig;

			ulong op = get_op((ulong *)addr);
			if (ERR_OP(op))
				break;

			regs->ARM_pc = (ulong)addr;
			pc_orig = regs->ARM_pc;
			emu_one_ins(op, regs);
			if (regs->ARM_pc != pc_orig)
				goto success;
		}

		if (addr == end)
			regs->ARM_pc = regs->ARM_lr;

	success:
		if (i && cs->caller == (cs-1)->caller)
			break;

		if (!regs->ARM_pc || !regs->ARM_sp)
			break;

		i ++;
	}

	return i;
}
#endif

static int em_get_callstack_fp(int elf_cnt, int no)
{
	int i = no;
	int init = 1;
	ulong *fp = (ulong *)em_regs->ARM_fp;
	ulong *fp_prev;

#ifndef CONFIG_FRAME_POINTER
	if (!user_mode(em_regs))
		fp = NULL;
#endif
	/*
	 * set callstack
	 */
	while (i < MAX_CALLSTACK) {
		if (init) {
			callstack[i].caller = em_regs->ARM_pc?:em_regs->ARM_lr?:arch_caller_address(fp);
			callstack[i].entry = fp?arch_entry_address(fp):em_regs->ARM_pc;
			init = 0;
		}
		else {
			callstack[i].caller = arch_caller_address(fp);
			callstack[i].entry = arch_entry_address(fp);
		}

		if (not_interrupt)
			em_get_entry_str(elf_cnt, &callstack[i]);

		i ++;
		fp_prev = fp;
		fp = arch_prev_frame(fp);
		if (!fp)
			break;
		if (fp == fp_prev)
			break;
	}

	return i;
}

#define DUMP_USER(regs) !user_mode(regs) && current->mm
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
# define USER_REGISTER \
	(struct pt_regs *)((unsigned long)current->thread_info + THREAD_SIZE - sizeof(struct pt_regs) - 8)
#else
# define USER_REGISTER task_pt_regs(current)
#endif

struct callstack *em_get_callstack(void)
{
	int elf_cnt;
	int no;

	elf_cnt = em_open_elffiles();

	fp_valid = 1;
	no = em_get_callstack_fp(elf_cnt, 0);

	if (DUMP_USER(em_regs)) {
		struct pt_regs *em_regs_tmp = em_regs;
		callstack[no++].caller = CALLER_USER;
		em_regs = USER_REGISTER;
		no = em_get_callstack_fp(elf_cnt, no);
		em_regs = em_regs_tmp;
	}

#ifdef EM_SP_CALLSTACK
	callstack[no].caller = CALLER_SP;
	strncpy(callstack[no].entry_str, "##################", sizeof(callstack[0].entry_str)/sizeof(callstack[0].entry_str[0]) - 1);
	callstack[no].entry_str[sizeof(callstack[0].entry_str)/sizeof(callstack[0].entry_str[0]) - 1] = '\0';
	no++;

	em_dump_write("%s: %d\n", __func__, no);

	fp_valid = 0;
	if (not_interrupt && !fp_valid) {
		no = em_get_callstack_sp(elf_cnt, no);
	}
#endif
	em_close_elffiles(elf_cnt);

	em_dump_write("%s: %d\n", __func__, no);
	callstack[no].caller = CALLER_END;

	return callstack;
}

void em_dump_user(void (func)(int, char **), int argc, char **argv, const char *msg)
{
	struct pt_regs *em_regs_tmp = em_regs;
	em_regs = USER_REGISTER;
	em_dump_write("\n--------------- user %s dump ----------------", msg);
	func(argc, argv);
	em_dump_write("---------------------------------------------------\n");
	em_regs = em_regs_tmp;
}

void em_do_dump_regs(int argc, char **argv)
{
	char *mode;
	static char mode_list[][4] = {"USR","FIQ","IRQ","SVC","ABT"
				      ,"UND","SYS","???"};

	em_dump_write("\n[register dump]\n");

	em_dump_write("r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n",
		      em_regs->ARM_r0, em_regs->ARM_r1,
		      em_regs->ARM_r2, em_regs->ARM_r3);
	em_dump_write("r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n",
		      em_regs->ARM_r4, em_regs->ARM_r5,
		      em_regs->ARM_r6, em_regs->ARM_r7);
	em_dump_write("r8: 0x%08x  r9: 0x%08x r10: 0x%08x  fp: 0x%08x\n",
		      em_regs->ARM_r8, em_regs->ARM_r9,
		      em_regs->ARM_r10, em_regs->ARM_fp);
	em_dump_write("ip: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n",
		      em_regs->ARM_ip, em_regs->ARM_sp,
		      em_regs->ARM_lr, em_regs->ARM_pc);

#define PSR_MODE_MASK 0x0000001f
	switch (em_regs->ARM_cpsr & PSR_MODE_MASK) {
	case USR_MODE: mode = mode_list[0]; break;
	case FIQ_MODE: mode = mode_list[1]; break;
	case IRQ_MODE: mode = mode_list[2]; break;
	case SVC_MODE: mode = mode_list[3]; break;
	case ABT_MODE: mode = mode_list[4]; break;
	case UND_MODE: mode = mode_list[5]; break;
	case SYSTEM_MODE: mode = mode_list[6]; break;
	default: mode = mode_list[7];break;
	}
	em_dump_write("cpsr: 0x%08x: Flags: %c%c%c%c, "
		      "IRQ: o%s, FIQ: o%s, Thumb: o%s, Mode: %s\n",
		      em_regs->ARM_cpsr,
		      (em_regs->ARM_cpsr & PSR_N_BIT) ? 'N' : 'n',
		      (em_regs->ARM_cpsr & PSR_Z_BIT) ? 'Z' : 'z',
		      (em_regs->ARM_cpsr & PSR_C_BIT) ? 'C' : 'c',
		      (em_regs->ARM_cpsr & PSR_V_BIT) ? 'V' : 'v',
		      (em_regs->ARM_cpsr & PSR_I_BIT) ? "ff" : "n",
		      (em_regs->ARM_cpsr & PSR_F_BIT) ? "ff" : "n",
		      (em_regs->ARM_cpsr & PSR_T_BIT) ? "n" : "ff",
		      mode);
}

void em_dump_regs(int argc, char **argv)
{
	em_do_dump_regs(argc, argv);

	if (DUMP_USER(em_regs))
		em_dump_user(em_do_dump_regs, argc, argv, "register");
}

void em_dump_regs_detail(int argc, char **argv)
{
	em_dump_regs(1, NULL);
	em_dump_write("\n[cp15 register dump]\n\n");

	/* FIXME: This is ARM926EJ-S specific... */
	if (0) {
		unsigned long id, cache, tcm, control, trans,
			dac, d_fsr, i_fsr, far, d_lock, i_lock,
			d_tcm, i_tcm, tlb_lock, fcse, context;
		static char size_list[][8] = {
			"  0", "  4", "  8", " 16", " 32", " 64",
			"128", "256", "512", "1024", "???"};
		char *dsiz, *isiz;
		static char fault_stat[][32] = {
			"Alignment", "External abort on translation",
			"Translation", "Domain", "Permission",
			"External abort", "???" };
		char *stat;
		static char alloc[][8] = {"locked", "opened"};

		asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (id));
		asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (cache));
		asm volatile ("mrc p15, 0, %0, c0, c0, 2" : "=r" (tcm));
		asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (control));
		asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (trans));
		asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (dac));
		asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r" (d_fsr));
		asm volatile ("mrc p15, 0, %0, c5, c0, 1" : "=r" (i_fsr));
		asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r" (far));
		asm volatile ("mrc p15, 0, %0, c9, c0, 0" : "=r" (d_lock));
		asm volatile ("mrc p15, 0, %0, c9, c0, 1" : "=r" (i_lock));
		asm volatile ("mrc p15, 0, %0, c9, c1, 0" : "=r" (d_tcm));
		asm volatile ("mrc p15, 0, %0, c9, c1, 1" : "=r" (i_tcm));
		asm volatile ("mrc p15, 0, %0, c10, c0, 0" : "=r" (tlb_lock));
		asm volatile ("mrc p15, 0, %0, c13, c0, 0" : "=r" (fcse));
		asm volatile ("mrc p15, 0, %0, c13, c0, 1" : "=r" (context));

#define MASK_ASCII  0xff000000
#define MASK_SPEC   0x00f00000
#define MASK_ARCH   0x000f0000
#define MASK_PART   0x0000fff0
#define MASK_LAYOUT 0x0000000f
		em_dump_write("* ID code: %08x:  "
			      "tm: %c, spec: %1x, arch: %1x, "
			      "part: %3x, layout: %1x\n",
			      id,
			      ((id & MASK_ASCII)  >> 24),
			      ((id & MASK_SPEC)   >> 20),
			      ((id & MASK_ARCH)   >> 16),
			      ((id & MASK_PART)   >>  4),
			      ((id & MASK_LAYOUT)));

#define MASK_CTYPE 0x1e000000
#define MASK_S_BIT 0x01000000
#define MASK_DSIZE 0x00fff000
#define MASK_ISIZE 0x00000fff
#define MASK_SIZE  0x000003c0
#define MASK_ASSOC 0x00000038
#define MASK_LEN   0x00000003

		switch ((((cache & MASK_DSIZE) >> 12) & MASK_SIZE) >> 6) {
		case 0x3: dsiz = size_list[1]; break;
		case 0x4: dsiz = size_list[2]; break;
		case 0x5: dsiz = size_list[3]; break;
		case 0x6: dsiz = size_list[4]; break;
		case 0x7: dsiz = size_list[5]; break;
		case 0x8: dsiz = size_list[6]; break;
		default:  dsiz = size_list[10]; break;
		}
		switch (((cache & MASK_ISIZE) & MASK_SIZE) >> 6) {
		case 0x3: isiz = size_list[1]; break;
		case 0x4: isiz = size_list[2]; break;
		case 0x5: isiz = size_list[3]; break;
		case 0x6: isiz = size_list[4]; break;
		case 0x7: isiz = size_list[5]; break;
		case 0x8: isiz = size_list[6]; break;
		default:  isiz = size_list[10]; break;
		}
		em_dump_write("* Cache Type: %08x:  %s, %s,\n"
			      "\tDCache: %sKB, %s-way, line: %s word\n"
			      "\tICache: %sKB, %s-way, line: %s word\n",
			      cache,
			      (((cache & MASK_CTYPE) >> 25) == 0xe) ?
			      "write-back" : "write-???",
			      (cache & MASK_S_BIT) ?
			      "harvard" : "unified",
			      dsiz,
			      ((((cache & MASK_DSIZE) >> 12) & MASK_ASSOC >> 3)
			       == 0x2) ? "4" : "?",
			      ((((cache & MASK_DSIZE) >>12 ) & MASK_LEN)
			       == 0x2) ? "8" : "?",
			      isiz,
			      (((cache & MASK_ISIZE) & MASK_ASSOC >> 3)
			       == 0x2) ? "4" : "?",
			      (((cache & MASK_ISIZE) & MASK_LEN)
			       == 0x2) ? "8" : "?");

#define MASK_DTCM 0x00010000
#define MASK_ITCM 0x00000001
		em_dump_write("* TCM Status: %08x: "
			      "DTCM %spresent, ITCM %spresent\n",
			      tcm,
			      (tcm & MASK_DTCM) ? "" : "not ",
			      (tcm & MASK_ITCM) ? "" : "not ");

#define MASK_L4     0x00008000
#define MASK_RR     0x00004000
#define MASK_VEC    0x00002000
#define MASK_ICACHE 0x00001000
#define MASK_ROM    0x00000200
#define MASK_SYS    0x00000100
#define MASK_END    0x00000080
#define MASK_DCACHE 0x00000004
#define MASK_ALIGN  0x00000002
#define MASK_MMU    0x00000001
		em_dump_write("* Control: %08x: L4: %s, Cache: %s replace\n"
			      "\texception vector at %s endian: %s\n"
			      "\tICache %sabled, DCache %sabled, "
			      "Align %sabled, MMU %sabled\n"
			      "\tROM protection: %s, system protection: %s\n",
			      control,
			      (control & MASK_L4) ? "1" : "0",
			      (control & MASK_RR) ? "round robin" : "random",
			      (control & MASK_VEC) ?
			      "ffff00{00-1c}" : "000000{00-1c}",
			      (control & MASK_END) ? "big" : "little",
			      (control & MASK_ICACHE) ? "en" : "dis",
			      (control & MASK_DCACHE) ? "en" : "dis",
			      (control & MASK_ALIGN) ? "en" : "dis",
			      (control & MASK_MMU) ? "en" : "dis",
			      (control & MASK_ROM) ? "1" : "0",
			      (control & MASK_SYS) ? "1" : "0");

		em_dump_write("* Translation Table Base: %08x\n", trans);
		em_dump_write("* Domain Access Control: %08x\n", dac);

#define MASK_DOMAIN 0x000000f0
#define MASK_STATUS 0x0000000f
		switch (d_fsr & MASK_STATUS) {
		case 0x1: case 0x3: stat = fault_stat[0]; break;
		case 0xc: case 0xe: stat = fault_stat[1]; break;
		case 0x5: case 0x7: stat = fault_stat[2]; break;
		case 0x9: case 0xb: stat = fault_stat[3]; break;
		case 0xd: case 0xf: stat = fault_stat[4]; break;
		case 0x8: case 0xa: stat = fault_stat[5]; break;
		default:            stat = fault_stat[6]; break;
		}
		em_dump_write("* Fault Status: data: %08x, inst: %08x\n"
			      "\tat domain: %x, status: %s\n",
			      d_fsr, i_fsr,
			      ((d_fsr & MASK_DOMAIN) >> 4), stat);

		em_dump_write("* Fault Address: %08x\n", far);

#define MASK_WAY3 0x00000008
#define MASK_WAY2 0x00000004
#define MASK_WAY1 0x00000002
#define MASK_WAY0 0x00000001
		em_dump_write("* Cache Lockdown: DCache: %08x, ICache: %08x\n"
			      "\tDCache: way 3: %s, 2: %s, 1: %s, 0: %s\n"
			      "\tICache: way 3: %s, 2: %s, 1: %s, 0: %s\n",
			      d_lock, i_lock,
			      (d_lock & MASK_WAY3) ? alloc[0] : alloc[1],
			      (d_lock & MASK_WAY2) ? alloc[0] : alloc[1],
			      (d_lock & MASK_WAY1) ? alloc[0] : alloc[1],
			      (d_lock & MASK_WAY0) ? alloc[0] : alloc[1],
			      (i_lock & MASK_WAY3) ? alloc[0] : alloc[1],
			      (i_lock & MASK_WAY2) ? alloc[0] : alloc[1],
			      (i_lock & MASK_WAY1) ? alloc[0] : alloc[1],
			      (i_lock & MASK_WAY0) ? alloc[0] : alloc[1]);

#define MASK_BASE   0xfffff000
#undef  MASK_SIZE
#define MASK_SIZE   0x0000003c
#define MASK_ENABLE 0x00000001
		switch ((d_tcm & MASK_SIZE) >> 2) {
		case 0x0: dsiz = size_list[0]; break;
		case 0x3: dsiz = size_list[1]; break;
		case 0x4: dsiz = size_list[2]; break;
		case 0x5: dsiz = size_list[3]; break;
		case 0x6: dsiz = size_list[4]; break;
		case 0x7: dsiz = size_list[5]; break;
		case 0x8: dsiz = size_list[6]; break;
		case 0x9: dsiz = size_list[7]; break;
		case 0xa: dsiz = size_list[8]; break;
		case 0xb: dsiz = size_list[9]; break;
		default:  dsiz = size_list[10]; break;
		}
		switch ((i_tcm & MASK_SIZE) >> 2) {
		case 0x0: isiz = size_list[0]; break;
		case 0x3: isiz = size_list[1]; break;
		case 0x4: isiz = size_list[2]; break;
		case 0x5: isiz = size_list[3]; break;
		case 0x6: isiz = size_list[4]; break;
		case 0x7: isiz = size_list[5]; break;
		case 0x8: isiz = size_list[6]; break;
		case 0x9: isiz = size_list[7]; break;
		case 0xa: isiz = size_list[8]; break;
		case 0xb: isiz = size_list[9]; break;
		default:  isiz = size_list[10]; break;
		}
		em_dump_write("* TCM Region: data: %08x, inst: %08x\n"
			      "\tDTCM: Base addr: %08x, size: %sKB, %sabled\n"
			      "\tITCM: Base addr: %08x, size: %sKB, %sabled\n",
			      d_tcm, i_tcm,
			      ((d_tcm & MASK_BASE) >> 12), dsiz,
			      (d_tcm & MASK_ENABLE) ? "en" : "dis",
			      ((i_tcm & MASK_BASE) >> 12), isiz,
			      (i_tcm & MASK_ENABLE) ? "en" : "dis");

#define MASK_VICT 0x1c000000
#define MASK_PBIT 0x00000001
		em_dump_write("* TLB Lockdown: %08x: "
			      "victim: %x, preserve: %s\n",
			      tlb_lock,
			      ((tlb_lock & MASK_VICT) >> 26),
			      (tlb_lock & MASK_PBIT) ? alloc[0] : alloc[1]);

#define MASK_FCSE 0xfe000000
		em_dump_write("* FCSE PID: %08x: pid: %x\n",
			      fcse, ((fcse & MASK_FCSE) >> 25));

		em_dump_write("* Context ID: %08x\n", context);
	}
	em_dump_write("\n");
}

#define EM_USERSTACK_MAXDUMP	4 /* pages */

static void em_dump_till_end_of_page(unsigned long *sp, int user)
{
	unsigned long *tail, *sp_orig = sp;
	unsigned long stackdata;

	if (!user) {
		unsigned long stack_size, stack_page;

		stack_size = calc_stack_size(sp);
		stack_page = (unsigned long)sp & ~(stack_size - 1);
#ifdef CONFIG_SNSC_DEBUG_STACK_LIMIT
		tail = (unsigned long *)(stack_page + calc_start_sp(sp));
#else /* CONFIG_SNSC_DEBUG_STACK_LIMIT */
		tail = (unsigned long *)(stack_page + stack_size);
#endif /* CONFIG_SNSC_DEBUG_STACK_LIMIT */
	} else {
		tail = (unsigned long *)(((unsigned long)sp & PAGE_MASK) + PAGE_SIZE*EM_USERSTACK_MAXDUMP);
	}

	while (sp < tail) {
		if (__get_user(stackdata, sp)) {
			em_dump_write("\n (bad stack address)\n");
			break;
		}

		if (((unsigned long)sp-(unsigned long)sp_orig) % 0x10 == 0) {
			em_dump_write("\n0x%08x: ", sp);
		}

		em_dump_write("0x%08x ", stackdata);

		sp++;
	}
}

void em_do_dump_stack(int argc, char **argv)
{
	unsigned long *sp = (unsigned long *)(em_regs->ARM_sp & ~0x03);
	unsigned long *fp = (unsigned long *)(em_regs->ARM_fp & ~0x03);
	unsigned long *tail;
	unsigned long backchain;
	unsigned long stackdata;
	int frame = 1;

#ifndef CONFIG_FRAME_POINTER
	if (!user_mode(em_regs))
		fp = NULL;
#endif
	tail = sp + PAGE_SIZE / 4;

	em_dump_write("\n[stack dump]\n");

	backchain = arch_stack_pointer(fp);
	while (sp < tail) {
		if (backchain == (unsigned long)sp) {
			em_dump_write("|");
			fp = arch_prev_frame(fp);
			if (!fp)
				break;

			backchain = arch_stack_pointer(fp);
			if (!backchain)
				break;
		} else {
			em_dump_write(" ");
		}

		if (backchain < (unsigned long)sp) {
			break;
		}

		if (__get_user(stackdata, sp)) {
			em_dump_write("\n (bad stack address)\n");
			break;
		}

		if (((unsigned long)tail-(unsigned long)sp) % 0x10 == 0) {
			if (frame) {
				em_dump_write("\n0x%08x:|", sp);
				frame = 0;
			} else {
				em_dump_write("\n0x%08x: ", sp);
			}
		}

		em_dump_write("0x%08x", stackdata);

		sp++;
	}

	em_dump_write("\n");

	em_dump_write("\n #################em_dump_till_end_of_page###########\n");
	em_dump_till_end_of_page(sp, user_mode(em_regs));
	em_dump_write("\n");
}

void em_dump_stack(int argc, char **argv)
{
	em_do_dump_stack(argc, argv);

	if (DUMP_USER(em_regs))
		em_dump_user(em_do_dump_stack, argc, argv, "stack");
}

static struct fsr_info {
	const char *name;
} fsr_info[] = {
	/*
	 * The following are the standard ARMv3 and ARMv4 aborts.  ARMv5
	 * defines these to be "precise" aborts.
	 */
	{ "vector exception"		   },
	{ "alignment exception"		   },
	{ "terminal exception"		   },
	{ "alignment exception"		   },
	{ "external abort on linefetch"	   },
	{ "section translation fault"	   },
	{ "external abort on linefetch"	   },
	{ "page translation fault"	   },
	{ "external abort on non-linefetch" },
	{ "section domain fault"		   },
	{ "external abort on non-linefetch" },
	{ "page domain fault"		   },
	{ "external abort on translation"   },
	{ "section permission fault"	   },
	{ "external abort on translation"   },
	{ "page permission fault"	   },
	/*
	 * The following are "imprecise" aborts, which are signalled by bit
	 * 10 of the FSR, and may not be recoverable.  These are only
	 * supported if the CPU abort handler supports bit 10.
	 */
	{ "unknown 16"			   },
	{ "unknown 17"			   },
	{ "unknown 18"			   },
	{ "unknown 19"			   },
	{ "lock abort"			   }, /* xscale */
	{ "unknown 21"			   },
	{ "imprecise external abort"	   }, /* xscale */
	{ "unknown 23"			   },
	{ "dcache parity error"		   }, /* xscale */
	{ "unknown 25"			   },
	{ "unknown 26"			   },
	{ "unknown 27"			   },
	{ "unknown 28"			   },
	{ "unknown 29"			   },
	{ "unknown 30"			   },
	{ "unknown 31"			   }
};

void em_show_syndrome(void)
{
	unsigned long fsr,far;
	const struct fsr_info *inf;
	struct task_struct *tsk = current;

	em_dump_write("\n\n[Exception Syndrome]\n");

	switch(tsk->thread.trap_no){
	case 6:
		em_dump_write("Illeagle Instruction at 0x%08lx\n",
			      em_regs->ARM_pc);

		break;
	case 14:
	default:
		fsr = tsk->thread.error_code;
		far = tsk->thread.address;
		inf = fsr_info + (fsr & 15) + ((fsr & (1 << 10)) >> 6);

		em_dump_write("%s (0x%03x) at 0x%08lx\n",
			      inf->name, fsr, far);

		break;
	}
}

#if defined(CONFIG_CPU_V6) || defined(CONFIG_CPU_V7)
static unsigned long p15_v2p(unsigned long v)
{
	unsigned long p;

	/* VA to PA */
	__asm__ __volatile__(
		"mcr    p15, 0, %0, c7, c8, 0   @ VA to PA translation"
		:
		: "r" (v)
		: "cc");

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c7, c4, 0   @ read PA"
		: "=r" (p)
		:
		: "cc");

	/* translation abort */
	if(p & 0x1)
		return 0xdeadbeef;

	return (p & PAGE_MASK) | (v & ~(PAGE_MASK));
}
#else
static unsigned long p15_v2p(unsigned long v)
{
	return 0;
}
#endif

static unsigned long pgd_v2p(unsigned long v)
{
	pgd_t *pgd;
	pmd_t *pmd;
        pte_t *pte;
	unsigned long p;

	if (v >= TASK_SIZE) {
		pgd = pgd_offset_k(v);
	}
	else {
		pgd = pgd_offset(current->active_mm, v);
	}

	pmd = pmd_offset(pgd, v);
	if(pmd_none(*pmd))
		return 0xdeadbeef;

	if ((pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_SECT){
		p = (*pmd & SECTION_MASK) | (v & (~SECTION_MASK));
		return p;
	}

	pte = pte_offset_kernel(pmd, v);
	p = pte_pfn(*pte) << PAGE_SHIFT | (v & ~(PAGE_MASK));

	return(p);
}

void em_cmd_v2p(int argc, char **argv)
{
	unsigned long addr;

	if (argc != 2)
		return;

	addr = simple_strtoul(argv[1], NULL, 16);
	em_dump_write("virt:0x%08lx \n\t-> phys(cpu):0x%08lx \n\t-> phys(pgd):0x%08lx\n", addr, p15_v2p(addr), pgd_v2p(addr));
}
