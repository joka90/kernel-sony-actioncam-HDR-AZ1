/*
 * arch/arm/mach-cxd4132/cmpr.c
 *
 * Compressor driver
 *
 * Copyright 2011 Sony Corporation
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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <asm/mach/warmboot.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <mach/time.h>
#include <mach/cxd90014_cpuid.h>
#include <mach/cmpr.h>

#define N_CMPRFILE 2
#define PATHLEN 64

static char cmpr_path[N_CMPRFILE][PATHLEN];
/* firm ID 0 */
module_param_string(path, cmpr_path[0], PATHLEN, S_IRUGO | S_IWUSR);
/* firm ID 1 */
module_param_string(path2, cmpr_path[1], PATHLEN, S_IRUGO | S_IWUSR);

static char cmpr_mem[64];
module_param_string(mem, cmpr_mem, sizeof(cmpr_mem), S_IRUGO | S_IWUSR);

static int cmpr_index_len = CMP_DEFAULT_INDEX_LEN;
module_param_named(index_len, cmpr_index_len, int, S_IRUSR|S_IWUSR);

static struct cmpr_parg *cmpr_pargs;
static unsigned long cmpr_bin[N_CMPRFILE];

#define N_MMUPAGE 22

typedef struct {
    uint32_t	vld;
    uint32_t	vadr_sta;
    uint32_t	vadr_end;
    uint32_t	padr_sta;
} cmpr_mmu_t;

typedef struct {
	void const __iomem *clock;
	void const __iomem *reset;
	void const __iomem *common;
	void const __iomem *core;
	cmpr_mmu_t *mmu;
	int firmid;
} cmpr_t;

static cmpr_t cmpr_rsrc[] = {
	{	.clock  = IO_ADDRESSP(CMPR3_ENV_CLK_ADDR),
		.reset  = IO_ADDRESSP(CMPR3_ENV_RST_ADDR),
		.common = IO_ADDRESSP(CMPR3_COMMON_ADDR),
		.core   = IO_ADDRESSP(CMPR3_CORE_ADDR),
		.mmu    = 0,
		.firmid	= 0,
	},
	{	.clock  = IO_ADDRESSP(CMPR1_ENV_CLK_ADDR),
		.reset  = IO_ADDRESSP(CMPR1_ENV_RST_ADDR),
		.common = IO_ADDRESSP(CMPR1_COMMON_ADDR),
		.core   = IO_ADDRESSP(CMPR1_CORE_ADDR),
		.mmu    = 0,
		.firmid	= 1,
	},
	{	.clock  = IO_ADDRESSP(CMPR2_ENV_CLK_ADDR),
		.reset  = IO_ADDRESSP(CMPR2_ENV_RST_ADDR),
		.common = IO_ADDRESSP(CMPR2_COMMON_ADDR),
		.core   = IO_ADDRESSP(CMPR2_CORE_ADDR),
		.mmu    = 0,
		.firmid	= 1,
	},
};

#define N_CMPRS ((sizeof cmpr_rsrc) / (sizeof cmpr_rsrc[0]))
static cmpr_t *cmprs = cmpr_rsrc;

static int cmpr_ncore = N_CMPRS;
module_param_named(ncore, cmpr_ncore, int, S_IRUSR|S_IWUSR);

static ssize_t cmpr_load_file(char *path, unsigned long phys, ssize_t maxsz)
{
	int ret = 0;
	struct kstat stat;
	struct file *fp;
	ssize_t r_size;
	loff_t offs = 0;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(KERNEL_DS);

	if (vfs_stat(path, &stat)) {
		printk(KERN_ERR "cmpr: vfs_stat error: %s\n", path);
		ret = -EACCES;
		goto out_fs;
	}

	if (stat.size > maxsz) {
		printk(KERN_ERR "cmpr: file size error: %d > %d\n",
		       (size_t)stat.size, maxsz);
		ret = -EINVAL;
		goto out_fs;
	}

	fp = filp_open(path, O_RDONLY, 0444);
	if (IS_ERR(fp)) {
		printk(KERN_ERR "cmpr: file open error: %ld\n", PTR_ERR(fp));
		ret = PTR_ERR(fp);
		goto out;
	}

	r_size = vfs_read(fp, __va(phys), stat.size, &offs);
	if (r_size < 0) {
		printk(KERN_ERR "cmpr: load error: %d\n", r_size);
		ret = r_size;
		goto out;
	}
	if (r_size != stat.size) {
		printk(KERN_ERR "cmpr: load error: %d > %d\n", r_size,
		       (size_t)stat.size);
		ret = -EAGAIN;
		goto out;
	}
	ret = r_size;

	dma_sync_single_for_device(NULL, phys, stat.size, DMA_TO_DEVICE);

out:
	filp_close(fp, NULL);

out_fs:
	set_fs(fs);

	return ret;
}

