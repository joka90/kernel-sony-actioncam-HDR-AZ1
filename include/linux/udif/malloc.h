/*
 * include/linux/udif/malloc.h
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
#ifndef __UDIF_MALLOC_H__
#define __UDIF_MALLOC_H__

#include <linux/slab.h>

#define KMALLOC_FLAGS	GFP_KERNEL

extern unsigned long udif_km_debug;
extern UDIF_VP __udif_kmalloc(const UDIF_U8 *name, UDIF_UINT size, UDIF_ULONG flags);
extern UDIF_ERR __udif_kfree(UDIF_VP p);

static inline UDIF_VP udif_kmalloc(const UDIF_U8 *name, UDIF_UINT size, UDIF_ULONG flags)
{
	if (udif_km_debug)
		return __udif_kmalloc(name, size, flags | KMALLOC_FLAGS);
	else
		return kmalloc(size, flags | KMALLOC_FLAGS);
}

static inline UDIF_ERR udif_kfree(UDIF_VP p)
{
	if (udif_km_debug)
		return __udif_kfree(p);
	else {
		kfree(p);
		return UDIF_ERR_OK;
	}
}
#endif /* __UDIF_MALLOC_H__ */
