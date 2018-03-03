/*
 * arch/arm/mach-cxd90014/error.c
 *
 * ERROR Indicator
 *
 * Copyright 2010,2011,2012 Sony Corporation
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
#include <linux/moduleparam.h>
#include <mach/moduleparam.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/em_export.h>
#include <mach/errind.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>

#define ERRIND_USE_THREAD
#define ERRIND_USE_CONSOLE_CALLBACK

#ifdef ERRIND_USE_CONSOLE_CALLBACK
# ifndef CONFIG_SERIAL_AMBA2_CONSOLE
#  error "CONFIG_SERIAL_AMBA2_CONSOLE is not defined."
# endif
extern void (*amba2_console_idle_callback)(void);
#endif /* ERRIND_USE_CONSOLE_CALLBACK */

static unsigned long errind_led = 0xffffffffUL;
module_param_named(led, errind_led, port, S_IRUSR|S_IWUSR);

struct errind_ctrl {
	unsigned int intro;
	unsigned int on;
	unsigned int off;
	unsigned int interval;
	unsigned int ptnlen;
	char ptn[8];
};
static const struct errind_ctrl errind_user = { 1500, 200, 200,   0, 3,"337" };
static const struct errind_ctrl errind_oom  = { 1500,1000, 500,1500, 1,"3" };
static const struct errind_ctrl errind_oops = { 1500,  50,   0,   0, 1,"1" };
static const struct errind_ctrl *errind_ctrls[] = {
	&errind_user,
	&errind_oom,
	&errind_oops,
};

static int errind_event = ERRIND_NONE;
static const struct errind_ctrl *errind_ctl = NULL;

/* LED driver */
static void set_led(int on)
{
	unsigned int port, bit;

	if (0xffffffffUL == errind_led)
		return;

	port = errind_led & 0xff;
	bit  = (errind_led >> 8) & 0xff;

	if (on)
		writel_relaxed(1 << bit, VA_GPIO(port, WDATA) + GPIO_CLR);
	else
		writel_relaxed(1 << bit, VA_GPIO(port, WDATA) + GPIO_SET);
}

#ifdef ERRIND_USE_CONSOLE_CALLBACK
/* subroutine return value */
#define STATE_DONE 1
#define STATE_CONT 0

/*------------------ delay subroutine --------------------*/
static struct {
	enum {
		DELAY_INIT = 0,
		DELAY_WAIT,
		DELAY_DONE
	} state;
	unsigned long t;
} st_delay = { .state = DELAY_INIT, };

static int errind_delay(unsigned int ms)
{
	unsigned long now = mach_read_cycles();
	long dt;
	int ret = STATE_CONT;

	if (!ms) {
		return STATE_DONE;
	}
	switch (st_delay.state) {
	case DELAY_INIT:
		st_delay.t = now + mach_usecs_to_cycles(ms * 1000);
		st_delay.state = DELAY_WAIT;
		break;
	case DELAY_WAIT:
		dt = now - st_delay.t;
		if (dt < 0)
			break;
		st_delay.state = DELAY_DONE;
		break;
	case DELAY_DONE:
	default:
		st_delay.state = DELAY_INIT;
		ret = STATE_DONE;
		break;
	}
	return ret;
}
/*--------------------------------------------------------*/

struct state_main {
	enum {
		ST_INIT = 0,
		ST_INTRO_OFF,
		ST_INFINI_LOOP,
		ST_PTN_LOOP,
		ST_CHAR_LOOP,
		ST_OFF,
		ST_CHAR_LOOP_END,
		ST_PTN_LOOP_END
	} state;
	unsigned int pos;
	int cnt;
};
static struct state_main st_main = { .state = ST_INIT, };