static void cmpr_reset(int id, int assert)
{
	if (assert)
		writel(CMPR_ENV_RST_MASK, cmprs[id].reset+HTP_CLKRST_CLR);
	else
		writel(CMPR_ENV_RST_MASK, cmprs[id].reset+HTP_CLKRST_SET);
}

static void cmpr_clock(int id, int on)
{
	if (on)
		writel(CMPR_ENV_CLK_MASK, cmprs[id].clock+HTP_CLKRST_SET);
	else
		writel(CMPR_ENV_CLK_MASK, cmprs[id].clock+HTP_CLKRST_CLR);
}

static void cmpr_setup_env(int id)
{
	cmpr_reset(id, 0);
	cmpr_clock(id, 1);
}

static void cmpr_unset_env(int id)
{
	cmpr_clock(id, 0);
	cmpr_reset(id, 1);
}

static unsigned long cmpr_demand_wlen(unsigned long slen)
{
	unsigned long blk_size, num_blk;

	blk_size = 1 << CMPR_LZPART_BLKBITS;
	num_blk = (slen + blk_size - 1) / blk_size;

	return num_blk * ((blk_size / 4096) * 4112 + 8);
}

static unsigned long cmpr_demand_dlen(unsigned long slen)
{
	return cmpr_demand_wlen(slen) + sizeof(struct wbi_lzp_hdr);
}

