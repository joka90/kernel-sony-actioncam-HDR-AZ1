/*
 * include/asm-arm/arch-cxd4132/regs-gpio.h
 *
 * GPIO registers
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
#ifndef __REGS_GPIO_H__
#define __REGS_GPIO_H__

#define GPIO_SET(x)		((x)+0x04)
#define GPIO_CLR(x)		((x)+0x08)

#define GPIO_DIR		0x00
#define GPIO_DIR_SET		GPIO_SET(GPIO_DIR)
#define GPIO_DIR_CLR		GPIO_CLR(GPIO_DIR)
#define GPIO_DATA_READ		0x10
#define GPIO_DATA_SET		GPIO_SET(GPIO_DATA_READ)
#define GPIO_DATA_CLEAR		GPIO_CLR(GPIO_DATA_READ)
#define GPIO_INEN		0x20
#define GPIO_INEN_SET		GPIO_SET(GPIO_INEN)
#define GPIO_INEN_CLR		GPIO_CLR(GPIO_INEN)
#define GPIO_PORTSEL		0x30
#define GPIO_PORTSEL_SET	GPIO_SET(GPIO_PORTSEL)
#define GPIO_PORTSEL_CLR	GPIO_CLR(GPIO_PORTSEL)

#ifndef __ASSEMBLY__
# include <asm/io.h>
static inline void gpio_set(int nr, int reg, int bitnr)
{
	writel(1 << bitnr, VA_GPIO(nr)+GPIO_SET(reg));
}

static inline void gpio_clr(int nr, int reg, int bitnr)
{
	writel(1 << bitnr, VA_GPIO(nr)+GPIO_CLR(reg));
}
#endif /* !__ASSEMBLY__ */

#endif /* __REGS_GPIO_H__ */
