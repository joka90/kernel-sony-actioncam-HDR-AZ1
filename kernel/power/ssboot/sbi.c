/* 2012-08-08: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot - SBI writer core
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
#include <linux/crc32.h>
#include "internal.h"
#include "sbi.h"

#define SBI_REGION_MAX_NUM	(PAGE_SIZE / sizeof(sbi_region_t))

static sbi_writer_t *sbi_writer;
static char *sbi_name = NULL;
static int sbi_compress = 0;
static int sbi_encrypt = 0;

/*
 * Image writer operations
 */
int
ssboot_sbi_prepare(void *priv)
{
	int ret = 0;

	if (sbi_writer == NULL) {
		ssboot_err("SBI writer is not registered\n");
		return -ENODEV;
	}
	if (sbi_writer->prepare != NULL) {
		ret = sbi_writer->prepare(sbi_writer->priv);
	}
	return ret;
}

int
ssboot_sbi_cleanup(void *priv)
{
	int ret = 0;

	if (sbi_writer->cleanup != NULL) {
		ret = sbi_writer->cleanup(sbi_writer->priv);
	}
	return ret;
}

int
ssboot_sbi_write(ssboot_image_t *image, void *priv)
{
	u_int8_t              *hdr;
	u_int8_t              *hdr_pos;
	size_t                 hdr_len;
	sbi_header_fixed_t    *hdr_fixed;
	sbi_header_end_t      *hdr_end;
	sbi_header_listinfo_t *hdr_listinfo;
#ifdef CONFIG_SNSC_SSBOOT_NO_KERNEL
	sbi_header_kernelid_t *hdr_kernelid;
#endif
	sbi_header_cmdlineinfo_t *hdr_cmdlineinfo;
	u_int8_t              *list;
	u_int8_t              *list_pos;
	u_int32_t              list_off = 0;
	u_int32_t              list_len = 0;
	u_int32_t              list_num = 0;
	sbi_list_header_t     *list_hdr;
	sbi_list_section_t    *list_sect;

	loff_t                 dst_off = 0;
	size_t                 dst_len = 0;
	u_int32_t              dst_num = 0;
	u_int16_t              mode;

	sbi_region_t          *reg;
	unsigned long          reg_num;

	unsigned long          i, j;
	int                    ret;

	/* allocate image header */
	hdr_len  = sizeof(sbi_header_fixed_t);
	hdr_len += sizeof(sbi_header_end_t);
	hdr_len += sizeof(sbi_header_listinfo_t);
#ifdef CONFIG_SNSC_SSBOOT_NO_KERNEL
	hdr_len += sizeof(sbi_header_kernelid_t);
#endif
	hdr_len += sizeof(sbi_header_cmdlineinfo_t);
	hdr = kmalloc(hdr_len, GFP_KERNEL);
	if(hdr == NULL){
		ssboot_err("cannot allocate buffer for SBI header\n");
		return -ENOMEM;
	}
	hdr_pos = hdr;

	/* allocate region list */
	reg = kmalloc(sizeof(sbi_region_t) * SBI_REGION_MAX_NUM, GFP_KERNEL);
	if(reg == NULL) {
		ssboot_err("cannot allocate buffer for region list\n");
		kfree(hdr);
		return -ENOMEM;
	}

	/* allocate section list */
	list = kmalloc((sizeof(sbi_list_header_t) +
			sizeof(sbi_list_section_t)) * image->num_section,
		       GFP_KERNEL);
	if(list == NULL) {
		ssboot_err("cannot allocate buffer for SBI section list\n");
		kfree(reg);
		kfree(hdr);
		return -ENOMEM;
	}
	list_pos = list;

	/* initialize SBI writer */
	ret = sbi_writer->initialize(sbi_writer->priv);
	if (ret < 0) {
		ssboot_err("failed to initialize SBI writer: %d\n", ret);
		goto out;
	}

	/* write dummy image header */
	memset(hdr, 0xff, hdr_len);
	ret = sbi_writer->write_data(SBI_DATA_PLAIN, dst_off, &dst_len,
				     hdr, hdr_len, sbi_writer->priv);
	if (ret < 0) {
		ssboot_err("failed to write SBI header (1): %d\n", ret);
		goto out;
	}
	dst_off += dst_len;

	/* write sections */
	for (i = 0; i < image->num_section; i += dst_num) {
		/* determine write mode */
		mode = SBI_DATA_PLAIN;
		if (sbi_compress) {
			mode |= SBI_DATA_COMPRESS;
		}
		if (sbi_encrypt) {
			mode |= SBI_DATA_ENCRYPT;
		}

		/* create region list */
		reg_num = ((i + SBI_REGION_MAX_NUM) < image->num_section ?
			   SBI_REGION_MAX_NUM : image->num_section - i);
		for (j = 0; j < reg_num; j++) {
			reg[j].start_pfn = image->section[i+j].writer_pfn;
			reg[j].num_pages = image->section[i+j].num_pages;
		}

		/* write section */
		ret = sbi_writer->write_pages(mode, dst_off, &dst_len,
					      &dst_num, reg, reg_num,
					      sbi_writer->priv);
		if (ret < 0) {
			ssboot_err("failed to write sections: %d\n", ret);
			goto out;
		}
		if ((dst_num == 0) || (dst_num + i > image->num_section)) {
			ssboot_err("invalid dst_num value: %d\n", dst_num);
			ret = -EIO;
			goto out;
		}

		/* generate section list */
		list_hdr = (sbi_list_header_t*)list_pos;
		list_hdr->data_off = dst_off;
		list_hdr->data_len = dst_len;
		list_hdr->num_sect = dst_num;
		list_pos += sizeof(sbi_list_header_t);
		list_sect = (sbi_list_section_t*)list_pos;
		for (j = 0; j < dst_num; j++) {
			list_sect->start_pfn = image->section[i+j].start_pfn;
			list_sect->num_pages = image->section[i+j].num_pages;
			list_sect++;
		}
		list_pos += sizeof(sbi_list_section_t) * dst_num;
		list_num++;
		dst_off += dst_len;
	}
	list_off = dst_off;

	/* write section list */
	mode = sbi_encrypt ? SBI_DATA_ENCRYPT : SBI_DATA_PLAIN;
	ret = sbi_writer->write_data(mode, dst_off, &dst_len, list,
				     list_pos - list, sbi_writer->priv);
	if (ret < 0) {
		ssboot_err("failed to write section list: %d\n", ret);
		goto out;
	}
	list_len = dst_len;
	dst_off += dst_len;

	/* fixed image header */
	memset(hdr, 0x00, hdr_len);
	hdr_fixed                = (sbi_header_fixed_t *)hdr_pos;
	hdr_fixed->magic         = SBI_HEADER_MAGIC;
	hdr_fixed->version       = SBI_HEADER_VERSION;
	hdr_fixed->header_len    = hdr_len;
	hdr_fixed->total_len     = dst_off;
	hdr_fixed->writer_id     = sbi_writer->writer_id;
	hdr_fixed->target_id     = image->target_id;
	hdr_fixed->image_attr    = 0;
#ifdef CONFIG_SNSC_SSBOOT_NO_KERNEL
	hdr_fixed->image_attr   |= SBI_IMAGE_NOKERNEL;
#endif
	hdr_fixed->entry_addr    = (unsigned long)image->entry_addr;
	hdr_pos += sizeof(sbi_header_fixed_t);

	/* section list information */
	hdr_listinfo             = (sbi_header_listinfo_t *)hdr_pos;
	hdr_listinfo->item_id    = SBI_HEADER_ITEM_LISTINFO_ID;
	hdr_listinfo->item_len   = SBI_HEADER_ITEM_LISTINFO_LEN;
	hdr_listinfo->list_off   = list_off;
	hdr_listinfo->list_len   = list_len;
	hdr_listinfo->list_num   = list_num;
	hdr_listinfo->sect_attr  = sbi_compress ? SBI_SECTION_COMPRESS : 0;
	hdr_listinfo->sect_attr |= sbi_encrypt  ? SBI_SECTION_ENCRYPT : 0;
	hdr_pos += sizeof(sbi_header_listinfo_t);

#ifdef CONFIG_SNSC_SSBOOT_NO_KERNEL
	/* kernel ID */
	hdr_kernelid             = (sbi_header_kernelid_t *)hdr_pos;
	hdr_kernelid->item_id    = SBI_HEADER_ITEM_KERNELID_ID;
	hdr_kernelid->item_len   = SBI_HEADER_ITEM_KERNELID_LEN;
	hdr_kernelid->kernel_id_srcaddr = (u_int32_t)linux_banner;
	hdr_kernelid->kernel_id_srclen  = strnlen(linux_banner, PAGE_SIZE);
	hdr_kernelid->kernel_id  = crc32(0, hdr_kernelid->kernel_id_srcaddr,
					 hdr_kernelid->kernel_id_srclen);
	hdr_pos += sizeof(sbi_header_kernelid_t);
#endif

	/* cmdline info */
	hdr_cmdlineinfo          = (sbi_header_cmdlineinfo_t *)hdr_pos;
	hdr_cmdlineinfo->item_id = SBI_HEADER_ITEM_CMDLINEINFO_ID;
	hdr_cmdlineinfo->item_len     = SBI_HEADER_ITEM_CMDLINEINFO_LEN;
	hdr_cmdlineinfo->cmdline_addr = virt_to_phys(image->cmdline);
	hdr_cmdlineinfo->cmdline_len  = SSBOOT_CMDLINE_SIZE;
	hdr_pos += sizeof(sbi_header_cmdlineinfo_t);

	/* terminator of image header */
	hdr_end                  = (sbi_header_end_t *)hdr_pos;
	hdr_end->item_id         = SBI_HEADER_ITEM_END_ID;
	hdr_end->item_len        = SBI_HEADER_ITEM_END_LEN;
	hdr_pos += sizeof(sbi_header_end_t);

	/* write image header */
	ret = sbi_writer->write_data(SBI_DATA_PLAIN,	0, &dst_len,
				     hdr, hdr_len, sbi_writer->priv);
	if (ret < 0) {
		ssboot_err("failed to write image header (2): %d\n", ret);
		goto out;
	}

	/* finalize SBI writer */
	ret = sbi_writer->finalize(dst_off, &dst_len, sbi_writer->priv);
	if (ret < 0) {
		ssboot_err("failed to finalize SBI writer: %d\n", ret);
		goto out;
	}
	dst_off += dst_len;
 out:
	/* free allocated memory */
	kfree(hdr);
	kfree(reg);
	kfree(list);

	return (ret < 0) ? -EIO : ret;
}

