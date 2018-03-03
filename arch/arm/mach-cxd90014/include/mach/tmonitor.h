/*
 * arch/arm/plat-cxd41xx/include/mach/tmonitor.h
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
#ifndef __PLAT_CXD41XX_INCLUDE_MACH_TMONITOR_H
#define __PLAT_CXD41XX_INCLUDE_MACH_TMONITOR_H

#ifdef CONFIG_THREAD_MONITOR
extern int trace_cpu;
#define TMON_TRACE_ALL		0x7fffffff
extern int tmon_autostop;
#define TMON_AUTOSTOP_NONE	0
#define TMON_AUTOSTOP_STR	0x01
#define TMON_AUTOSTOP_DELAY	0x02
#define TMON_AUTOSTOP_DELAY2	0x04

extern void add_trace_entry(int cpu, struct task_struct *prev,
			    struct task_struct *next, int data);
extern void tmonitor_write(const char *buffer);
extern void tmonitor_stop(void);
extern void tmonitor_boot_time_notify(char *);
extern void tmonitor_timer_notify(void);

static inline void tmonitor_tick(void)
{
	if (unlikely(TMON_AUTOSTOP_DELAY & tmon_autostop)) {
		tmonitor_timer_notify();
	}
}

#else /* CONFIG_THREAD_MONITOR */

static inline void tmonitor_tick(void) { }
static inline void tmonitor_boot_time_notify(char *) { }

#endif /* CONFIG_THREAD_MONITOR */
#endif /* __PLAT_CXD41XX_INCLUDE_MACH_TMONITOR_H */