#define MAX_SLEN 0x0fffffffUL
static unsigned long cmpr_get_max_slen(unsigned long dlen, unsigned long wlen)
{
	unsigned long slen, blk_size;

	blk_size = 1 << CMPR_LZPART_BLKBITS;

	for (slen = blk_size; slen < MAX_SLEN; slen += blk_size) {
		if ((dlen < cmpr_demand_dlen(slen))
		    || (wlen < cmpr_demand_wlen(slen))) break;
	}
	slen -= blk_size;

	return slen;
}
static int cmpr_deflate_start(int id,
			      unsigned long src, unsigned long slen,
                              unsigned long dst, unsigned long dlen,
                              unsigned long work, unsigned long wlen)
{
	void const __iomem *cmpr_common = cmprs[id].common;
	void const __iomem *cmpr_core = cmprs[id].core;
	int firm = cmprs[id].firmid;
	struct cmpr_parg *parg = &cmpr_pargs[id];
	cmpr_mmu_t *mmu = cmprs[id].mmu;
	int i;

	if (!IS_ALIGNED(src, CMPR_ALIGN) || !IS_ALIGNED(dst, CMPR_ALIGN)
            || !IS_ALIGNED(work, CMPR_ALIGN)) {
		printk(KERN_ERR "src=%lx,dst=%lx,work=%lx\n", src, dst, work);
		return -EINVAL;
	}

	if ((dlen < cmpr_demand_dlen(slen))
	    || (wlen < cmpr_demand_wlen(slen))) {
		printk(KERN_ERR "dlen=%lx,%lx wlen=%lx,%lx\n",
		       dlen, cmpr_demand_dlen(slen),
		       wlen, cmpr_demand_wlen(slen));
		return -EINVAL;
	}

	mmu[0].vld = 1;
	mmu[0].vadr_sta = CMPR_VADR(CMPR_VADR_ARG_STA);
	mmu[0].vadr_end = CMPR_VADR(CMPR_VADR_ARG_END);
	mmu[0].padr_sta = CMPR_PADR(parg);

	mmu[1].vld = 1;
	mmu[1].vadr_sta = CMPR_VADR(CMPR_VADR_SRC_STA);
	mmu[1].vadr_end = CMPR_VADR(CMPR_VADR_SRC_END);
	mmu[1].padr_sta = CMPR_PADR(src);

	mmu[2].vld = 1;
	mmu[2].vadr_sta = CMPR_VADR(CMPR_VADR_DST_STA);
	mmu[2].vadr_end = CMPR_VADR(CMPR_VADR_DST_END);
	mmu[2].padr_sta = CMPR_PADR(dst);

	mmu[3].vld = 1;
	mmu[3].vadr_sta = CMPR_VADR(CMPR_VADR_WRK_STA);
	mmu[3].vadr_end = CMPR_VADR(CMPR_VADR_WRK_END);
	mmu[3].padr_sta = CMPR_PADR(work);

	for( i=4; i<N_MMUPAGE; i++){
	    mmu[i].vld = 0;
	    mmu[i].vadr_sta = 0;
	    mmu[i].vadr_end = 0;
	    mmu[i].padr_sta = 0;
	}

	parg->src_adr = mmu[1].padr_sta;
	parg->dst_adr = mmu[2].padr_sta;
	parg->work_adr = mmu[3].padr_sta;

	parg->input_size = (uint32_t)slen;
	if (slen > SZ_64K) {
		parg->blk_bits = CMP_DEFAULT_BLKBITS;
	} else if (slen <= 32*SZ_1K) {
		parg->blk_bits = 15;
	} else {
		parg->blk_bits = 16;
	}

	parg->reserved = 0;
	parg->reserved2 = 0;
	parg->index_len = cmpr_index_len;
	dsb();

	cmpr_setup_env(id);

	writel(CMPR_CMN_CKEN_BIT, REG_CMN(CMPR_CMN_CKEN));

	writel(CMPR_ADDR_BASE, REG_CMN(CMPR_CMN_IPC));
	writel(CMPR_ADDR_OFST(cmpr_bin[firm]), REG_CMN(CMPR_ADRS_OFST));

	writel(mmu[0].padr_sta, REG_COR(CMPR_COR_MBX(0)));
	writel(CMPR_ADDR_CPU2CMPR(mmu), REG_COR(CMPR_COR_MBX(1)));
	writel(0, REG_COR(CMPR_COR_MBX(3)));

	writel(CMPR_ENDFCT_INIT,  REG_COR(CMPR_COR_IMSR));
	writel(CMPR_COR_IENR_BIT, REG_COR(CMPR_COR_IENR));

	writel(CMPR_CMN_XRST_BIT, REG_CMN(CMPR_CMN_XRST));
	return 0;
}

static void cmpr_deflate_end(int id)
{
	void const __iomem *cmpr_common = cmprs[id].common;

	writel(0, REG_CMN(CMPR_CMN_XRST));
	writel(0, REG_CMN(CMPR_CMN_CKEN));

	cmpr_unset_env(id);
}

static void cmpr_deflate_cancel(int id)
{
	void const __iomem *cmpr_core = cmprs[id].core;

	writel(CMPR_COR_MBX3_CANCEL, REG_COR(CMPR_COR_MBX(3)));
}

static uint32_t cmpr_deflate_is_end(int id)
{
	void const __iomem *cmpr_core = cmprs[id].core;
	u32 endfct;

	endfct = readl(REG_COR(CMPR_COR_IMSR));
	if (CMPR_ENDFCT_INIT == endfct)
		endfct = 0;

	return endfct;
}

static int cmpr_get_image_size(unsigned long image)
{
	struct wbi_lzp_hdr *head = (struct wbi_lzp_hdr *)image;
	struct wbi_lzp_entry *entry;

	entry = (struct wbi_lzp_entry *)
	            (image + sizeof(struct wbi_lzp_hdr) + head->tbl1_sz);
	entry--; /* last entry */

	return entry->off + entry->sz;
}

static bool cmpr_image_is_valid(unsigned long image)
{
	struct wbi_lzp_hdr *head = (struct wbi_lzp_hdr *)image;

	return head->signature == CMPR_LZPART_MAGIC;
}

