/*
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
#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

struct _elf_info {
	char *filename;
	struct file *file;
	mm_segment_t fs;
	Elf32_Shdr sh_dynsym;
	Elf32_Shdr sh_dynstr;
	Elf32_Shdr sh_strtab;
	Elf32_Shdr sh_symtab;
	unsigned long addr_offset;
	unsigned long addr_end;
	int strip;
};

#define CALLSTACK_STR_SZ 127
struct callstack {
	unsigned long entry;
	unsigned long caller;
#define CALLER_END       (-1UL)
#define CALLER_SP        (-2UL)
#define CALLER_USER      (-3UL)

	unsigned long size;
	struct _elf_info *elf_info;
	char entry_str[CALLSTACK_STR_SZ + 1];
};

extern void em_cmd_v2p(int argc, char **argv);
extern void em_dump_regs(int argc, char **argv);
extern void em_dump_stack(int argc, char **argv);
extern void em_dump_regs_detail(int argc, char **argv);
extern struct callstack *em_get_callstack(void);
extern void em_show_syndrome(void);

extern void em_dump_write(const char *format, ...);
extern char *em_get_execname(void);

#endif
