/*
 * arch/arm/plat-cxd41xx/gpio.c
 *
 * GPIO driver
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
#include <linux/udif/module.h>
#include <linux/udif/cdev.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/uaccess.h>
#include <linux/udif/proc.h>
#include <linux/udif/print.h>
#include <linux/udif/macros.h>
#include <mach/udif/devno.h>
#include <mach/gpio.h>

#define GPIO_VERSION	"1.0"
#define	DEV_NAME	"gpio"

/*--------------------------------------------------------------------------*/
// Global variable
/*--------------------------------------------------------------------------*/
typedef struct gpio_reg{
    UDIF_U32 dir;
    UDIF_U32 rdata;
    UDIF_U32 inen;
    UDIF_U32 portsel;
}GPIO_REG;

typedef struct gpio_info{
	UDIF_VA		base;
	GPIO_REG 	reg;
	UDIF_SPINLOCK 	gpio_lock;
}GPIO_INFO;
static GPIO_INFO ginfo[UDIF_NR_GPIO];

/*--------------------------------------------------------------------------*/
// Register assign
/*--------------------------------------------------------------------------*/
// GPIO REGISTER
#ifdef CONFIG_ARCH_CXD4132BASED
#define GPIO_DIR		0x00
#define GPIO_DIR_SET		0x04
#define GPIO_DIR_CLEAR		0x08
#define GPIO_DATA_READ		0x10
#define GPIO_DATA_SET		0x14
#define GPIO_DATA_CLEAR		0x18
#define GPIO_INEN		0x20
#define GPIO_INEN_SET		0x24
#define GPIO_INEN_CLEAR		0x28
#define GPIO_PORTSEL		0x30
#define GPIO_PORTSEL_SET	0x34
#define GPIO_PORTSEL_CLR	0x38
#else
#define GPIO_DIR		0x00
#define GPIO_DATA_READ		0x04
#define GPIO_DATA_SET		0x08
#define GPIO_DATA_CLEAR		0x0c
#define GPIO_INEN		0x10
#define GPIO_PORTSEL		0x14
#define GPIO_PORTSEL_SET	0x18
#define GPIO_PORTSEL_CLR	0x1c
#endif
#define GPIO_DIRECTION(x)	(ginfo[x].base + GPIO_DIR)
#define GPIO_RDATA(x)		(ginfo[x].base + GPIO_DATA_READ)
#define GPIO_DATASET(x)		(ginfo[x].base + GPIO_DATA_SET)
#define GPIO_DATACLR(x)		(ginfo[x].base + GPIO_DATA_CLEAR)
#define GPIO_INPUTEN(x)		(ginfo[x].base + GPIO_INEN)
#define GPIO_PORTSELDAT(x)	(ginfo[x].base + GPIO_PORTSEL)
#define GPIO_PORTSELSET(x)	(ginfo[x].base + GPIO_PORTSEL_SET)
#define GPIO_PORTSELCLR(x)	(ginfo[x].base + GPIO_PORTSEL_CLR)

/*--------------------------------------------------------------------------*/
// MACRO / inline
/*--------------------------------------------------------------------------*/
//#define DBG_ERR(arg...)	do{ udif_printk(arg); } while (0)
#define DBG_ERR(arg...)		do{  }while(0)
//#define DBG_MSG(arg...)	do{ udif_printk(arg); } while (0)
#define DBG_MSG(arg...)		do{  }while(0)

#define GPIOMODE_FUNC		(GPIOMODE_GPIO | GPIOMODE_PERIPHERAL)
#define GPIOMODE_DIR		(GPIOMODE_INPUT |\
				 GPIOMODE_OUTPUT_H | GPIOMODE_OUTPUT_L)

static inline UDIF_U32 mask_val(UDIF_U8 bit)
{
	return (1 << bit);
}

/*--------------------------------------------------------------------------*/
// IO access inline functions
/*--------------------------------------------------------------------------*/
#define gpio_read(addr)		udif_ioread32(addr)
#define gpio_write(val, addr)	udif_iowrite32(val, addr)

/*--------------------------------------------------------------------------*/
// get/set/clr register inline functions
/*--------------------------------------------------------------------------*/
static inline UDIF_U32 gpio_get_bits(UDIF_U32 bitmask,
					const UDIF_VA addr){

	return (gpio_read(addr) & bitmask);
}

static inline void gpio_set_bits(UDIF_U8 port, UDIF_U32 bitmask,
					const UDIF_VA addr){
	unsigned long	flags = 0;
	//lock
	udif_spin_lock_irqsave(&ginfo[port].gpio_lock, flags);
	// write register
	gpio_write((gpio_read(addr) | bitmask), addr);
	//unlock
	udif_spin_unlock_irqrestore(&ginfo[port].gpio_lock, flags);
}

