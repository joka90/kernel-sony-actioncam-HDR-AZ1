/*
 * fs/partitions/sdm_partition_table.h
 *
 * Definitions of the SDM partiton table
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
 */

#ifndef __SDM_PARTITION_TABLE_H__
#define __SDM_PARTITION_TABLE_H__

#include <linux/types.h>

/* kernel sector size */
#define SECTOR_SHIFT  9  /* 512 bytes */
#define SECTOR_SIZE   (1 << SECTOR_SHIFT)

#define SDM_LABEL_N_PARTITION 30

typedef struct {
	__u32 magic;     /* Magic number */
	__u32 version;
    	__u32 n_partition;
	__u8 spare[20];
	struct sdm_partition {
		__u32 start; /* byte offset */
		__u32 size;  /* byte size */
		__u32 type;
		__u32 flag;
	} __attribute__ ((packed)) partitions[SDM_LABEL_N_PARTITION];
} sdm_partition_tbl;

#define SDM_LABEL_MAGIC		0x36343238
#define SDM_LABEL_VERSION	0x30302e31
/* flag */
#define SDM_LABEL_VALID		0x00000001

#endif /* __SDM_PARTITION_TABLE_H__ */
