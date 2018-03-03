/*
 * include/linux/udif/spinlock.h
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
#ifndef __UDIF_SPINLOCK_H__
#define __UDIF_SPINLOCK_H__

#include <linux/spinlock.h>
#include <linux/udif/macros.h>
#include <linux/udif/types.h>

typedef struct UDIF_SPINLOCK {
	spinlock_t lock;
} UDIF_SPINLOCK;

#define UDIF_SPINLOCK_INIT(name) { .lock = __SPIN_LOCK_UNLOCKED(name), }
#define UDIF_DECLARE_SPINLOCK(name) \
	UDIF_SPINLOCK name = UDIF_SPINLOCK_INIT(name)

typedef struct UDIF_RAW_SPINLOCK {
	raw_spinlock_t lock;
} UDIF_RAW_SPINLOCK;

#define UDIF_DECLARE_RAW_SPINLOCK(name) \
	UDIF_RAW_SPINLOCK name = { .lock = __RAW_SPIN_LOCK_UNLOCKED(name), }

#define __udif_spinlock(func, arg0, ...)					\
do {										\
	if (__builtin_types_compatible_p(typeof(arg0), raw_spinlock_t *))	\
		raw_##func((raw_spinlock_t *)arg0, ##__VA_ARGS__);		\
	else									\
		func((spinlock_t *)arg0, ##__VA_ARGS__);			\
} while (0)

#define udif_spin_lock_init(l)		__udif_spinlock(spin_lock_init, &(l)->lock)
#define udif_spin_lock(l)		__udif_spinlock(spin_lock, &(l)->lock)
#define udif_spin_unlock(l)		__udif_spinlock(spin_unlock, &(l)->lock)
#define udif_spin_lock_irqsave(l,f)	__udif_spinlock(spin_lock_irqsave, &(l)->lock, f)
#define udif_spin_unlock_irqrestore(l,f) __udif_spinlock(spin_unlock_irqrestore, &(l)->lock, f)
#define udif_local_irq_save(f)		local_irq_save(f)
#define udif_local_irq_restore(f)	local_irq_restore(f)

#endif /* __UDIF_SPINLOCK_H__ */
