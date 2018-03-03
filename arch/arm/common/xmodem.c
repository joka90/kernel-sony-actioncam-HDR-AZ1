/*
 * arch/arm/common/xmodem.c
 *
 * xmodem protocol implementation
 * for image creation by serial
 *
 * Copyright 2005,2006 Sony Corporation
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
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/mach/warmboot.h>
#include <asm/mach/xmodem.h>
#include <asm/delay.h>

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define X_BLOCK_SIZE 128

static u8 block_no;

static int putc(struct xmodem_device *dev, u8 c){
	return dev->outb(dev, c);
}

static int getc(struct xmodem_device *dev, u8 *c){
	return dev->inb(dev, c);
}

static int getc_timeout(struct xmodem_device *dev, u8 *c, u32 msec){
	int i;
	for(i=0; i<msec*1000; i++){
		if(getc(dev, c)==1)
			return 1;
	}
	return 0;
}

static void printascii(struct xmodem_device *dev, char *s){
	while(*s){
		putc(dev, *s++);
	}
}

static void send_data(struct xmodem_device *dev, u8 *buf, u8 block){
	u8 ck;
	u32 i,ck_i;
	u8 com_block_no;

	block_no += block;
	block_no = block_no % 256;
	com_block_no = ~block_no;

	putc(dev, SOH);
	putc(dev, block_no);
	putc(dev, com_block_no);

	for(ck_i=0,i=0;i<128;i++){
		putc(dev, buf[i]);
		ck_i += buf[i];
	}
	ck_i &= 0xFF;
	ck = (u8)ck_i;
	putc(dev, ck);
}

static int xmodem_device_start(struct wb_device *wb_device)
{
	struct xmodem_device *dev = wb_device->data;
	u8 c;

	if(dev->open)
		dev->open(dev);

	printascii(dev, "\r\nReady: Please set your terminal to Xmodem mode to receive image!\r\n");

	block_no = 0;

	while(getc_timeout(dev, &c, 1)<=0 || c!=NAK)
		if(c == CAN) return -1;

	return 0;
}

static int send_one_block(struct xmodem_device *dev, u8 *buf)
{
	u8 c;

	send_data(dev, buf, 1);
	do{
		if(getc_timeout(dev, &c, 1000)<=0){
			send_data(dev, buf, 0);
			continue;
		}

		if(c == ACK)//ACKed
			return 0;
		else if(c == CAN)//CANCELLed
			return -1;
		else{
			send_data(dev, buf, 0);
			continue;
		}
	}while(1);
}

static int xmodem_device_write(struct wb_device *wb_device,u8 *buf, u32 cur, u32 nsect)
{
	struct xmodem_device *dev = wb_device->data;
	u8 *s = buf;
	u8 *se = buf+(nsect<<9);

	while(s<se){
		send_one_block(dev, s);
		s += X_BLOCK_SIZE;
	}

	return 0;
}

static int xmodem_device_finish(struct wb_device *wb_device)
{
	struct xmodem_device *dev = wb_device->data;
	u8 c;

	putc(dev, EOT);
	while(getc_timeout(dev, &c, 1)<=0 || c!=ACK)
		if(c == CAN) return -1;

	printascii(dev, "Sending image done!\r\n");

	if(dev->close)
		dev->close(dev);

	return 0;
}

int xmodem_register_device(struct xmodem_device *device)
{
	struct wb_device *wb_xmodem;

	if(!device)
		return -1;

	wb_xmodem = kmalloc(sizeof(*wb_xmodem), GFP_KERNEL);
	if(!wb_xmodem){
		printk("xmodem_register_device: kmalloc failed\n");
		return -1;
	}

	wb_xmodem->name = device->name;
	wb_xmodem->sector_size = PAGE_SIZE;
	wb_xmodem->suspend_prepare = NULL;
	wb_xmodem->open = xmodem_device_start;
	wb_xmodem->write_sector = xmodem_device_write;
	wb_xmodem->close = xmodem_device_finish;
	wb_xmodem->suspend_finish = NULL;
	INIT_LIST_HEAD(&wb_xmodem->list);
	wb_xmodem->flag |= WBI_SILENT_MODE;
	wb_xmodem->data = device;

	device->wb_device = wb_xmodem;

	wb_register_device(wb_xmodem, 1);

	return 0;
}

int xmodem_unregister_device(struct xmodem_device *device)
{
	struct wb_device *wb_xmodem = device->wb_device;

	if(!device)
		return -1;

	wb_unregister_device(wb_xmodem);
	device->wb_device = NULL;

	kfree(wb_xmodem);

	return 0;
}

EXPORT_SYMBOL(xmodem_register_device);
EXPORT_SYMBOL(xmodem_unregister_device);