static void gpio_clr_bits(UDIF_U8 port, UDIF_U32 bitmask,
					const UDIF_VA addr){
	unsigned long	flags = 0;
	//lock
	udif_spin_lock_irqsave(&ginfo[port].gpio_lock, flags);
	// write register
	gpio_write((gpio_read(addr) & ~bitmask), addr);
	//unlock
	udif_spin_unlock_irqrestore(&ginfo[port].gpio_lock, flags);
}
/*--------------------------------------------------------------------------*/
// for setting mode
/*--------------------------------------------------------------------------*/
static int set_mode_bits(unsigned char port,unsigned int bitmask, unsigned int mode)
{
	// function
	switch(mode & GPIOMODE_FUNC){
	case GPIOMODE_GPIO:
		gpio_write(bitmask, GPIO_PORTSELSET(port));
		break;
	case GPIOMODE_PERIPHERAL:
		gpio_write(bitmask, GPIO_PORTSELCLR(port));
		break;
	default:
		break;
	}
	// Direction
	switch(mode & GPIOMODE_DIR){
	case GPIOMODE_INPUT:
		gpio_set_bits(port, bitmask, GPIO_INPUTEN(port));
		gpio_clr_bits(port, bitmask, GPIO_DIRECTION(port));
		break;
	case GPIOMODE_OUTPUT_H:
		gpio_clr_bits(port, bitmask, GPIO_INPUTEN(port));
		gpio_write(bitmask, GPIO_DATASET(port));
		gpio_set_bits(port, bitmask, GPIO_DIRECTION(port));
		break;
	case GPIOMODE_OUTPUT_L:
		gpio_clr_bits(port, bitmask, GPIO_INPUTEN(port));
		gpio_write(bitmask, GPIO_DATACLR(port));
		gpio_set_bits(port, bitmask, GPIO_DIRECTION(port));
		break;
	default:
		break;
	}

	return 0;
}
/*--------------------------------------------------------------------------*/
//
//	Kernel API
//
/*--------------------------------------------------------------------------*/
int gpio_set_mode_port(unsigned char port,unsigned int bitmask, unsigned int mode)
{
	if (port >= UDIF_NR_GPIO) {
		udif_printk("gpio_set_mode_port: illegal port:%u\n", port);
		return 0;
	}

	return set_mode_bits(port, bitmask, mode);
}

// get mode
int gpio_get_mode_bit(unsigned char port, unsigned int bit, unsigned int *mode)
{
	UDIF_U32 mask = mask_val(bit);

	if (port >= UDIF_NR_GPIO) {
		udif_printk("gpio_get_mode_bit: illegal port:%u\n", port);
		return 0;
	}

	*mode = 0;
	// GPIO mode if PORTSEL register is set.
	if(gpio_get_bits(mask, GPIO_PORTSELDAT(port)) != 0){
		*mode |= GPIOMODE_GPIO;
	}else{
		*mode |= GPIOMODE_PERIPHERAL;
	}
	// Direction
	if(gpio_get_bits(mask, GPIO_DIRECTION(port)) != 0){
		if(gpio_get_bits(mask, GPIO_RDATA(port)) != 0){
			// output(high)
			*mode |= GPIOMODE_OUTPUT_H;
		}else{
			// output(low)
			*mode |= GPIOMODE_OUTPUT_L;
		}
	}else{
		// input
		*mode |= GPIOMODE_INPUT;
	}
	return 0;
}

int gpio_get_data_port(unsigned char port, unsigned int bitmask, unsigned int *data )
{
	if (port >= UDIF_NR_GPIO) {
		udif_printk("gpio_get_data_port: illegal port:%u\n", port);
		return 0;
	}

	*data = gpio_get_bits(bitmask, GPIO_RDATA(port));
	return 0;
}

int gpio_set_data_port(unsigned char port, unsigned int bitmask, unsigned int data)
{
	if (port >= UDIF_NR_GPIO) {
		udif_printk("gpio_set_data_port: illegal port:%u\n", port);
		return 0;
	}

	gpio_write(bitmask & data, GPIO_DATASET(port));
	gpio_write(bitmask & ~data, GPIO_DATACLR(port));
	return 0;
}

/*--------------------------------------------------------------------------*/
//
// for system call
//
/*--------------------------------------------------------------------------*/
// open()
static UDIF_ERR gpio_open(UDIF_FILE *file)
{
	return UDIF_ERR_OK;
}

