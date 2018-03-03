/* 2012-07-26: File added by Sony Corporation */
/*
 *  Snapshot Boot - SBI file writer
 *
 *  Copyright 2008,2009,2010 Sony Corporation
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
#include <linux/module.h>
#include <linux/pfn.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/statfs.h>
#include <linux/uaccess.h>
#include <linux/ssboot.h>
#include <linux/ssboot_sbi.h>

/* alignment size */
#define SSBOOT_SBI_FILE_ALIGN_LEN	PAGE_SIZE

/* buffer for alignment */
static u_int8_t *buf;

/* for file access */
static struct file *filp;
static mm_segment_t oldfs;

static void
ssboot_sbi_file_close(void)
{
	/* close file */
	fput(filp);
	filp_close(filp, current->files);

	/* restore setting */
	set_fs(oldfs);
}

static int
ssboot_sbi_file_prepare(void *priv)
{
	const char *sbiname;

	/* invalidate page cache associated with image file */
	sbiname = ssboot_sbi_get_sbiname();
	if (sbiname != NULL) {
		ssboot_invalidate_page_cache(sbiname);
	}

	/* allocate buffer for write_data() method */
	buf = kmalloc(SSBOOT_SBI_FILE_ALIGN_LEN, GFP_KERNEL);
	if (buf == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static int
ssboot_sbi_file_initialize(void *priv)
{
	const char *sbiname;

	/* get sbiname */
	sbiname = ssboot_sbi_get_sbiname();
	if (sbiname == NULL) {
		return -ENOENT;
	}

	/* open file */
	filp = filp_open(sbiname, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0644);
	if (IS_ERR(filp)) {
		ssboot_err("cannot open file: %s\n", sbiname);
		return PTR_ERR(filp);
	}
	get_file(filp);

	/* for kernel address */
	oldfs = get_fs();
	set_fs(get_ds());

	return 0;
}

static int
ssboot_sbi_file_write_data(u_int32_t mode, loff_t dst_off, size_t *dst_len,
			   u_int8_t *src_buf, size_t src_len, void *priv)
{
	size_t align_len;
	ssize_t ret;

	*dst_len = 0;

	/* write aligned size */
	align_len = src_len & ~(SSBOOT_SBI_FILE_ALIGN_LEN - 1);
	if (align_len > 0) {
		/* write data to file */
		ret = vfs_write(filp, src_buf, align_len, &dst_off);
		if (ret < 0) {
			ssboot_sbi_file_close();
			return ret;
		}
		src_buf += align_len;
		src_len -= align_len;
		*dst_len += (size_t)ret;
	}

	/* write remainder */
	if (src_len > 0) {
		/* copy to aligned buffer */
		memcpy(buf, src_buf, src_len);
		memset(buf + src_len, 0xff, SSBOOT_SBI_FILE_ALIGN_LEN - src_len);

		/* write data to file */
		ret = vfs_write(filp, buf, SSBOOT_SBI_FILE_ALIGN_LEN, &dst_off);
		if (ret < 0) {
			ssboot_sbi_file_close();
			return ret;
		}
		*dst_len += (size_t)ret;
	}
	return 0;
}

static int
ssboot_sbi_file_write_pages(u_int32_t mode, loff_t dst_off, size_t *dst_len,
			    u_int32_t *dst_num, sbi_region_t *src_list,
			    u_int32_t src_num, void *priv)
{
	void *src_addr;
	size_t src_len, total_len = 0;
	u_int32_t i, pct, pct_old = 0;
	ssize_t ret;

	ssboot_info("Writing sections .");

	/* write all regions */
	for (i = 0; i < src_num; i++, src_list++) {

		/* setup address and length */
		src_addr = ssboot_pfn_to_virt(src_list->start_pfn);
		src_len = PFN_PHYS(src_list->num_pages);

		/* write pages to file */
		ret = vfs_write(filp, src_addr, src_len, &dst_off);
		if (ret < 0) {
			ssboot_sbi_file_close();
			return ret;
		}
		total_len += ret;

		/* show percent complete */
		pct = i * 100 / src_num;
		if (pct - pct_old >= 10) {
			printk(".");
			pct_old = pct;
		}
	}
	printk(" done.\n");

	*dst_len = total_len;
	*dst_num = src_num;

	return 0;
}

static int
ssboot_sbi_file_finalize(loff_t dst_off, size_t *dst_len, void *priv)
{
	/* close file */
	ssboot_sbi_file_close();

	/* no parameter */
	*dst_len = 0;

	return 0;
}

static int
ssboot_sbi_file_cleanup(void *priv)
{
	/* free buffer */
	kfree(buf);

	return 0;
}

static sbi_writer_t ssboot_sbi_file_writer = {
	.writer_id		= SBI_WRITERID_FILE,
	.capability		= 0,
	.default_sbiname	= CONFIG_SNSC_SSBOOT_SBI_FILE_NAME,
	.prepare		= ssboot_sbi_file_prepare,
	.initialize		= ssboot_sbi_file_initialize,
	.write_data		= ssboot_sbi_file_write_data,
	.write_pages		= ssboot_sbi_file_write_pages,
	.finalize		= ssboot_sbi_file_finalize,
	.cleanup		= ssboot_sbi_file_cleanup,
};

static int __init
ssboot_sbi_file_init_module(void)
{
	int ret;

	ret = ssboot_sbi_writer_register(&ssboot_sbi_file_writer);
	if (ret < 0) {
		return ret;
	}
	ssboot_dbg("registered SBI file writer\n");

	return 0;
}

static void __exit
ssboot_sbi_file_cleanup_module(void)
{
	int ret;

	ret = ssboot_sbi_writer_unregister(&ssboot_sbi_file_writer);
	if (ret < 0) {
		return;
	}
	ssboot_dbg("unregistered SBI file writer\n");
}

module_init(ssboot_sbi_file_init_module);
module_exit(ssboot_sbi_file_cleanup_module);

MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION("SBI file writer");
MODULE_LICENSE("GPL v2");
