/* 2010-04-03: File added by Sony Corporation */
/*
 *  Snapshot Boot - SBI writer interface
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
#ifndef _LINUX_SSBOOT_SBI_H
#define _LINUX_SSBOOT_SBI_H

/* capability */
#define SBI_WRITER_COMPRESS	(1 << 0)
#define SBI_WRITER_ENCRYPT	(1 << 1)

/* write mode */
#define SBI_DATA_PLAIN		(0)
#define SBI_DATA_COMPRESS	(1 << 0)
#define SBI_DATA_ENCRYPT	(1 << 1)

/* writer id */
#define SBI_WRITERID_UNKNOWN	0xffff
#define SBI_WRITERID_FILE	0x0100

/* region list */
typedef struct sbi_region {
	u_int32_t start_pfn;
	u_int32_t num_pages;
} sbi_region_t;

/* SBI writer descriptor */
typedef struct sbi_writer {
	u_int16_t writer_id;
	u_int32_t capability;
	const char *default_sbiname;
	int (*prepare)(void *priv);
	int (*initialize)(void *priv);
	int (*write_data)(u_int32_t mode, loff_t dst_off, size_t *dst_len,
			  u_int8_t *src_buf, size_t src_len, void *priv);
	int (*write_pages)(u_int32_t mode, loff_t dst_off, size_t *dst_len,
			   u_int32_t *dst_num, sbi_region_t *src_list,
			   u_int32_t src_num, void *priv);
	int (*finalize)(loff_t dst_off, size_t *dst_len, void *priv);
	int (*cleanup)(void *priv);
	void *priv;
} sbi_writer_t;

/* SBI writer core interface */
const char * ssboot_sbi_get_sbiname(void);
int ssboot_sbi_writer_register(sbi_writer_t *writer);
int ssboot_sbi_writer_unregister(sbi_writer_t *writer);

#endif /* _LINUX_SSBOOT_SBI_H */