static void errind_work(void)
{
	const struct errind_ctrl *p;
	unsigned int pos;

	if (!errind_event)
		return;

	p = errind_ctl;
	switch (st_main.state) {
	case ST_INIT:
		set_led(1);
		if (errind_delay(p->intro))
			st_main.state = ST_INTRO_OFF;
		break;
	case ST_INTRO_OFF:
		set_led(0);
		if (errind_delay(p->on + p->off))
			st_main.state = ST_INFINI_LOOP;
		break;
	case ST_INFINI_LOOP:
		st_main.pos = 0;
		st_main.state = ST_PTN_LOOP;
		break;
	case ST_PTN_LOOP: /*=========== ptn[] loop ==============*/
		pos = st_main.pos;
		if (pos >= p->ptnlen) {
			st_main.state = ST_PTN_LOOP_END;
			break;
		}
		st_main.cnt = p->ptn[pos] - '0';
		st_main.pos = pos + 1;
		st_main.state = ST_CHAR_LOOP;
		break;
	case ST_CHAR_LOOP: /*----- On/Off ptn[pos] times -----*/
		if (st_main.cnt <= 0) {
			st_main.state = ST_CHAR_LOOP_END;
			break;
		}
		set_led(1);
		if (errind_delay(p->on))
			st_main.state = ST_OFF;
		break;
	case ST_OFF:
		set_led(0);
		if (errind_delay(p->off)) {
			st_main.cnt--;
			st_main.state = ST_CHAR_LOOP;
		}
		break;
	case ST_CHAR_LOOP_END: /*-----------------------------*/
		if (errind_delay(p->on + p->off))
			st_main.state = ST_PTN_LOOP;
		break;
	case ST_PTN_LOOP_END:  /*================================*/
		if (errind_delay(p->interval))
			st_main.state = ST_INFINI_LOOP;
		break;
	default:
		st_main.state = ST_INIT;
		break;
	}
}
#endif /* ERRIND_USE_CONSOLE_CALLBACK */

#ifdef ERRIND_USE_THREAD
static struct task_struct *errind_task = NULL;
static unsigned int errind_task_event = ERRIND_NONE;

static void errind_sequencer(int code)
{
	const struct errind_ctrl *ctl;
	int pos, cnt;

	if (code < 1  ||  ERRIND_MAX < code) {
		printk(KERN_ERR "errind_sequncer:ERROR:code=%d\n", code);
		return;
	}
	ctl = errind_ctrls[code - 1];

	set_led(1);
	msleep(ctl->intro);
	set_led(0);
	msleep(ctl->on + ctl->off);
	while (1) {
		for (pos = 0; pos < ctl->ptnlen; pos++) {
			cnt = ctl->ptn[pos] - '0';
			while (cnt > 0) {
				set_led(1);
				msleep(ctl->on);
				set_led(0);
				msleep(ctl->off);
				cnt--;
			}
			msleep(ctl->on + ctl->off);
		}
		msleep(ctl->interval);
	}
	/* never reach */
}

static int errind_thread(void *data)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		if (kthread_should_stop())
			break;
		if (errind_task_event) {
			errind_sequencer(errind_task_event);
		}
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

void errind_start(int code)
{
	if (!errind_task)
		return;
	errind_task_event = code;
	wmb();
	wake_up_process(errind_task);
}
#endif /* ERRIND_USE_THREAD */

static void errind_handler(struct pt_regs *regs)
{
	int code;

	/* analyze */
	code = ERRIND_OOPS;
	if (user_mode(regs)) {
		code = ERRIND_USER;
		if (test_tsk_thread_flag(current, TIF_MEMDIE)) {
			code = ERRIND_OOM;
		}
	}

	/* local indicator */
	errind_event = code;
	errind_ctl = errind_ctrls[code - 1];
	wmb();
#ifdef ERRIND_USE_CONSOLE_CALLBACK
	amba2_console_idle_callback = errind_work;
#endif /* ERRIND_USE_CONSOLE_CALLBACK */
}

static int __init errind_init(void)
{
#ifdef ERRIND_USE_THREAD
	struct task_struct *th;

	th = kthread_create(errind_thread, NULL, "errind");
	if (IS_ERR(th)) {
		printk(KERN_ERR "errind:ERROR:kthread_create failed.\n");
	} else {
		wake_up_process(th);
		errind_task = th;
	}
#endif /* ERRIND_USE_THREAD */

	em_hook = errind_handler;

	return 0;
}

static void __exit errind_exit(void)
{
#ifdef ERRIND_USE_THREAD
	struct task_struct *th;

	if ((th = errind_task) != NULL) {
		errind_task = NULL;
		kthread_stop(th);
	}
#endif /* ERRIND_USE_THREAD */

	em_hook = NULL;
}

module_init(errind_init);
module_exit(errind_exit);
