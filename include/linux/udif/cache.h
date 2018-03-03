/*
 * include/linux/udif/cache.h
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
#ifndef __UDIF_CACHE_H__
#define __UDIF_CACHE_H__

#include <linux/dma-mapping.h>
#include <linux/udif/types.h>
#include <linux/udif/macros.h>
#include <asm/dma-mapping.h>

typedef enum {
	UDIF_CACHE_BIDRECTIONAL	= DMA_BIDIRECTIONAL,
	UDIF_CACHE_FLUSH	= DMA_TO_DEVICE,
	UDIF_CACHE_INVALIDATE	= DMA_FROM_DEVICE,
	UDIF_CACHE_NONE		= DMA_NONE,
} UDIF_CACHE_FLAGS;

static inline void udif_cache_ctrl(UDIF_VA va, UDIF_SIZE size, UDIF_CACHE_FLAGS dir)
{
	if (unlikely(!va || !size)) {
		UDIF_PERR("invalid param: addr:0x%p, size:0x%08x\n", va, size);
		return;
	}

	dma_sync_single_for_device(NULL, virt_to_phys(va), size, dir);
}

#define udif_cache_ctrl_r(a,s,f) udif_cache_ctrl(a,s,f)

#endif /* __UDIF_CACHE_H__ */
