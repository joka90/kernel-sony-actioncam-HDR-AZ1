/*
 * include/linux/gpio/gpio.h
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
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*/
// define
/*--------------------------------------------------------------------------*/
#define GPIOMODE_GPIO                   (0x00000001U)
#define GPIOMODE_PERIPHERAL             (0x00000002U)

#define GPIOMODE_INPUT                  (0x00000004U)
#define GPIOMODE_OUTPUT_H               (0x00000008U)
#define GPIOMODE_OUTPUT_L               (0x00000010U)

#define GPIO_CMD_READ                   (0xC0104700U)
#define GPIO_CMD_WRITE                  (0xC0104701U)
#define GPIO_CMD_SET_MODE               (0xC0104702U)
#define GPIO_CMD_GET_MODE               (0xC0104703U)

#define GPIO_PORT_MAX                   UDIF_NR_GPIO
#define GPIO_PORT_0                     0U
#define GPIO_PORT_1                     1U
#define GPIO_PORT_2                     2U
#define GPIO_PORT_3                     3U
#define GPIO_PORT_4                     4U
#define GPIO_PORT_5                     5U
#define GPIO_PORT_6                     6U
#define GPIO_PORT_7                     7U
#define GPIO_PORT_8                     8U
#define GPIO_PORT_9                     9U
#define GPIO_PORT_10                    10U
#define GPIO_PORT_11                    11U
#define GPIO_PORT_12                    12U
#define GPIO_PORT_13                    13U
#define GPIO_PORT_14                    14U
#define GPIO_PORT_15                    15U
#define GPIO_PORT_16                    16U
#define GPIO_PORT_17                    17U

typedef enum gpio_portctrl
{
    GPIO_CTRL_BIT = 0,
    GPIO_CTRL_PORT,
}GPIO_PORTCTRL;

/*--------------------------------------------------------------------------*/
// struct
/*--------------------------------------------------------------------------*/
typedef struct gpio_ioctl_port
{
    GPIO_PORTCTRL       portctl;                            /**< port contrl flag(BIT/PORT) */
    unsigned char       portid;                             /**< Port number */
    union{
        unsigned char   bitnum;                             /**< bit number */
        unsigned int    bitmask;                            /**< bitmask */
    };
    unsigned int value;                                     /**< set/get value */
}GPIO_PORT;


#ifdef __KERNEL__
/* kernel-API */
extern int gpio_get_data_bit(unsigned char port, unsigned int bit, unsigned int* data);
extern int gpio_get_data_port(unsigned char port, unsigned int bitmask, unsigned int* data );
extern int gpio_set_data_bit(unsigned char port, unsigned int bit, unsigned int data);
extern int gpio_set_data_port(unsigned char port, unsigned int bitmask, unsigned int data);
extern int gpio_set_mode_bit(unsigned char port, unsigned int bit, unsigned int mode);
extern int gpio_set_mode_port(unsigned char port, unsigned int bitmask, unsigned int mode);
extern int gpio_get_mode_bit(unsigned char port, unsigned int bit, unsigned int* mode);
#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /*__GPIO_H__*/
