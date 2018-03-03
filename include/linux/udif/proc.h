/*
 * include/linux/udif/proc.h
 *
 *
 * Copyright 2010 Sony Corporation
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
 */
#ifndef __UDIF_PROC_H__
#define __UDIF_PROC_H__

#include <linux/udif/types.h>
#include <linux/udif/uaccess.h>
#include <linux/udif/print.h>

#define udif_proc_getdata(x)	((x) ? (x)->data : 0)
#define udif_proc_setend(x)	((x) ? *(x)->eof = 1 : 0)
#define udif_proc_bufsize(x)	((x) ? (x)->count : 0)
#define udif_proc_pos(x)	((x) ? *(x)->pos : 0)
#define udif_proc_pos_inc(x,n)	((x) ? *(x)->pos += (n) : 0)

#define udif_proc_getbuf(x,buf,cnt) \
({ \
	UDIF_SIZE len = 0; \
	if ((x)) { \
		if ((cnt) > (x)->count) \
			len = (x)->count; \
		else \
			len = (cnt); \
		len -= udif_copy_from_user(buf, (x)->uaddr, len); \
	} \
	len; \
})

#define udif_proc_setbuf(x,len,fmt,args...) \
	((x) && ((x)->count > (len)) ? \
	udif_snprintf((x)->kaddr + (len), (x)->count - (len), fmt, ##args) : 0)

typedef struct UDIF_PROC_READ {
	UDIF_U8		*kaddr;
	UDIF_UINT	count;
	UDIF_OFF	*pos;
	UDIF_INT	*eof;
	UDIF_VP		data;
} UDIF_PROC_READ;

typedef struct UDIF_PROC_WRITE {
	UDIF_USR_RO	uaddr;
	UDIF_UINT	count;
	UDIF_VP		data;
} UDIF_PROC_WRITE;

typedef	UDIF_INT (*UDIF_PROC_READ_CB)(UDIF_PROC_READ *);
typedef	UDIF_INT (*UDIF_PROC_WRITE_CB)(UDIF_PROC_WRITE *);

typedef struct UDIF_PROC {
	const UDIF_U8		*name;
	UDIF_PROC_READ_CB	read;
	UDIF_PROC_WRITE_CB	write;
	UDIF_UINT		flags;
	UDIF_VP			data;
} UDIF_PROC;

UDIF_ERR udif_create_proc(UDIF_PROC *);
UDIF_ERR udif_remove_proc(UDIF_PROC *);

#endif /*__UDIF_PROC_H__ */
