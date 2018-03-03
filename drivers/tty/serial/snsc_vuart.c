/* 2013-03-14: File added by Sony Corporation */
/*
 *  Virtual Uart Driver for serial multiplexing
 *
 *  Copyright 2013 Sony Corporation
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
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/snsc_vuart.h>

static struct uart_driver snsc_vuart_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "vuart",
	.dev_name		= "ttyV",
	.major			= SNSC_VUART_MAJOR,
	.minor			= SNSC_VUART_MINOR,
	.nr			= SNSC_VUART_NUM,
	.cons			= NULL,
};

/**
 *	snsc_vuart_bind_buddy
 *	@v_tty: tty_struct of virtual device
 *
 *	Binding multiplexed serial and virtual uart
 */
int snsc_vuart_bind_buddy(struct tty_struct *vurt_tty)
{
	int ret = 0, idx;
	struct console *console;
	struct tty_struct *cons_tty;
	struct tty_driver *cons_tdriver, *vurt_tdriver;
	struct uart_driver *cons_udriver, *vurt_udriver;
	static unsigned int mapping_once;

	/* Binding multiplexed serial and virtual uart */
	console = snsc_vuart_multiplexed_console();
	if (console != NULL) {
		cons_tdriver = console->device(console, &idx);
		cons_tty = cons_tdriver->ttys[console->index];
		cons_tty->buddy = vurt_tty;
	}

	/* Mapping state of virtual dev & console dev once*/
	if (mapping_once)
		goto out;

	vurt_tdriver = vurt_tty->driver;
	if (console != NULL) {
		cons_tdriver = console->device(console, &idx);
	} else {
		printk(KERN_ERR "Vuart: Failed to mapping virtual uart\n");
		ret = -ENXIO;
		goto out;
	}

	/*
	 * Mapping to multiplexed serial
	 * By following way, vuart device will be able to share the same
	 * low level driver with current using console device.
	 */
	cons_udriver = (struct uart_driver *)cons_tdriver->driver_state;
	vurt_udriver = (struct uart_driver *)vurt_tdriver->driver_state;
	vurt_udriver->state = cons_udriver->state;

	mapping_once = 1;
out:
	return ret;
}

/**
 *	snsc_vuart_unbind_buddy
 *	@v_tty: tty_struct of virtual device
 *
 *	Unbind multiplexed serial and virtual uart
 */
void snsc_vuart_unbind_buddy(void)
{
	struct tty_driver *console_driver;
	struct tty_struct *console_tty;
	struct console *console;
	int idx;

	/* Locate current console device */
	console = snsc_vuart_multiplexed_console();
	if (console != NULL) {
		console_driver = console->device(console, &idx);
		console_tty = console_driver->ttys[console->index];
		console_tty->buddy = NULL;
	}
}

static int __init snsc_vuart_init(void)
{
	int ret;
	struct tty_driver *driver;

	printk(KERN_INFO "Serial: virtual uart driver, "
		"%d ports\n", snsc_vuart_reg.nr);

	/* Registering vuart to uart core layer */
	ret = uart_register_driver(&snsc_vuart_reg);

	/* Modify vuart flags */
	driver = snsc_vuart_reg.tty_driver;
	driver->init_termios.c_lflag &= ~(ICANON | ECHO);

	return ret;
}

static void __exit snsc_vuart_exit(void)
{
	uart_unregister_driver(&snsc_vuart_reg);
}

module_init(snsc_vuart_init);
module_exit(snsc_vuart_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Generic virtual uart driver");
