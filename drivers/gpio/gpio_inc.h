/*
 * drivers/gpio/gpio_inc.h
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
#ifndef __GPIO_INC_H__
#define __GPIO_INC_H__

#ifdef __cplusplus
extern "C" {
#endif

//#define TEST_GPIO_DRV
//#define TEST_GPIO_DRV_CONFLICT
//#include <linux/udif/delay.h>

/*--------------------------------------------------------------------------*/
// Register assign
/*--------------------------------------------------------------------------*/
// GPIO REGISTER
#define GPIO_DIR                0x00U
#define GPIO_DIR_SET            0x04U
#define GPIO_DIR_CLR            0x08U
/* NotUsed                      0x0CU */
#define GPIO_RDATA              0x10U
/* NotUsed                      0x14U */
/* NotUsed                      0x18U */
/* NotUsed                      0x1CU */
#define GPIO_INEN               0x20U
#define GPIO_INEN_SET           0x24U
#define GPIO_INEN_CLR           0x28U
/* NotUsed                      0x2CU */
#define GPIO_PORTSEL            0x30U
#define GPIO_PORTSEL_SET        0x34U
#define GPIO_PORTSEL_CLR        0x38U
/* NotUsed                      0x3CU */
#define GPIO_WDATA              0x40U
#define GPIO_WDATA_SET          0x44U
#define GPIO_WDATA_CLR          0x48U
/* NotUsed                      0x4CU */

#define GPIO_REG_DIRECTION(x)       (ginfo[x].base + GPIO_DIR)
#define GPIO_REG_DIRECTION_SET(x)   (ginfo[x].base + GPIO_DIR_SET)
#define GPIO_REG_DIRECTION_CLR(x)   (ginfo[x].base + GPIO_DIR_CLR)
#define GPIO_REG_RDATA(x)           (ginfo[x].base + GPIO_RDATA)
#define GPIO_REG_INPUTEN(x)         (ginfo[x].base + GPIO_INEN)
#define GPIO_REG_INPUTEN_SET(x)     (ginfo[x].base + GPIO_INEN_SET)
#define GPIO_REG_INPUTEN_CLR(x)     (ginfo[x].base + GPIO_INEN_CLR)
#define GPIO_REG_PORTSEL(x)         (ginfo[x].base + GPIO_PORTSEL)
#define GPIO_REG_PORTSEL_SET(x)     (ginfo[x].base + GPIO_PORTSEL_SET)
#define GPIO_REG_PORTSEL_CLR(x)     (ginfo[x].base + GPIO_PORTSEL_CLR)
#define GPIO_REG_WDATA(x)           (ginfo[x].base + GPIO_WDATA)
#define GPIO_REG_WDATA_SET(x)       (ginfo[x].base + GPIO_WDATA_SET)
#define GPIO_REG_WDATA_CLR(x)       (ginfo[x].base + GPIO_WDATA_CLR)

#define GPIOMODE_FUNC   (GPIOMODE_GPIO  | GPIOMODE_PERIPHERAL)
#define GPIOMODE_DIR    (GPIOMODE_INPUT | GPIOMODE_OUTPUT_H | GPIOMODE_OUTPUT_L)

#define GPIO_PORT_H                     1U
#define GPIO_PORT_L                     0

#define GPIO_VERSION                    "1.0"                                   /**< Driver SW Version */
#define DEV_NAME                        "gpio"                                  /**< Driver Name */

#define UDIF_VALID_MASK_GPIO            ((1 << UDIF_NR_GPIO) - 1)

#define GPIO_PROC_NAME                  "gpio"

#define GPIO_REG_ADDR_INIT              (0xFFFF8000U)

/*--------------------------------------------------------------------------*/
// IO access inline functions
/*--------------------------------------------------------------------------*/
#define gpio_read(addr)         udif_ioread32(addr)
#define gpio_write(val, addr)   udif_iowrite32(val, addr)

/*--------------------------------------------------------------------------*/
// lock/unlock inline functions
/*--------------------------------------------------------------------------*/
#ifndef TEST_GPIO_DRV
#define gpio_lock(port, flags)      udif_spin_lock_irqsave(&ginfo[port].gpio_lock, flags)
#define gpio_unlock(port, flags)    udif_spin_unlock_irqrestore(&ginfo[port].gpio_lock, flags)
#else  /* #ifndef TEST_GPIO_DRV */
#ifndef TEST_GPIO_DRV_CONFLICT
#define gpio_lock(port, flags)                                                  \
    {                                                                           \
        gpio_lock_cnt[port]++;                                                  \
        udif_spin_lock_irqsave(&ginfo[port].gpio_lock, flags);                  \
    }