/*
 * proc I/F
 */
static int
sbiname_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
sbiname_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", sbi_name);
	return 0;
}

static int
sbiname_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* set sbiname */
	strncpy(sbi_name, ops->write_buf, SSBOOT_PATH_MAX);
	sbi_name[SSBOOT_PATH_MAX - 1] = '\0';

	return 0;
}
ssboot_single_proc(sbiname, SSBOOT_PROC_RDWR, SSBOOT_PATH_MAX);

static int
compress_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
compress_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", sbi_compress);
	return 0;
}

static int
compress_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	int mode = -1;

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* parse string */
	if (ops->write_len == 1) {
		mode = simple_strtoul(ops->write_buf, NULL, 0);
	}
	if (mode < 0 || mode > 1) {
		ssboot_err("invalid compress mode\n");
		return 0;
	}

	/* set compress mode */
	sbi_compress = mode;

	return 0;
}
ssboot_single_proc(compress, SSBOOT_PROC_RDWR, 4);

static int
encrypt_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
encrypt_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", sbi_encrypt);
	return 0;
}

static int
encrypt_release(struct inode *inode, struct file *file)
{
	ssboot_proc_ops_t *ops = ssboot_proc_get_ops_file(file);
	int mode = -1;

	/* check if data exists */
	if (ops->write_len == 0) {
		return 0;
	}

	/* parse string */
	if (ops->write_len == 1) {
		mode = simple_strtoul(ops->write_buf, NULL, 0);
	}
	if (mode < 0 || mode > 1) {
		ssboot_err("invalid encrypt mode\n");
		return 0;
	}

	/* set encrypt mode */
	sbi_encrypt = mode;

	return 0;
}
ssboot_single_proc(encrypt, SSBOOT_PROC_RDWR, 4);

