/*
 * include/linux/udif/wait.h
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
#ifndef __UDIF_WAIT_H__
#define __UDIF_WAIT_H__

#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/udif/types.h>
#include <linux/udif/macros.h>

/* simple wait */
typedef struct UDIF_WAIT {
	struct simple_wait wait;
} UDIF_WAIT;

#define UDIF_WAIT_INIT { .wait = SIMPLE_WAIT_INIT, }
#define UDIF_DECLARE_WAIT(name) \
	UDIF_WAIT name = UDIF_WAIT_INIT

#define udif_wait_init(w)	UDIF_PARM_CHK_FN(w, init_simple_wait, &(w)->wait)
#define udif_wake_up(w)		UDIF_PARM_CHK_FN(w, wake_up_task, &(w)->wait)
#define udif_wait_event(w, c)	task_wait_event(&(w)->wait, c)
#define udif_wait_event_timeout(w, c, tmo) \
({ \
	task_wait_event_timeout(&(w)->wait, c, (tmo) * HZ) ? UDIF_ERR_TIMEOUT : \
	UDIF_ERR_OK; \
})

/* wait queue head for poll */
typedef wait_queue_head_t UDIF_WAIT_QUEUE_HEAD;

#define UDIF_WAIT_QUEUE_INIT(name)	__WAIT_QUEUE_HEAD_INITIALIZER(name)
#define UDIF_DECLARE_WAIT_QUEUE(name)	DECLARE_WAIT_QUEUE_HEAD(name)

#define udif_wait_queue_init(wq)			init_waitqueue_head(wq)
#define udif_wait_queue_wake_up(wq)			wake_up(wq)
#define udif_wait_queue_wake_up_interruptible(wq)	wake_up_interruptible(wq)
#define udif_wait_queue_wake_up_all(wq)			wake_up_all(wq)
#define udif_wait_queue_event(wq, c)			wait_event(wq, c)
#define udif_wait_queue_event_timeout(wq, c, tmo) \
({ \
	wait_event_timeout(wq, c, (tmo) * HZ) ? UDIF_ERR_OK : \
	UDIF_ERR_TIMEOUT; \
})

#endif /* __UDIF_WAIT_H__ */
