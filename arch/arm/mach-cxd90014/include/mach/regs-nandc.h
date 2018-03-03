/*
 * arch/arm/mach-cxd90014/include/mach/regs-nandc.h
 *
 * Copyright 2012,2013,2014 Sony Corporation.
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

#ifndef __REGS_NANDC_H
#define __REGS_NANDC_H

#include <mach/platform.h>

/*---------------------------------------------------------------------------*/
/* NAND Controler init parameter					     */
/*---------------------------------------------------------------------------*/
/*-----------------------------------*/
/* NAND Controler reg addr           */
/*-----------------------------------*/

#define NC_BASE_ADDR    (0x00020000)
#define NC_AREA_SIZE    (0x00010000)

/* Virtual Address */
#define VA_NC_POR_RESET_COUNT(base)        ((volatile unsigned long*)((unsigned long)((base)+0x2a0)))

#define OFFSET_NC_POR_RESET_COUNT            (unsigned long)(0x2a0)
# define SHIFT_NC_EBI_EN                     (20)
# define SHIFT_NC_EBI_BURST_GRANULARITY      (16)
# define MASK_NC_EBI_EN                      (0x00100000)
# define MASK_NC_EBI_BURST_GRANULARITY       (0x000f0000)
# define NC_EBI_EN_ENABLE                    (1 << SHIFT_NC_EBI_EN)


/* ADDR_RAM_MISC_INFO */
#define ADDR_RAM_MISC_INFO                (CXD90014_ESRAM_BASE + 0x1200)
#define MISC_NAND_INIT_PARAM_OFFSET       (0x300)
#define MISC_NAND_INIT_REGPARAM_OFFSET    (MISC_NAND_INIT_PARAM_OFFSET + 0x14)
# define REGPARAM_POR_RESET_COUNT_OFFSET  (0xa0)

#endif
