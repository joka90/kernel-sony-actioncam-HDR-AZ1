/*
 * include/linux/udif/mutex.h
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
#ifndef __UDIF_MUTEX_H__
#define __UDIF_MUTEX_H__

#include <linux/mutex.h>
#include <linux/udif/types.h>
#include <linux/udif/macros.h>

typedef struct UDIF_MUTEX {
	struct mutex	mtx;
} UDIF_MUTEX;

#define UDIF_MUTEX_INIT(name) { .mtx = __MUTEX_INITIALIZER(name.mtx) }
#define UDIF_DECLARE_MUTEX(name) \
	UDIF_MUTEX name = UDIF_MUTEX_INIT(name)

#define udif_mutex_init(m)	UDIF_PARM_CHK_FN(m, mutex_init, &(m)->mtx)
#define udif_mutex_lock(m)	UDIF_PARM_CHK_FN(m, mutex_lock, &(m)->mtx)
#define udif_mutex_unlock(m)	UDIF_PARM_CHK_FN(m, mutex_unlock, &(m)->mtx)

#endif /* __UDIF_MUTEX_H__ */
