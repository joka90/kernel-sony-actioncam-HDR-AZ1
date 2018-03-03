/*
 * drivers/gpio/gpio.c
 *
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/udif/module.h>
#include <linux/udif/cdev.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/uaccess.h>
#include <linux/udif/print.h>
#include <linux/udif/macros.h>
#include <linux/udif/proc.h>
#include <mach/udif/devno.h>
#include <mach/udif/platform.h>
#include <linux/gpio/gpio.h>
#include "gpio_inc.h"

static UDIF_CDEV_OPS gpio_fops = {
    .open  = gpio_open,
    .close = gpio_close,
    .ioctl = gpio_ioctl,
};

static UDIF_DRIVER_OPS gpio_ops = {
    .init       = gpio_init,
    .exit       = gpio_exit,
    .probe      = gpio_probe,
    .remove     = gpio_remove,
    .suspend    = gpio_suspend,
    .resume     = gpio_resume,
};

static UDIF_DECLARE_CDEV(
    cdev_gpio,
    udif_device_node(UDIF_NODE_GPIO),
    &gpio_fops,
    NULL
);

UDIF_IDS(gpio_ids) = {
    UDIF_ID(UDIF_ID_GPIO, UDIF_VALID_MASK_GPIO),
};

UDIF_DEPS(gpio_deps) = {};

UDIF_DECLARE_DRIVER(udif_gpio, DEV_NAME, GPIO_VERSION, &gpio_ops, gpio_ids, gpio_deps, NULL);

static int __init __udif_init_gpio(void)
{
	UDIF_DRIVER *drv = &udif_gpio;
	return udif_driver_register(&drv, 1);
}
arch_initcall(__udif_init_gpio);

static UDIF_PROC gpio_proc = {
    .name  = GPIO_PROC_NAME,
    .read  = gpio_read_proc,
    .write  = gpio_write_proc
};

static GPIO_INFO ginfo[UDIF_NR_GPIO] =
{
    [0 ... (UDIF_NR_GPIO -1)] = { .base = (UDIF_VA)GPIO_REG_ADDR_INIT, .reg = { 0, 0, 0, 0 }, },
};

#ifdef TEST_GPIO_DRV
static volatile int gpio_lock_cnt[UDIF_NR_GPIO] =
{
    [0 ... (UDIF_NR_GPIO -1)] = 0,
};
static int gpio_drv_open_cnt = 0;
static int gpio_lock_log = 0;
#endif /* #ifdef TEST_GPIO_DRV */

static inline UDIF_U32 mask_val(UDIF_U8 bit)
{
    return ( 0x01UL << bit );
}

static inline UDIF_U32 gpio_data_lsr(UDIF_U8 bit, UDIF_U32 data)
{
    return ( (data & mask_val(bit)) >> bit );
}

static inline UDIF_U32 gpio_get_port( UDIF_U32 bitmask, const UDIF_VA addr)
{
    return ( gpio_read(addr) & bitmask );
}

static UDIF_INT hw_gpio_get_data_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32* data)
{
    gpio_assert_NULL(data);

    *data = gpio_data_lsr( bit, gpio_read( GPIO_REG_RDATA(port) ) );

    return UDIF_ERR_OK;
}

static UDIF_INT hw_gpio_get_data_port( UDIF_U8 port, UDIF_U32 bitmask, UDIF_U32* data )
{
    gpio_assert_NULL(data);

    *data = gpio_get_port( bitmask, GPIO_REG_RDATA(port) );

    return UDIF_ERR_OK;
}

static UDIF_INT hw_gpio_set_data_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32 data)
{
    UDIF_U32 write_num;

    write_num = mask_val(bit);

    switch( data )
    {
        case GPIO_PORT_L:                               /* LOW : bit Clear */
            gpio_write( write_num, GPIO_REG_WDATA_CLR(port) );
            break;
        case GPIO_PORT_H:                               /* HIGH: bit Set */
            gpio_write( write_num, GPIO_REG_WDATA_SET(port) );
            break;
        default:
            return UDIF_ERR_PAR;
    }

    return UDIF_ERR_OK;
}
static UDIF_INT hw_gpio_set_data_port(UDIF_U8 port, UDIF_U32 bitmask, UDIF_U32 data)
{
    UDIF_U32    write_data;

    write_data  = ( gpio_read( GPIO_REG_WDATA(port) ) & ~bitmask );
    write_data |= ( data & bitmask );
    gpio_write( write_data , GPIO_REG_WDATA(port) );

    return UDIF_ERR_OK;
}

