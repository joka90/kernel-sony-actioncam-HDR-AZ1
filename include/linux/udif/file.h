/*
 * include/linux/udif/file.h
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
#ifndef __UDIF_FILE_H__
#define __UDIF_FILE_H__

#include <linux/ioctl.h>
#include <linux/udif/types.h>

#define udif_file_data(x)	(*(x)->data)
#define udif_file_major(x)	((x) ? MAJOR((x)->devno): 0)
#define udif_file_minor(x)	((x) ? MINOR((x)->devno): 0)
#define udif_file_bdev(x)	((x) ? (x)->bdev: 0)

#define UDIF_MKDEV(m, n)	MKDEV(m, n)

typedef struct UDIF_FILE {
	UDIF_DEVNO	devno;
	UDIF_VP		*data;
} UDIF_FILE;

typedef struct UDIF_IOCTL {
	UDIF_UINT	cmd;
	UDIF_ULONG	arg;
	UDIF_UINT	disk_change;
} UDIF_IOCTL;

typedef	UDIF_ERR (*UDIF_FILE_CB)(UDIF_FILE *);
typedef	UDIF_ERR (*UDIF_IOCTL_CB)(UDIF_FILE *, UDIF_IOCTL *);

#endif /* __UDIF_FILE_H__ */
