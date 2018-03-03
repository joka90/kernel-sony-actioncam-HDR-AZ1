/*
 * cxd4132_clock.h
 *
 * CLOCK definitions
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
#ifndef __MACH_CXD4132_INCLUDE_MACH_CXD4132_CLOCK_H
#define __MACH_CXD4132_INCLUDE_MACH_CXD4132_CLOCK_H

struct cxd4132_clk_info {
	unsigned long cpu;
	unsigned long fsp;
	unsigned long twd_MHz, twd_per_tick;
};

#define CXD4132_N_SPEED		(2)
#define CXD4132_SPEED_HI	(0)
#define CXD4132_SPEED_LO	(1)

extern struct cxd4132_clk_info *cxd4132_get_clk_info(void);
extern unsigned long cxd4132_cpuclk(void);
extern unsigned long cxd4132_fspclk(void);
extern unsigned long cxd4132_localtimer_rate(void);
extern void cxd4132_set_speed(unsigned int);
extern unsigned int cxd4132_get_speed(void);

#endif /* __MACH_CXD4132_INCLUDE_MACH_CXD4132_CLOCK_H */
