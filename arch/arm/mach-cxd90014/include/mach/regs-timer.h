/*
 * arch/arm/mach-cxd90014/include/mach/regs-timer.h
 *
 * Timer registers
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
#ifndef __REGS_TIMER_H__
#define __REGS_TIMER_H__

#define CXD90014_TIMERCTL  0x00
# define TMRST		0x80000000
# define TMST		0x1000
# define TMINT		0x0100
# define TMCS_PERIODIC	0x0000
# define TMCS_ONESHOT	0x0010
# define TMCS_FREERUN	0x0030
# define TMCK_DIV1	0x0000
# define TMCK_DIV2	0x0001
# define TMCK_DIV4	0x0002
# define TMCK_DIV8	0x0003
# define TMCK_DIV16	0x0004
# define TMCK_DIV32	0x0005
# define TMCK_DIV64	0x0006
# define TMCK_DIV128	0x0007
#define CXD90014_TIMERCLR  0x04
# define TMCLR		0x0010
# define TMINTCLR	0x0001
#define CXD90014_TIMERCMP  0x08
#define CXD90014_TIMERREAD 0x0c
#define CXD90014_TIMERLOAD 0x10
#define CXD90014_TIMERCAP  0x14

#endif /* __REGS_TIMER_H__ */
