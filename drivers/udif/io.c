/*
 * drivers/udif/io.c
 *
 * UDM
 *
 * Copyright 2012,2013 Sony Corporation
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
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/udif/types.h>
#include <linux/udif/io.h>
#include <linux/udif/mutex.h>
#include <linux/udif/macros.h>
#include <asm/io.h>

#define UDIF_IO_FLAGS_REMAPPED	0x80000000

static UDIF_DECLARE_MUTEX(mutex);

static UDIF_VA __udif_ioremap(UDIF_IOMEM *iomem)
{
	void __iomem *p;

	if (!(iomem->flags & UDIF_IO_FLAGS_SHARED) &&
	    !request_mem_region(iomem->phys, iomem->size, iomem->name)) {
		UDIF_PERR("%s failed request_mem_region(), phys = 0x%lx, size = 0x%x\n",
			   iomem->name, iomem->phys, iomem->size);
		return NULL;
	}

	if (iomem->flags & UDIF_IO_FLAGS_CACHED)
		p = ioremap_cached(iomem->phys, iomem->size);
	else
		p = ioremap_nocache(iomem->phys, iomem->size);

	if (unlikely(!p))
		UDIF_PERR("%s failed ioremap(), phys = 0x%lx, size = 0x%x, flags = 0x%x\n",
			   iomem->name, iomem->phys, iomem->size, iomem->flags);

	return p;
}

static void __udif_iounmap(UDIF_IOMEM *iomem)
{
	iounmap(iomem->virt);

	if (!(iomem->flags & UDIF_IO_FLAGS_SHARED)) {
		release_mem_region(iomem->phys, iomem->size);
	}
}

UDIF_VA udif_ioremap(UDIF_IOMEM *iomem)
{
	UDIF_VA p;

	UDIF_PARM_CHK(!iomem, "invalid iomem", NULL);

	udif_mutex_lock(&mutex);

	if (unlikely(iomem->flags & UDIF_IO_FLAGS_REMAPPED)) {
		UDIF_PERR("%s already ioremapped, phys = 0x%lx, size = 0x%x, flags = 0x%x\n",
			   iomem->name, iomem->phys, iomem->size, iomem->flags);

		udif_mutex_unlock(&mutex);
		return 0;
	}

	if (!iomem->virt && iomem->size) {
		iomem->virt = __udif_ioremap(iomem);
		if (likely(iomem->virt))
			iomem->flags |= UDIF_IO_FLAGS_REMAPPED;
	}

	p = iomem->virt;

	udif_mutex_unlock(&mutex);

	return p;
}

UDIF_ERR udif_iounmap(UDIF_IOMEM *iomem)
{
	UDIF_PARM_CHK(!iomem, "invalid iomem", UDIF_ERR_PAR);

	udif_mutex_lock(&mutex);

	if (iomem->flags & UDIF_IO_FLAGS_REMAPPED) {
		__udif_iounmap(iomem);
		iomem->flags &= ~UDIF_IO_FLAGS_REMAPPED;
		iomem->virt = 0;
	}

	udif_mutex_unlock(&mutex);

	return UDIF_ERR_OK;
}

EXPORT_SYMBOL(udif_ioremap);
EXPORT_SYMBOL(udif_iounmap);
