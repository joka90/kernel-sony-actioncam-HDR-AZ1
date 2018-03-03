/*
 * include/asm-arm/hardware/serial_amba2_ioctl.h
 *
 * UART ioctl definitions
 *
 * Copyright 2005,2006,2007 Sony Corporation
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
#ifndef __SERIAL_AMBA2_IOCTL_H__
#define __SERIAL_AMBA2_IOCTL_H__

#define UART_IOC_MAGIC   'u'

#define UART_IO(num)             _IO  (UART_IOC_MAGIC, num)
#define UART_IOR(num, dataitem)  _IOR (UART_IOC_MAGIC, num, dataitem)
#define UART_IOW(num, dataitem)  _IOW (UART_IOC_MAGIC, num, dataitem)
#define UART_IOWR(num, dataitem) _IOWR(UART_IOC_MAGIC, num, dataitem)

#define AMBAUART_IOC_GETMODE    UART_IOR(1, unsigned long)
#define AMBAUART_IOC_SETMODE    UART_IOW(2, unsigned long)
# define AMBAUART_TX_OFF 0
# define AMBAUART_TX_ON  1

#endif /* __SERIAL_AMBA2_IOCTL_H__ */