/*
 * Exported functions for SBI writer
 */
int
ssboot_sbi_writer_register(sbi_writer_t *writer)
{
	/* sanity check */
	if (writer              == NULL ||
	    writer->initialize  == NULL ||
	    writer->write_data  == NULL ||
	    writer->write_pages == NULL ||
	    writer->finalize    == NULL) {
		ssboot_err("cannot register invalid SBI writer: %p\n",
			   writer);
		return -EINVAL;
	}

	/* check if already registered */
	if (sbi_writer != NULL) {
		ssboot_err("SBI writer is already registered\n");
		return -EEXIST;
	}

	/* setup sbiname */
	if (writer->default_sbiname != NULL) {
		sbi_name = (char*)kmalloc(SSBOOT_PATH_MAX, GFP_KERNEL);
		if (sbi_name == NULL) {
			ssboot_err("cannot allocate memory for sbiname\n");
			return -ENOMEM;
		}
		strncpy(sbi_name, writer->default_sbiname, SSBOOT_PATH_MAX);
		sbi_name[SSBOOT_PATH_MAX - 1] = '\0';
		ssboot_proc_create_entry("sbiname", &sbiname_ops,
					 ssboot_proc_root);
	}

	/* setup compress mode */
	if (writer->capability & SBI_WRITER_COMPRESS) {
		sbi_compress = 1;
		ssboot_proc_create_entry("compress", &compress_ops,
					 ssboot_proc_root);
	}

	/* setup encrypt mode */
	if (writer->capability & SBI_WRITER_ENCRYPT) {
		sbi_encrypt = 1;
		ssboot_proc_create_entry("encrypt", &encrypt_ops,
					 ssboot_proc_root);
	}

	/* register SBI writer */
	sbi_writer = writer;

	return 0;
}
EXPORT_SYMBOL(ssboot_sbi_writer_register);

