/*
 * include/asm-arm/arch-cxd4132/regs-misc.h
 *
 * MISCCTRL registers
 *
 * Copyright 2009,2010 Sony Corporation
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
#ifndef __REGS_MISC_H__
#define __REGS_MISC_H__

#define MISCCTRL_MODEREAD	0
# define DDRCKSEL_MASK		0x01000000
#  define DDRCKSEL_DDR1		0x01000000
#  define DDRCKSEL_DDR2		0x00000000
# define MODEREAD_BM1		0x00000002
# define MODEREAD_DBG0		0x00000100
#define MISCCTRL_READDONE	1
# define FREAD_DONE		0x1
#define MISCCTRL_TYPEID		2
# define TYPEID_IDMASK		0xff00
#  define TYPEID_CXD4132	0x0300
#  define TYPEID_CXD4133	0x0400

#endif /* __REGS_MISC_H__ */
