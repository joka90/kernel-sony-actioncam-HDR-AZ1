/*
 * arch/arm/plat-cxd41xx/include/mach/gpio.h
 *
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
#ifndef __GPIO_EJ_H__
#define __GPIO_EJ_H__

// function
#define GPIOMODE_GPIO       (0x00000001) // General PORT
#define GPIOMODE_PERIPHERAL (0x00000002) // Peripheral
// Direction
#define GPIOMODE_INPUT      (0x00000004) // Input
#define GPIOMODE_OUTPUT_H   (0x00000008) // Output with High level
#define GPIOMODE_OUTPUT_L   (0x00000010) // Output with Low level

// ioctl command
enum {
	GPIO_CMD_READ = 0,
	GPIO_CMD_WRITE,
	GPIO_CMD_SET_MODE,
	GPIO_CMD_GET_MODE,
};

typedef enum gpio_portctrl{
	GPIO_CTRL_BIT = 0,
	GPIO_CTRL_PORT,
}GPIO_PORTCTRL;

typedef struct gpio_ioctl_port{
	GPIO_PORTCTRL portctl; 	// port contrl flag
	unsigned char portid;  		// Port number
	union{
		unsigned char	bitnum;	// bit number
		unsigned int	bitmask;// bitmask
	};
	unsigned int value;		// set/get value
}GPIO_PORT;

#ifdef __KERNEL__

int gpio_get_mode_bit(unsigned char port, unsigned int bit, unsigned int *mode);
int gpio_set_mode_port(unsigned char port,unsigned int bitmask, unsigned int mode);
int gpio_get_data_port(unsigned char port, unsigned int bitmask, unsigned int *data );
int gpio_set_data_port(unsigned char port, unsigned int bitmask, unsigned int data);

#define gpio_set_mode_bit(port, bit, mode) \
		gpio_set_mode_port(port, (1 << (bit)), mode)
#define gpio_get_data_bit(port, bit, val) 			\
({								\
	int ret = gpio_get_data_port(port, (1 << (bit)), val);	\
	*(val) = (*(val) != 0) ?  1 : 0;			\
	ret;							\
})

#define gpio_set_data_bit(port, bit, val) 			\
	gpio_set_data_port(port, (1 << (bit)), ((val) ? (1 << (bit)) : 0))
#endif

#endif /*__EJ_H__*/
