/*
 * include/linux/udif/tasklet.h
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
#ifndef __UDIF_TASKLET_H__
#define __UDIF_TASKLET_H__

#include <linux/interrupt.h>
#include <linux/udif/macros.h>
#include <linux/udif/types.h>

typedef struct UDIF_TASKLET {
	struct tasklet_struct tsk;
} UDIF_TASKLET;

typedef	void (*UDIF_TASKLET_CB)(UDIF_ULONG);

#define UDIF_TASKLET_INIT(f,d) { .tsk = { NULL,0,ATOMIC_INIT(0),f,d }, }
#define UDIF_DECLARE_TASKLET(name,f,d) \
	UDIF_TASKLET name = UDIF_TASKLET_INIT(f,d)

#define udif_tasklet_init(t,f,d)	UDIF_PARM_CHK_FN(t, tasklet_init, &(t)->tsk, f, d)
#define udif_tasklet_schedule(t)	UDIF_PARM_CHK_FN(t, tasklet_schedule, &(t)->tsk)
#define udif_tasklet_kill(t)		UDIF_PARM_CHK_FN(t, tasklet_kill, &(t)->tsk)

#endif /* __UDIF_TASKLET_H__ */
