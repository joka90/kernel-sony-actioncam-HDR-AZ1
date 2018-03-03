/*
 * arch/arm/mach-cxd4132/include/mach/bootram.h
 *
 * Bootram map
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
#ifndef __ARCH_BOOTRAM_H__
#define __ARCH_BOOTRAM_H__

#define CXD4132_BOOTRAM_BANK_SHIFT	21
#define CXD4132_BOOTRAM_BANKSIZE	(1 << CXD4132_BOOTRAM_BANK_SHIFT)
#define CXD4132_BOOTRAM_BANKMASK	(CXD4132_BOOTRAM_BANKSIZE - 1)
#define CXD4132_BOOTRAM_N_BANK		2
#define CXD4132_BOOTRAM_SIZE		(CXD4132_BOOTRAM_BANKSIZE * CXD4132_BOOTRAM_N_BANK)
#define CXD4132_BOOTRAM_MASK		(CXD4132_BOOTRAM_SIZE - 1)

#define CXD4132_BOOTRAM_WORKAREA 0x00010000
#define  CXD4132_BOOTRAM_VECTOR   0x000c	/* CPU#0,#1,#2,#3 */
#define  CXD4132_BOOTRAM_FLAG     0x0020	/* CPU#0,#1,#2,#3 */
#define CXD4132_SUSPEND_WORKAREA 0x00020000

#endif /* __ARCH_BOOTRAM_H__ */