// close()
static UDIF_ERR gpio_release(UDIF_FILE *file)
{
	return UDIF_ERR_OK;
}
// ioctl()
static UDIF_ERR gpio_ioctl(UDIF_FILE *file, UDIF_IOCTL *ictl)
{
	UDIF_UINT cmd = ictl->cmd;
	UDIF_ULONG arg = ictl->arg;
	UDIF_ERR status = UDIF_ERR_OK;
	GPIO_PORT param;

	if(udif_copy_from_user((void*)&param, (void __user *)arg, sizeof(param)) != 0){
		status = UDIF_ERR_FAULT;
		goto error;
	}
	if (param.portid >= UDIF_NR_GPIO) {
		status = UDIF_ERR_PAR;
		goto error;
	}
	switch(cmd){
	case GPIO_CMD_READ:
		if(param.portctl == GPIO_CTRL_BIT){
			gpio_get_data_bit(param.portid, param.bitnum, &param.value);
		}else{
			gpio_get_data_port(param.portid, param.bitmask, &param.value);
		}
		break;
	case GPIO_CMD_WRITE:
		if(param.portctl == GPIO_CTRL_BIT){
			gpio_set_data_bit(param.portid, param.bitnum, param.value);
		}else{
			gpio_set_data_port(param.portid, param.bitmask, param.value);
		}
		break;
	case GPIO_CMD_SET_MODE:
		if(param.portctl == GPIO_CTRL_BIT){
			gpio_set_mode_bit(param.portid, param.bitnum, param.value);
		}else{
			gpio_set_mode_port(param.portid, param.bitmask, param.value);
		}
		break;
	case GPIO_CMD_GET_MODE:
		gpio_get_mode_bit(param.portid, param.bitnum, &param.value);
		break;
	default:
		status = UDIF_ERR_PAR;
		break;
	}
	if(udif_copy_to_user((void __user *)arg, (void*)&param, sizeof(param)) != 0){
		status = UDIF_ERR_FAULT;
	}
error:
	return status;
}

static UDIF_CDEV_OPS gpio_fops = {
	.open = gpio_open,
	.close = gpio_release,
	.ioctl = gpio_ioctl,
};

static UDIF_DECLARE_CDEV(
	cdev_gpio,
	udif_device_node(UDIF_NODE_GPIO),
	&gpio_fops,
	NULL
);

#define GPIO_PROC_NAME		"gpio"
static int proc_port = 0;
static int gpio_read_proc(UDIF_PROC_READ *proc)
{
	unsigned int len = 0;
	len += udif_proc_setbuf(proc, len, "GPIO %d\n", proc_port);
	len += udif_proc_setbuf(proc, len,
		"DIR    : 0x%08X\n", gpio_read(GPIO_DIRECTION(proc_port)));
	len += udif_proc_setbuf(proc, len,
		"RDATA  : 0x%08X\n", gpio_read(GPIO_RDATA(proc_port)));
	len += udif_proc_setbuf(proc, len,
		"INEN   : 0x%08X\n", gpio_read(GPIO_INPUTEN(proc_port)));
	len += udif_proc_setbuf(proc, len,
		"PORTSEL: 0x%08X\n", gpio_read(GPIO_PORTSELDAT(proc_port)));
	return len;
}
#define UDIF_GPIO_PROC_BUFSIZE	128
static int gpio_write_proc(UDIF_PROC_WRITE *proc)
{
	UDIF_U8 buf[UDIF_GPIO_PROC_BUFSIZE];
	UDIF_U8 rw, func;
	UDIF_U32 port, bitmask, value;
	//[r/w] [port] [d/m] [bitmask] [value]
	//(read :r 1)
	//(write:w 0 d ff10 5a5a)
	if(proc->count > UDIF_GPIO_PROC_BUFSIZE){
		return 0;
	}
	if(udif_proc_getbuf(proc, buf, UDIF_GPIO_PROC_BUFSIZE) == UDIF_GPIO_PROC_BUFSIZE){
		DBG_ERR("Fail to udif_copy_from_user()\n");
		return UDIF_ERR_FAULT;
	}
	sscanf(buf, "%c %d %c %x %x", &rw, &port, &func, &bitmask, &value);
	udif_printk("%c %d %c %x %x\n", rw, port, func, bitmask, value);

	if (port >= UDIF_NR_GPIO) {
		udif_printk("gpio_write_proc: illegal port:%u\n", port);
		return 0;
	}
	if(rw == 'r'){
		if(port < UDIF_NR_GPIO){
			proc_port = port;
		}
	}else if(rw == 'w'){
		udif_printk("write gpio\n");
		switch(func){
		case 'm':
			udif_printk("mode %d %X %X\n", port, bitmask, value);
			gpio_set_mode_port(port, bitmask, value);
			break;
		case 'd':
			udif_printk("data %d %X %X\n", port, bitmask, value);
			gpio_set_data_port(port, bitmask, value);
			break;
		default:
			break;
		}
	}else{
		//error
	}
	return 0;
}
static UDIF_PROC gpio_proc = {
	.name = GPIO_PROC_NAME,
	.read = gpio_read_proc,
	.write = gpio_write_proc,
};

