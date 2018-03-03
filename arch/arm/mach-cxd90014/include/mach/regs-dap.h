/*
 * arch/arm/mach-cxd90014/includ/mach/regs-dap.h
 *
 * DAP register
 *
 * Copyright 2013 Sony Corporation
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

#ifndef __MACH_REGS_DAP_H__
#define __MACH_REGS_DAP_H__

#define N_DAP_CTI	4

/* offset of register */
#define DAP_CTI(x)	(0x8000 + 0x1000*(x))

#define VA_DAPCTI(x)	IO_ADDRESSP(CXD90014_DAP_BASE + DAP_CTI(x))

/* CTI */
#define N_DAP_CTI_TRIG	8

#define DAP_CTICONTROL	0x00
#define DAP_CTIINTACK	0x10
#define DAP_CTIINEN(x)	(0x20 + 4*(x))
#define DAP_CTIOUTEN(x)	(0xa0 + 4*(x))
#define DAP_CTIGATE	0x140
#define DAP_CTILOCK	0xfb0
#define   DAP_CTI_UNLOCK  0xc5acce55

/* CTI trigger input */
#define DAP_CTIIN_PMUIRQ	1
#define DAP_CTIIN_COMMTX	4
#define DAP_CTIIN_COMMRX	5

/* CTI trigger output */
#define DAP_CTIOUT_NCTIRQ	6

/* CTI channel assign */
#define DAP_CTICH_PMUIRQ	1

#endif /* __MACH_REGS_DAP_H__ */