static UDIF_INT hw_gpio_set_mode_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32 mode)
{
    UDIF_U32    write_num;
    UDIF_INT    ret;

    write_num = mask_val(bit);

    ret = hw_gpio_set_mode_port( port, write_num, mode );

    return ret;
}

static UDIF_INT hw_gpio_set_mode_port(UDIF_U8 port, UDIF_U32 bitmask, UDIF_U32 mode)
{
    // function
    switch( mode & GPIOMODE_FUNC )
    {
        case GPIOMODE_GPIO:
            gpio_write( bitmask, GPIO_REG_PORTSEL_SET(port) );
            break;
        case GPIOMODE_PERIPHERAL:
            gpio_write( bitmask, GPIO_REG_PORTSEL_CLR(port) );
            break;
        default:
            break;
    }

    // Direction
    switch( mode & GPIOMODE_DIR )
    {
        case GPIOMODE_INPUT:
            gpio_write( bitmask, GPIO_REG_INPUTEN_SET(port) );
            gpio_write( bitmask, GPIO_REG_DIRECTION_CLR(port) );
            break;
        case GPIOMODE_OUTPUT_H:
            gpio_write( bitmask, GPIO_REG_INPUTEN_CLR(port) );
            gpio_write( bitmask, GPIO_REG_WDATA_SET(port) );
            gpio_write( bitmask, GPIO_REG_DIRECTION_SET(port) );
            break;
        case GPIOMODE_OUTPUT_L:
            gpio_write( bitmask, GPIO_REG_INPUTEN_CLR(port) );
            gpio_write( bitmask, GPIO_REG_WDATA_CLR(port) );
            gpio_write( bitmask, GPIO_REG_DIRECTION_SET(port) );
            break;
        default:
            break;
    }

    return UDIF_ERR_OK;
}

static UDIF_INT hw_gpio_get_mode_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32* mode)
{
    UDIF_U32 read_num = 0;

    gpio_assert_NULL(mode);

    // function
    switch( gpio_data_lsr( bit, gpio_read( GPIO_REG_PORTSEL(port) ) ) )
    {
        case GPIO_PORT_L:
            read_num |= GPIOMODE_PERIPHERAL;
            break;
        case GPIO_PORT_H:
            read_num |= GPIOMODE_GPIO;
            break;
        default:
            break;
    }

    // Direction
    switch( gpio_data_lsr( bit, gpio_read( GPIO_REG_INPUTEN(port) ) ) )
    {
        case GPIO_PORT_L:
            if( gpio_data_lsr( bit, gpio_read( GPIO_REG_WDATA(port) ) ) == 0 )
            {
                read_num |= GPIOMODE_OUTPUT_L;
            }
            else
            {
                read_num |= GPIOMODE_OUTPUT_H;
            }
            break;
        case GPIO_PORT_H:
            read_num |= GPIOMODE_INPUT;
            break;
        default:
            break;
    }

    *mode = read_num;

    return UDIF_ERR_OK;
}

int gpio_get_data_bit(unsigned char port, unsigned int bit, unsigned int* data)
{
    unsigned long   flags = 0;
    int             ret;

    if( data == NULL )
    {
        return UDIF_ERR_PAR;
    }

    if( bit > 31U )
    {
        return UDIF_ERR_PAR;
    }
    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_get_data_bit( (UDIF_U8)port, (UDIF_U32)bit, (UDIF_U32*)data );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}

int gpio_get_data_port(unsigned char port, unsigned int bitmask, unsigned int* data )
{
    unsigned long   flags = 0;
    int             ret;

    if( data == NULL )
    {
        return UDIF_ERR_PAR;
    }
    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_get_data_port( (UDIF_U8)port, (UDIF_U32)bitmask, (UDIF_U32*)data );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}

int gpio_set_data_bit(unsigned char port, unsigned int bit, unsigned int data)
{
    unsigned long   flags = 0;
    int             ret;

    if( bit > 31U )
    {
        return UDIF_ERR_PAR;
    }
    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_set_data_bit( (UDIF_U8)port, (UDIF_U32)bit, (UDIF_U32)data );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}
