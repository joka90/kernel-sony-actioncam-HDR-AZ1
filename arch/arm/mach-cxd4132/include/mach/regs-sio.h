/*
 * include/asm-arm/arch-cxd4132/regs-sio.h
 *
 * SIO registers
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
#ifndef __REGS_SIO_H__
#define __REGS_SIO_H__

#define SIO_SHIFT 24
#define SIOCS	0x00
# define SIOCS_CSEL (1 << 24)
# define SIOCS_OE   (1 << 25)
# define SIOCS_LMS  (1 << 26)
# define SIOCS_SOEN  (1 << 27)
# define SIOCS_SCKOE (1 << 28)
# define SIOCS_SIOST (1 << 29)
# define SIOCS_CLOE  (1 << 30)
# define SIOCS_ICLSIO (1 << 31)
#define SIOM	0x04
# define SIOM_CSOMOD (1 << 24)
# define SIOM_RTMOD (1 << 25)
# define SIOM_ICK_16 (0 << 26)
# define SIOM_TRM_SENDREC (3 << 28)
# define SIOM_IMOD (1 << 30)
# define SIOM_CSEN (1 << 31)
#define SIOSA	0x08
#define SION	0x0c
#define SIOEN	0x10
# define SIOEN_EN (1 << 24)
#define SIOBGC0	0x14
#define SIOBGC1	0x18
#define SIOBUF  0x100
#endif /* __REGS_SIO_H__ */
