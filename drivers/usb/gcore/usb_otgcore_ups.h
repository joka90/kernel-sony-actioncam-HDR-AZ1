/*--------------------------------------------------------------------------*
 * USB port selector API
 * 
 * Copyright 2013 Sony Corporation
 *---------------------------------------------------------------------------*/

#ifndef _UPS_H_
#define _UPS_H_

/*
 *	ポート制御情報構造体
 */

enum ups_io_type {
        UPS_IO_TYPE_HFIX,
        UPS_IO_TYPE_LFIX,
        UPS_IO_TYPE_MISC,
        UPS_IO_TYPE_GPIO,
        UPS_IO_TYPE_GPIOX,
};

struct ups_io {
        enum ups_io_type type;
        unsigned char gpio_port;
        unsigned int gpio_bit;
};

#define UPS_MAX_PORT_NUM	2

struct ups_port_descriptor {
        unsigned char port_num;     /* number of ports */
        struct {
                struct ups_io cid;
                struct ups_io vbus;
	} port[UPS_MAX_PORT_NUM];
	struct ups_io select;
};

#endif  /* _UPS_H_ */