// Probe
static UDIF_ERR gpio_probe(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
	ginfo[ch].base = udif_devio_virt(dev, ch);
	udif_spin_lock_init(&ginfo[ch].gpio_lock);
	return UDIF_ERR_OK;
}
// Remove
static UDIF_ERR gpio_remove(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
	udif_remove_proc(&gpio_proc);
	udif_cdev_unregister(&cdev_gpio);
	return UDIF_ERR_OK;
}
// Suspend
static UDIF_ERR gpio_suspend(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
	ginfo[ch].reg.dir 	= gpio_read(GPIO_DIRECTION(ch));
	ginfo[ch].reg.rdata 	= gpio_read(GPIO_RDATA(ch));
	ginfo[ch].reg.inen 	= gpio_read(GPIO_INPUTEN(ch));
	ginfo[ch].reg.portsel 	= gpio_read(GPIO_PORTSELDAT(ch));
	return UDIF_ERR_OK;
}

// Resume
static unsigned int ignore_port = -1U;
static unsigned int ignore_bitmask = 0;
module_param_named(port, ignore_port, uint, S_IRUSR);
module_param_named(bitmask, ignore_bitmask, uint, S_IRUSR);

static UDIF_ERR gpio_resume(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
	UDIF_U32 setdata, clrdata;

	setdata = ginfo[ch].reg.rdata;
	clrdata = ~ginfo[ch].reg.rdata;

	if (ignore_port == ch) {
		setdata &= ~ignore_bitmask;
		clrdata &= ~ignore_bitmask;
	}

	gpio_write(ginfo[ch].reg.portsel, GPIO_PORTSELSET(ch));
	gpio_write(~ginfo[ch].reg.portsel, GPIO_PORTSELCLR(ch));
	gpio_write(ginfo[ch].reg.inen, GPIO_INPUTEN(ch));
	gpio_write(setdata, GPIO_DATASET(ch));
	gpio_write(clrdata, GPIO_DATACLR(ch));
	gpio_write(ginfo[ch].reg.dir, GPIO_DIRECTION(ch));
	return UDIF_ERR_OK;
}
// init
static UDIF_ERR gpio_init(UDIF_VP data)
{
	UDIF_ERR ret;

	if((ret = udif_cdev_register(&cdev_gpio)) != UDIF_ERR_OK){
		return ret;
	}
	if((ret = udif_create_proc(&gpio_proc)) != UDIF_ERR_OK){
		udif_cdev_unregister(&cdev_gpio);
		return ret;
	}
	return ret;
}
// exit
static UDIF_ERR gpio_exit(UDIF_VP data)
{
	udif_remove_proc(&gpio_proc);
	udif_cdev_unregister(&cdev_gpio);
	return UDIF_ERR_OK;
}

UDIF_IDS(gpio_ids) = {
	UDIF_ID(UDIF_ID_GPIO, UDIF_VALID_MASK_GPIO),
};

static UDIF_DRIVER_OPS gpio_ops = {
	.init		= gpio_init,
	.exit		= gpio_exit,
	.probe 		= gpio_probe,
	.remove 	= gpio_remove,
	.suspend 	= gpio_suspend,
	.resume 	= gpio_resume,
};
UDIF_DEPS(gpio_deps) = {};

UDIF_DECLARE_DRIVER(
	udif_gpio,
	DEV_NAME,
	GPIO_VERSION,
	&gpio_ops,
	gpio_ids,
	gpio_deps,
	NULL
);

static int __init __udif_init_gpio(void)
{
	UDIF_DRIVER *drv = &udif_gpio;
	return udif_driver_register(&drv, 1);
}
arch_initcall(__udif_init_gpio);

EXPORT_SYMBOL(gpio_set_mode_port);
EXPORT_SYMBOL(gpio_get_mode_bit);
EXPORT_SYMBOL(gpio_get_data_port);
EXPORT_SYMBOL(gpio_set_data_port);
