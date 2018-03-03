/*
 * include/linux/udif/types.h
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
#ifndef __UDIF_TYPES_H__
#define __UDIF_TYPES_H__

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/init.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>

typedef enum {
	UDIF_ERR_OK	= 0,
	UDIF_ERR_NOENT	= -ENOENT,
	UDIF_ERR_IO	= -EIO,
	UDIF_ERR_NOMEM	= -ENOMEM,
	UDIF_ERR_ACCES	= -EACCES,
	UDIF_ERR_FAULT	= -EFAULT,
	UDIF_ERR_BUSY	= -EBUSY,
	UDIF_ERR_DEV	= -ENODEV,
	UDIF_ERR_PAR	= -EINVAL,
	UDIF_ERR_NOTTY	= -ENOTTY,
	UDIF_ERR_NOSPC	= -ENOSPC,
	UDIF_ERR_NOSYS	= -ENOSYS,
	UDIF_ERR_TIMEOUT = -ETIMEDOUT,
} UDIF_ERR;

typedef signed char		UDIF_S8;
typedef unsigned char		UDIF_U8;

typedef signed short		UDIF_S16;
typedef unsigned short		UDIF_U16;

typedef signed int		UDIF_S32;
typedef unsigned int		UDIF_U32;

typedef signed long long	UDIF_S64;
typedef unsigned long long	UDIF_U64;

typedef char			UDIF_CHAR;
typedef unsigned char		UDIF_UCHAR;

typedef short			UDIF_SHORT;
typedef unsigned short		UDIF_USHORT;

typedef long			UDIF_LONG;
typedef unsigned long		UDIF_ULONG;

typedef int			UDIF_INT;
typedef unsigned int		UDIF_UINT;

typedef void 			*UDIF_VP;

typedef UDIF_U32		UDIF_CH;
typedef UDIF_U32		UDIF_IRQ;

typedef size_t			UDIF_SIZE;
typedef ssize_t			UDIF_SSIZE;
typedef off_t			UDIF_OFF;
typedef loff_t			UDIF_LOFF;

typedef char __user		*UDIF_USR_RW;
typedef const char __user	*UDIF_USR_RO;

typedef unsigned long		UDIF_PA;
typedef void __iomem		*UDIF_VA;

typedef dev_t			UDIF_DEVNO;
typedef sector_t		UDIF_SECTOR;

#define UDIF_INIT	__init
#define UDIF_INITDATA	__initdata

#define UDIF_EXIT	__exit
#define UDIF_EXITDATA	__exitdata

#endif /* __UDIF_TYPES_H__ */
