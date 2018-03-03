/*
 * arch/arm/common/wbi.h
 *
 * WBI header
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

#ifndef __WBI_H__
#define __WBI_H__

#define RU_SIZE(len, alignment) (((len) + (alignment) -1)&~((alignment)-1))
#define RU_SECTOR_SIZE(len) RU_SIZE(len, wb_default_device->phys_sector_size)
#define INIT_CKSUM(cksum) do{(cksum) = 0x0F0F0F0F;}while(0)
#define wb_msg(fmt,arg...)                                   \
        do{                                                    \
	        if(!(wb_default_device->flag&WBI_SILENT_MODE)) \
                        printk(fmt, ##arg);                    \
	}while(0);
extern struct wb_device *wb_default_device;
extern struct wbi_compressor *wbi_default_compressor;
extern struct wb_header wbheader;

extern void wb_printk_init(void);
extern int wb_printk(const char *fmt, ...);
extern int wbi_is_canceled(void);
extern int wbi_is_timeout(void);
extern int wbi_section_idx(struct wb_section *p);
extern void wbi_calc_cksum(ulong *cksum, ulong *buf, ulong size);
extern void wbi_recalc_cksum(ulong *cksum, ulong *buf, ulong size);

/* WBI compress sequencer API */
extern int wbi_setup(ulong work, ulong *wlen, ulong dst, ulong *dlen);
extern int wbi_send_data(void *b, ulong len);
extern int wbi_send_sections(struct wb_header *header, struct wb_section *s, ulong n);
extern void wbi_align_sector(void);
extern int wbi_flush(void);
extern int wbi_rewrite_header(struct wb_header *header);
extern int wbi_read_header(struct wb_header *header);
extern void wbi_stat(void);

#endif /* __WBI_H__ */
