/*
 * include/asm-arm/mach/warmboot.h
 *
 * warmboot header
 *
 * Copyright 2005,2006 Sony Corporation
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
 *
 */

#ifndef __WARMBOOT_H__
#define __WARMBOOT_H__

#include <linux/list.h>
#include <linux/suspend.h>

struct wb_device;
struct wb_device {
	struct list_head list;

	char *name;
	unsigned long capacity;
	unsigned long phys_sector_size;
	unsigned long sector_size;
	unsigned long flag;
#define WBI_SILENT_MODE 0x01
	int (*suspend_prepare)(struct wb_device *);
	int (*open)(struct wb_device *);
	int (*write_sector)(struct wb_device *,unsigned char *, u32, u32);
	int (*read_sector)(struct wb_device *,unsigned char *, u32, u32);
	int (*close)(struct wb_device *);
	int (*suspend_finish)(struct wb_device *);
	void *data;
};

/* fVersion */
#define WBI_INVAL_VER "invl"
#define WBI_PROF_VER  "prof"
#define WBI_PRE_VER   "WBI0"
#define WBI_VER       "WBI1"

struct wb_header {
	char fVersion[4];
	unsigned long sections;
	unsigned long flag;
#define WBI_COMPRESS 0x01
#define WBI_NOT_KERNEL_RO 0x02
#define WBI_CKSUM 0x04
#define WBI_LZ77_COMPRESS 0x08
	unsigned long resume_vector;
	unsigned long dVersion;
	unsigned long sector_size;
	unsigned long r_data_size;//real data size
	unsigned long kern_start;
	unsigned long kern_size;
	unsigned long kern_cksum;
	unsigned long o_data_size;//original data size, not compressed
};

struct wb_section {
	unsigned long addr;
	unsigned long rlen;//real length
	unsigned long cksum;
	unsigned long flag;
	unsigned long olen;//original length, not compressed
	unsigned long virt;//just for linux
	unsigned long __pad[1];
	unsigned long meta_cksum;
};

struct wbi_compressor {
	int ncore;
	int (*init)(void);
	int (*deflate)(int id, unsigned long src, unsigned long slen,
	               unsigned long dst, unsigned long dlen,
	               unsigned long work, unsigned long wlen);
	uint (*deflate_poll)(int id);
	void (*deflate_cancel)(int id);
	void (*deflate_end)(int id);
	int (*deflate_finish)(int id, unsigned long dst, unsigned long dlen);
	void (*exit)(void);
	unsigned long (*get_max_slen)(unsigned long dlen, unsigned long wlen);
};

struct wbi_lzp_entry {
	uint32_t off;
	uint32_t sz;
};

struct wbi_lzp_hdr {
	uint32_t signature;
	uint32_t blkbits;
	uint32_t tbl1_off;
	uint32_t tbl1_sz;
	struct wbi_lzp_entry fhdr;
};

#if defined(CONFIG_WARM_BOOT_IMAGE)
extern int cxd_create_warmbootimage(unsigned long entry, unsigned long adjust_vma);
extern int wb_register_device(struct wb_device *,int);
extern int wb_unregister_device(struct wb_device *);
extern int wbi_register_compressor(struct wbi_compressor *);

extern int  warm_boot_pm_begin(suspend_state_t);
extern int  warm_boot_pm_prepare(void);
extern void warm_boot_pm_finish(void);
extern int wbi_drop(void);
extern int wbi_resume_drop(void);
extern int wbi_add_region(ulong phys, ulong size);
#else
#define wb_register_device(device,def) do{}while(0);
#define wb_unregister_device(device) do{}while(0);

static inline int wbi_register_compressor(struct wbi_compressor *device){
	return 0;
}

static inline int warm_boot_pm_begin(suspend_state_t state){
	return 0;
}

static inline int warm_boot_pm_prepare(void){
	return 0;
}

static inline void warm_boot_pm_finish(void){
}

static inline int wbi_drop(void){
	return 0;
}
static inline int wbi_resume_drop(void){
	return 0;
}
static inline int wbi_add_region(ulong phys, ulong size)
{
	return 0;
}
#endif
#ifdef CONFIG_EJ_CLEAR_MEMORY
extern int wbi_memclr(void);
#endif /* CONFIG_EJ_CLEAR_MEMORY */

#endif /* __WARMBOOT_H__ */