static int cmpr_cmdline_parse(char *p, unsigned long *phys, unsigned long *size)
{
	*size = memparse(p, &p);
	if (!*size) return 1;

	if (*p++ != '@') return 1;

	*phys = memparse(p, &p);
	if (!*phys) return 1;

	return 0;
}

static int cmpr_init(void)
{
	ssize_t ret;
	unsigned long start, size, end;
	int i;

	if (cmpr_cmdline_parse(cmpr_mem, &start, &size)) {
		printk(KERN_ERR "cmpr: invalid cmdline:%s\n", cmpr_mem);
		return -EINVAL;
	}

	end = start + size;

	cmpr_pargs = (struct cmpr_parg *)ALIGN(start, CMPR_ALIGN);

	cmprs[0].mmu = (cmpr_mmu_t *)PTR_ALIGN(cmpr_pargs+N_CMPRS, CMPR_ALIGN);

	for (i = 1; i < N_CMPRS; i++)
	    cmprs[i].mmu = (cmpr_mmu_t *)PTR_ALIGN(cmprs[i-1].mmu+N_MMUPAGE, CMPR_ALIGN);

	cmpr_bin[0] = (unsigned long)PTR_ALIGN(cmprs[N_CMPRS-1].mmu+N_MMUPAGE, CMPR_ALIGN);
	if (end < cmpr_bin[0])
		return -EINVAL;

	if (!*cmpr_path[0]) {
		printk(KERN_ERR "ERROR:WBI:cmpr.path is not specified.\n");
		return -EINVAL;
	}
	ret = cmpr_load_file(cmpr_path[0], cmpr_bin[0], end-cmpr_bin[0]);
	if (ret < 0)
		return ret;

	if (!*cmpr_path[1]) {
		for (i = 0; i < N_CMPRS; i++) {
			cmpr_rsrc[i].firmid = 0; /* use cmpr.path */
		}
		return 0;
	}
	cmpr_bin[1] = (unsigned long)PTR_ALIGN(cmpr_bin[0]+ret, CMPR_ALIGN);
	ret = cmpr_load_file(cmpr_path[1], cmpr_bin[1], end-cmpr_bin[1]);
	if (ret < 0)
		return ret;

	return 0;
}

static int cmpr_deflate(int id,
			unsigned long src, unsigned long slen,
                        unsigned long dst, unsigned long dlen,
                        unsigned long work, unsigned long wlen)
{
	return cmpr_deflate_start(id, src, slen, dst, dlen, work, wlen);
}

static uint cmpr_deflate_poll(int id)
{
	return cmpr_deflate_is_end(id);
}

static int cmpr_deflate_finish(int id, unsigned long dst, unsigned long dlen)
{
	int ret = 0;
	u32 endfct;

	endfct = cmpr_deflate_is_end(id);
	if (CMPR_ENDFCT_DONE != endfct) {
		printk(KERN_ERR "cmpr: deflate abort: %d\n", endfct);
		ret = -EFAULT;
		goto out;
	}

	if (!cmpr_image_is_valid(dst)) {
		printk("cmpr: image is broken\n");
		ret = -ENOEXEC;
		goto out;
	}

	ret = cmpr_get_image_size(dst);

out:
	cmpr_deflate_end(id);

	return ret;
}

static void cmpr_exit(void)
{
	return ;
}

static struct wbi_compressor compressor = {
	.ncore = N_CMPRS,
	.init = cmpr_init,
	.deflate = cmpr_deflate,
	.deflate_poll = cmpr_deflate_poll,
	.deflate_cancel = cmpr_deflate_cancel,
	.deflate_end = cmpr_deflate_end,
	.deflate_finish = cmpr_deflate_finish,
	.exit = cmpr_exit,
	.get_max_slen = cmpr_get_max_slen,
};

static int __init cmpr_register_compressor(void)
{
	if (cmpr_ncore > 0  &&  cmpr_ncore < compressor.ncore) { /* override */
		compressor.ncore = cmpr_ncore;
	}
	return wbi_register_compressor(&compressor);
}

late_initcall(cmpr_register_compressor);