#define gpio_unlock(port, flags)                                                \
    {                                                                           \
        udif_spin_unlock_irqrestore(&ginfo[port].gpio_lock, flags);             \
        gpio_lock_cnt[port]--;                                                  \
    }
#else  /* #ifndef TEST_GPIO_DRV_CONFLICT */
#define GPIO_LOCK_SINGLE          0x01UL
#define GPIO_LOCK_COMB            0x02UL
#define GPIO_LOCK_COUNT           240U      /* Cnt * 100usec. */
#define gpio_lock(port, flags)                                                  \
    {                                                                           \
        gpio_lock_cnt[port]++;                                                  \
        udif_spin_lock_irqsave(&ginfo[port].gpio_lock, flags);                  \
        if( (gpio_lock_log & GPIO_LOCK_SINGLE) != 0x00 ){                       \
            gpio_lock_log |= GPIO_LOCK_COMB;                                    \
        }                                                                       \
        gpio_lock_log |= GPIO_LOCK_SINGLE;                                      \
    }
#define gpio_unlock(port, flags)                                                \
    {                                                                           \
        int loop_cnt = 0;                                                       \
        for(loop_cnt=0; loop_cnt<GPIO_LOCK_COUNT; loop_cnt++) udif_udelay(100); \
        gpio_lock_log &= ~GPIO_LOCK_SINGLE;                                     \
        udif_spin_unlock_irqrestore(&ginfo[port].gpio_lock, flags);             \
        gpio_lock_cnt[port]--;                                                  \
    }
#endif /* #ifndef TEST_GPIO_DRV_CONFLICT */
#endif /* #ifndef TEST_GPIO_DRV */

#ifdef NDEBUG
#define gpio_assert_NULL(b)
#else /* #ifdef NDEBUG */
#define gpio_assert_NULL(b)                                                     \
    if ((b) == NULL)                                                            \
    {                                                                           \
        printk(KERN_ERR " %s(%d) %p *** ASSERT \n",__FILE__,__LINE__,(b));      \
        BUG();                                                                  \
    }
#endif

typedef struct gpio_reg{
    UDIF_U32 dir;
    UDIF_U32 inen;
    UDIF_U32 portsel;
    UDIF_U32 wdata;
}GPIO_REG;

typedef struct gpio_info{
    UDIF_VA         base;
    GPIO_REG        reg;
    UDIF_SPINLOCK   gpio_lock;
}GPIO_INFO;

static inline UDIF_U32 mask_val(UDIF_U8 bit);
static inline UDIF_U32 gpio_data_lsr(UDIF_U8 bit, UDIF_U32 data);
static inline UDIF_U32 gpio_get_port( UDIF_U32 bitmask, const UDIF_VA addr);

static UDIF_INT hw_gpio_get_data_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32* data);
static UDIF_INT hw_gpio_get_data_port( UDIF_U8 port, UDIF_U32 bitmask, UDIF_U32* data );
static UDIF_INT hw_gpio_set_data_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32 data);
static UDIF_INT hw_gpio_set_data_port(UDIF_U8 port, UDIF_U32 bitmask, UDIF_U32 data);
static UDIF_INT hw_gpio_set_mode_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32 mode);
static UDIF_INT hw_gpio_set_mode_port(UDIF_U8 port, UDIF_U32 bitmask, UDIF_U32 mode);
static UDIF_INT hw_gpio_get_mode_bit(UDIF_U8 port, UDIF_U32 bit, UDIF_U32* mode);

static UDIF_ERR gpio_open(UDIF_FILE *file);
static UDIF_ERR gpio_close(UDIF_FILE *file);
static UDIF_ERR gpio_ioctl(UDIF_FILE *file, UDIF_IOCTL *ictl);
static UDIF_ERR gpio_probe(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR gpio_remove(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR gpio_suspend(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR gpio_resume(const UDIF_DEVICE *dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR gpio_init(UDIF_VP data);
static UDIF_ERR gpio_exit(UDIF_VP data);
static int gpio_read_proc(UDIF_PROC_READ *proc);
static int gpio_write_proc(UDIF_PROC_WRITE *proc);
#ifdef __cplusplus
}
#endif

#endif /*__GPIO_INC_H__*/
