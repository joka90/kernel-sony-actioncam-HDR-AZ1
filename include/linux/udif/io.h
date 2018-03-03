/*
 * include/linux/udif/io.h
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
#ifndef __UDIF_IO_H__
#define __UDIF_IO_H__

#include <linux/udif/string.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/udif/io.h>

#define UDIF_IO_FLAGS_NONCACHED	0x0000
#define UDIF_IO_FLAGS_CACHED	0x0001
#define UDIF_IO_FLAGS_SHARED	0x0002

typedef struct UDIF_IOMEM {
	UDIF_U8		name[8];
	UDIF_PA		phys;
	UDIF_SIZE	size;
	UDIF_VA		virt;
	UDIF_U32	flags;
} UDIF_IOMEM;

#define udif_iomem_virt(x) ((x)?(x)->virt:0)
#define udif_iomem_phys(x) ((x)?(x)->phys:0)
#define udif_iomem_size(x) ((x)?(x)->size:0)
#define udif_iomem_flags(x) ((x)?(x)->flags:(-1UL))

#define UDIF_IOMEM_INIT(n,p,s,f) \
{ \
	.name	= n, \
	.phys	= p, \
	.size	= s, \
	.virt	= UDIF_IO_ADDRESS(p), \
	.flags	= f, \
}
#define UDIF_DECLARE_IOMEM(name,n,p,s,f) \
	UDIF_IOMEM name = UDIF_IOMEM_INIT(n,p,s,f)

UDIF_VA  udif_ioremap(UDIF_IOMEM *);
UDIF_ERR udif_iounmap(UDIF_IOMEM *);

#define udif_ioread8(a)		readb(a)
#define udif_ioread16(a)	readw(a)
#define udif_ioread32(a)	readl(a)

#define udif_iowrite8(v,a)	writeb(v,a)
#define udif_iowrite16(v,a)	writew(v,a)
#define udif_iowrite32(v,a)	writel(v,a)

static inline void udif_iomem_init(UDIF_IOMEM *io, const UDIF_U8 *name, UDIF_PA phys,
				   UDIF_SIZE size, UDIF_U32 flags)
{
	__strncpy(io->name, name, sizeof(io->name));
	io->phys = phys;
	io->size = size;
	io->virt = UDIF_IO_ADDRESS(phys);
	io->flags = flags;
}

#endif /* __UDIF_IO_H__ */