int
ssboot_sbi_writer_unregister(sbi_writer_t *writer)
{
	/* sanity check */
	if (writer == NULL) {
		ssboot_err("cannot unregister invalid SBI writer: %p\n",
			   writer);
		return -EINVAL;
	}

	/* check if already unregistered */
	if (sbi_writer == NULL) {
		ssboot_err("SBI writer is already unregistered\n");
		return -ENOENT;
	}

	/* check unknown SBI writer */
	if (sbi_writer != writer) {
		ssboot_err("cannot unregister unknown SBI writer: %p\n",
			   writer);
		return -ENOENT;
	}

	/* remove 'sbiname' entry */
	if (writer->default_sbiname != NULL) {
		ssboot_proc_remove_entry("sbiname", ssboot_proc_root);
	}

	/* remove 'compress' entry */
	if (writer->capability & SBI_WRITER_COMPRESS) {
		ssboot_proc_remove_entry("compress", ssboot_proc_root);
	}

	/* remove 'encrypt' entry */
	if (writer->capability & SBI_WRITER_ENCRYPT) {
		ssboot_proc_remove_entry("encrypt", ssboot_proc_root);
	}

	/* free sbiname */
	if (sbi_name != NULL) {
		kfree(sbi_name);
	}

	/* unregister SBI writer */
	sbi_writer = NULL;
	sbi_name = NULL;
	sbi_compress = 0;
	sbi_encrypt = 0;

	return 0;
}
EXPORT_SYMBOL(ssboot_sbi_writer_unregister);

const char *
ssboot_sbi_get_sbiname(void)
{
	return sbi_name;
}
EXPORT_SYMBOL(ssboot_sbi_get_sbiname);

/*
 * Initialization
 */
static ssboot_writer_t ssboot_sbi_writer_core = {
	.prepare	= ssboot_sbi_prepare,
	.write		= ssboot_sbi_write,
	.cleanup	= ssboot_sbi_cleanup,
};

static int __init
ssboot_sbi_init_module(void)
{
	int ret;

	/* register SBI writer core */
	ret = ssboot_writer_register(&ssboot_sbi_writer_core);
	if (ret < 0) {
		return ret;
	}
	ssboot_dbg("registered SBI writer core\n");

	return 0;
}

static void __exit
ssboot_sbi_cleanup_module(void)
{
	int ret;

	/* unregister SBI writer core */
	ret = ssboot_writer_unregister(&ssboot_sbi_writer_core);
	if (ret < 0) {
		return;
	}
	ssboot_dbg("unregistered SBI writer core\n");
}

module_init(ssboot_sbi_init_module);
module_exit(ssboot_sbi_cleanup_module);

MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION("SBI writer core");
MODULE_LICENSE("GPL v2");
