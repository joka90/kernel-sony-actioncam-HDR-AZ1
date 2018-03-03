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
#ifndef _SSBOOT_SBI_H
#define _SSBOOT_SBI_H

#include <linux/types.h>
#include <linux/ssboot_sbi.h>

/* fixed header */
#define SBI_HEADER_MAGIC		0x53624968  /* "SbIh" */
#define SBI_HEADER_VERSION		0x0100      /* 1.0 */
#define SBI_IMAGE_NOKERNEL		(1 << 0)

typedef struct sbi_header_fixed {
	u_int32_t magic;
	u_int16_t version;
	u_int16_t header_len;
	u_int32_t total_len;
	u_int16_t writer_id;
	u_int16_t target_id;
	u_int32_t image_attr;
	u_int32_t entry_addr;
	u_int32_t reserved[2];
} __attribute__((packed)) sbi_header_fixed_t;

/* end of header */
#define SBI_HEADER_ITEM_END_ID		0xffff
#define SBI_HEADER_ITEM_END_LEN		0

typedef struct sbi_header_end {
	u_int16_t item_id;
	u_int16_t item_len;
} __attribute__((packed)) sbi_header_end_t;

/* list info */
#define SBI_HEADER_ITEM_LISTINFO_ID	0x0010
#define SBI_HEADER_ITEM_LISTINFO_LEN	16
#define SBI_SECTION_COMPRESS		(1 << 0)
#define SBI_SECTION_ENCRYPT		(1 << 1)

typedef struct sbi_header_listinfo {
	u_int16_t item_id;
	u_int16_t item_len;
	u_int32_t list_off;
	u_int32_t list_len;
	u_int32_t list_num;
	u_int32_t sect_attr;
} __attribute__((packed)) sbi_header_listinfo_t;

/* kernel id */
#define SBI_HEADER_ITEM_KERNELID_ID	0x0020
#define SBI_HEADER_ITEM_KERNELID_LEN	12

typedef struct sbi_header_kernelid {
	u_int16_t item_id;
	u_int16_t item_len;
	u_int32_t kernel_id;
	u_int32_t kernel_id_srcaddr;
	u_int32_t kernel_id_srclen;
} __attribute__((packed)) sbi_header_kernelid_t;

/* cmdline info */
#define SBI_HEADER_ITEM_CMDLINEINFO_ID	0x0030
#define SBI_HEADER_ITEM_CMDLINEINFO_LEN	8

typedef struct sbi_header_cmdlineinfo {
	u_int16_t item_id;
	u_int16_t item_len;
	u_int32_t cmdline_addr;
	u_int32_t cmdline_len;
} __attribute__((packed)) sbi_header_cmdlineinfo_t;

/* section list */
typedef struct sbi_list_header {
	u_int32_t data_off;
	u_int32_t data_len;
	u_int32_t num_sect;
} __attribute__((packed)) sbi_list_header_t;

typedef struct sbi_list_section {
	u_int32_t start_pfn;
	u_int32_t num_pages;
} __attribute__((packed)) sbi_list_section_t;

#endif /* _SSBOOT_SBI_H */