int gpio_set_data_port(unsigned char port, unsigned int bitmask, unsigned int data)
{
    unsigned long   flags = 0;
    int             ret;

    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_set_data_port( (UDIF_U8)port, (UDIF_U32)bitmask, (UDIF_U32)data );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}

int gpio_set_mode_bit(unsigned char port, unsigned int bit, unsigned int mode)
{
    unsigned long   flags = 0;
    int             ret;

    if( bit > 31U )
    {
        return UDIF_ERR_PAR;
    }
    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_set_mode_bit( (UDIF_U8)port, (UDIF_U32)bit, (UDIF_U32)mode );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}

int gpio_set_mode_port(unsigned char port, unsigned int bitmask, unsigned int mode)
{
    unsigned long   flags = 0;
    int             ret;

    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_set_mode_port( (UDIF_U8)port, (UDIF_U32)bitmask, (UDIF_U32)mode );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}

int gpio_get_mode_bit(unsigned char port, unsigned int bit, unsigned int* mode)
{
    unsigned long   flags = 0;
    int             ret;

    if( mode == NULL )
    {
        return UDIF_ERR_PAR;
    }
    if( bit > 31U )
    {
        return UDIF_ERR_PAR;
    }
    if( port >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( port, flags );

    ret = (int)hw_gpio_get_mode_bit( (UDIF_U8)port, (UDIF_U32)bit, (UDIF_U32*)mode );

    //unlock
    gpio_unlock( port, flags );

    return ret;
}

static UDIF_ERR gpio_open(UDIF_FILE *file)
{
#ifdef TEST_GPIO_DRV
    gpio_drv_open_cnt++;
#endif /* #ifdef TEST_GPIO_DRV */
    return UDIF_ERR_OK;
}

static UDIF_ERR gpio_close(UDIF_FILE *file)
{
#ifdef TEST_GPIO_DRV
    gpio_drv_open_cnt--;
#endif /* #ifdef TEST_GPIO_DRV */
    return UDIF_ERR_OK;
}

static UDIF_ERR gpio_ioctl(UDIF_FILE *file, UDIF_IOCTL *ictl)
{
    UDIF_UINT       cmd;
    UDIF_ULONG      arg;
    UDIF_ERR        status  = UDIF_ERR_OK;
    GPIO_PORT       param;
    unsigned long   flags   = 0;

    if( ( file == NULL ) || ( ictl == NULL ) )
    {
        return UDIF_ERR_PAR;
    }

    cmd = ictl->cmd;
    arg = ictl->arg;

    if( (void __user *)ictl->arg == NULL )
    {
        return UDIF_ERR_PAR;
    }

    if( udif_copy_from_user( (void*)&param, (void __user *)arg, sizeof(param)) != 0 )
    {
        return UDIF_ERR_FAULT;
    }

    if( param.portid >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    //lock
    gpio_lock( param.portid, flags );

    switch( cmd )
    {
        case GPIO_CMD_READ:
            if(param.portctl == GPIO_CTRL_BIT)
            {
                hw_gpio_get_data_bit( (UDIF_U8)param.portid, (UDIF_U32)param.bitnum, (UDIF_U32*)&param.value );
            }
            else
            {
                hw_gpio_get_data_port( (UDIF_U8)param.portid, (UDIF_U32)param.bitmask, (UDIF_U32*)&param.value );
            }

            if( udif_copy_to_user((void __user *)arg, (void*)&param, sizeof(param)) != 0 )
            {
                status = UDIF_ERR_FAULT;
                goto _ERR;
            }
            break;

        case GPIO_CMD_WRITE:
            if(param.portctl == GPIO_CTRL_BIT)
            {
                hw_gpio_set_data_bit( (UDIF_U8)param.portid, (UDIF_U32)param.bitnum, (UDIF_U32)param.value );
            }
            else
            {
                hw_gpio_set_data_port( (UDIF_U8)param.portid, (UDIF_U32)param.bitmask, (UDIF_U32)param.value );
            }

            break;

        case GPIO_CMD_SET_MODE:
            if(param.portctl == GPIO_CTRL_BIT)
            {
                hw_gpio_set_mode_bit( (UDIF_U8)param.portid, (UDIF_U32)param.bitnum, (UDIF_U32)param.value );
            }
            else
            {
                hw_gpio_set_mode_port( (UDIF_U8)param.portid, (UDIF_U32)param.bitmask, (UDIF_U32)param.value );
            }

            break;

        case GPIO_CMD_GET_MODE:
            if( param.portctl == GPIO_CTRL_PORT )
            {
                status = UDIF_ERR_PAR;
                goto _ERR;
            }

            hw_gpio_get_mode_bit( (UDIF_U8)param.portid, (UDIF_U32)param.bitnum, (UDIF_U32*)&param.value );

            if( udif_copy_to_user((void __user *)arg, (void*)&param, sizeof(param)) != 0 )
            {
                status = UDIF_ERR_FAULT;
                goto _ERR;
            }
            break;

        default:
            status = UDIF_ERR_PAR;
            goto _ERR;
    }

_ERR:
    //unlock
    gpio_unlock( param.portid, flags );

    return status;
}

static UDIF_ERR gpio_probe(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
    if( dev == NULL )
    {
        return UDIF_ERR_PAR;
    }
    if( ch >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    ginfo[ch].base = udif_devio_virt(dev, ch);

    udif_spin_lock_init(&ginfo[ch].gpio_lock);

#ifdef TEST_GPIO_DRV
    /* init */
    gpio_lock_cnt[ch] = 0;
#endif /* #ifdef TEST_GPIO_DRV */

    return UDIF_ERR_OK;
}

static UDIF_ERR gpio_remove(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
    return UDIF_ERR_OK;
}

static UDIF_ERR gpio_suspend(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
    unsigned long   flags   = 0;

    if( ch >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    gpio_lock( ch, flags );

    ginfo[ch].reg.dir       = gpio_read( GPIO_REG_DIRECTION(ch) );
    ginfo[ch].reg.inen      = gpio_read( GPIO_REG_INPUTEN(ch) );
    ginfo[ch].reg.portsel   = gpio_read( GPIO_REG_PORTSEL(ch) );
    ginfo[ch].reg.wdata     = gpio_read( GPIO_REG_WDATA(ch) );

    gpio_unlock( ch, flags );

    return UDIF_ERR_OK;
}

static unsigned int ignore_port = -1U;
static unsigned int ignore_bitmask = 0;
module_param_named(port, ignore_port, uint, S_IRUSR);
module_param_named(bitmask, ignore_bitmask, uint, S_IRUSR);

static UDIF_ERR gpio_resume(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data)
{
    UDIF_U32 setdata, clrdata;

    if( ch >= UDIF_NR_GPIO )
    {
        return UDIF_ERR_PAR;
    }

    setdata = ginfo[ch].reg.wdata;
    clrdata = ~ginfo[ch].reg.wdata;

    if (ignore_port == ch) {
        setdata &= ~ignore_bitmask;
        clrdata &= ~ignore_bitmask;
    }

    gpio_write( ginfo[ch].reg.portsel,  GPIO_REG_PORTSEL(ch) );
    gpio_write( setdata, GPIO_REG_WDATA_SET(ch) );
    gpio_write( clrdata, GPIO_REG_WDATA_CLR(ch) );
    gpio_write( ginfo[ch].reg.dir,      GPIO_REG_DIRECTION(ch) );
    gpio_write( ginfo[ch].reg.inen,     GPIO_REG_INPUTEN(ch) );

    return UDIF_ERR_OK;
}

static UDIF_ERR gpio_init(UDIF_VP data)
{
    UDIF_ERR ret;

    udif_cdev_init(&cdev_gpio, udif_device_node(UDIF_NODE_GPIO), &gpio_fops, NULL);

    if((ret = udif_cdev_register(&cdev_gpio)) != UDIF_ERR_OK)
    {
        printk("gpio driver: cdev register err. \n");
        return ret;
    }

    if((ret = udif_create_proc(&gpio_proc)) != UDIF_ERR_OK){
        printk("gpio driver: proc create err. \n");
        return ret;
    }

    return UDIF_ERR_OK;
}

static UDIF_ERR gpio_exit(UDIF_VP data)
{
    udif_cdev_unregister(&cdev_gpio);
    udif_remove_proc(&gpio_proc);

    return UDIF_ERR_OK;
}

static int gpio_read_proc(UDIF_PROC_READ *proc)
{
    uint32_t len            = 0;
    uint32_t i;

    if( proc == NULL )
    {
        return UDIF_ERR_PAR;
    }

    for( i=0; i < UDIF_NR_GPIO; i++ )
    {
        len += udif_proc_setbuf(proc, len,
            "GPIO%dDIR   : 0x%08x \n", i, gpio_read( GPIO_REG_DIRECTION(i)) );
        len += udif_proc_setbuf(proc, len,
            "GPIO%dRDATA : 0x%08x \n", i, gpio_read( GPIO_REG_RDATA(i))     );
        len += udif_proc_setbuf(proc, len,
            "GPIO%dIEN   : 0x%08x \n", i, gpio_read( GPIO_REG_INPUTEN(i))   );
        len += udif_proc_setbuf(proc, len,
            "GPIO%dPORT  : 0x%08x \n", i, gpio_read( GPIO_REG_PORTSEL(i))   );
        len += udif_proc_setbuf(proc, len,
            "GPIO%dWDATA : 0x%08x \n", i, gpio_read( GPIO_REG_WDATA(i))     );
    }
    udif_proc_setend( proc );

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
		return UDIF_ERR_PAR;
	}
	if(udif_proc_getbuf(proc, buf, UDIF_GPIO_PROC_BUFSIZE) == UDIF_GPIO_PROC_BUFSIZE){
		udif_printk("Fail to udif_copy_from_user()\n");
		return UDIF_ERR_FAULT;
	}
	sscanf(buf, "%c %d %c %x %x", &rw, &port, &func, &bitmask, &value);
	udif_printk("%c %d %c %x %x\n", rw, port, func, bitmask, value);

	if (port >= UDIF_NR_GPIO) {
		udif_printk("gpio_write_proc: illegal port:%u\n", port);
		return UDIF_ERR_PAR;
	}
	if(rw == 'r'){
		//if(port < UDIF_NR_GPIO){
		//	proc_port = port;
		//}
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
	return proc->count;
}

#ifdef TEST_GPIO_DRV
int get_gpio_lock_cnt(unsigned char port)
{
    return gpio_lock_cnt[port];
}
int get_gpio_drv_open_cnt( void )
{
    return gpio_drv_open_cnt;
}
void get_gpio_drv_ginfo( unsigned char port, GPIO_INFO *info )
{
    info->base = ginfo[port].base;
    info->reg  = ginfo[port].reg;
    return;
}
void set_gpio_drv_ginfo( unsigned char port, GPIO_INFO *info )
{
    ginfo[port].base = info->base;
    ginfo[port].reg  = info->reg;
    return;
}
void test_gpio_suspend( UDIF_CH ch )
{
    UDIF_DEVICE dev;
    UDIF_VP     data = NULL;

    gpio_suspend( &dev, ch, data);
}
void test_gpio_resume( UDIF_CH ch )
{
    UDIF_DEVICE dev;
    UDIF_VP     data = NULL;

    gpio_resume( &dev, ch, data);
    return;
}
void test_gpio_init( void )
{
    UDIF_VP     data = NULL;

    gpio_init( data );
    return;
}
void test_gpio_exit( void )
{
    UDIF_VP     data = NULL;

    gpio_exit( data );
    return;
}
unsigned int test_lock_log( void )
{
    return gpio_lock_log;
}
void test_lock_log_clr( void )
{
    gpio_lock_log = 0;
    return;
}
#endif /* #ifdef TEST_GPIO_DRV */

EXPORT_SYMBOL(gpio_get_data_bit);
EXPORT_SYMBOL(gpio_get_data_port);
EXPORT_SYMBOL(gpio_set_data_bit);
EXPORT_SYMBOL(gpio_set_data_port);
EXPORT_SYMBOL(gpio_set_mode_bit);
EXPORT_SYMBOL(gpio_set_mode_port);
EXPORT_SYMBOL(gpio_get_mode_bit);
#ifdef TEST_GPIO_DRV
EXPORT_SYMBOL(get_gpio_lock_cnt);
EXPORT_SYMBOL(get_gpio_drv_open_cnt);
EXPORT_SYMBOL(get_gpio_drv_ginfo);
EXPORT_SYMBOL(set_gpio_drv_ginfo);
EXPORT_SYMBOL(test_gpio_suspend);
EXPORT_SYMBOL(test_gpio_resume);
EXPORT_SYMBOL(test_gpio_init);
EXPORT_SYMBOL(test_gpio_exit);
EXPORT_SYMBOL(test_lock_log);
EXPORT_SYMBOL(test_lock_log_clr);
#endif /* #ifdef TEST_GPIO_DRV */

/*---------------------------------------------------------------------------
  END
---------------------------------------------------------------------------*/
