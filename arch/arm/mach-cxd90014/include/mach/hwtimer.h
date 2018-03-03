/*
 * hwtimer.h
 *
 * Hardware timer definitions
 *
 * Copyright 2012 Sony Corporation
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


#ifndef __HWTIMER_H
#define __HWTIMER_H

#define TIMER_1MSEC 1000 /* 1msec = 1000 count */

#define HWTIMER_MULTICPU

/* #define HWTIMER_DEBUG */

struct hwtimer {
	void (*function)(unsigned long);
	unsigned long data;
	unsigned long expires;
	struct list_head entry;
	int cpuid;
	int pid;
#ifdef HWTIMER_DEBUG
	/* for debug */
	unsigned long start_time;
	unsigned long set_time;
	unsigned long spinlock_get;
	unsigned long starter;
#endif
};

extern unsigned long hwtimer_read_clock_cpu(int);
extern unsigned long hwtimer_read_clock(void);
extern void hwtimer_init_timer(struct hwtimer *, void (*func)(unsigned long), unsigned long, int);
extern void hwtimer_mod_timer(struct hwtimer *, unsigned long);
extern void hwtimer_del_timer(struct hwtimer *);
extern signed long hwtimer_schedule_timeout(unsigned long);
extern signed long hwtimer_usleep(unsigned long);

#endif /* __HWTIMER_H */
