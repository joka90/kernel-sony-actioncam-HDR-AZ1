/* 2011-02-25: File added and changed by Sony Corporation */
/*
 *  linux/drivers/char/null_console.c
 *
 *  NULL console
 *
 *  Copyright 2002 Sony Corporation.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License.
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
 *  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/snsc_major.h>

#ifndef SNSC_TTYNULL_MAJOR
#define SNSC_TTYNULL_MAJOR            241
#endif

/*
 *  functions and struct as a console driver
 */
static void
nullcon_write(struct console *c, const char *s, unsigned count) { }


static struct tty_driver *ttynull_driver;
static struct tty_driver *
nullcon_device(struct console *c, int *index)
{
	*index = c->index;
	return ttynull_driver;
}

/*
static int
nullcon_wait_key(struct console *c)
{
        return 0;
}
*/

static struct console nullcon = {
        name:           "ttynull",
        write:          nullcon_write,
        device:         nullcon_device,
/*        wait_key:       nullcon_wait_key, */
        flags:          CON_PRINTBUFFER,
        index:          -1,
};

/*
 *  functions as a tty driver
 */
static int
ttynull_open (struct tty_struct *tty, struct file *filp)
{
	return 0;
}

static int
ttynull_write(struct tty_struct * tty,
              const unsigned char *buf, int count)
{
        return count;
}

static int
ttynull_write_room(struct tty_struct *tty)
{
        return 80;
}

static int
ttynull_chars_in_buffer (struct tty_struct *tty)
{
	/* We have no buffer.  */
	return 0;
}

static struct tty_operations ops = {
	.open            = ttynull_open,
	.write           = ttynull_write,
	.write_room      = ttynull_write_room,
	.chars_in_buffer = ttynull_chars_in_buffer,
};

int __init
nullcon_init(void)
{
	struct tty_driver *driver = alloc_tty_driver(1);
	int err;
	if (!driver)
		return -ENOMEM;
	driver->name            = "ttynull";
	driver->major           = SNSC_TTYNULL_MAJOR;
	driver->minor_start     = 0;
	driver->init_termios    = tty_std_termios;
	tty_set_operations(driver, &ops);

	err = tty_register_driver(driver);;
	if (err)
		panic("Couldn't register ttynull driver\n");

	ttynull_driver = driver;

        register_console(&nullcon);
        printk("ttynull: NULL console driver $Revision: 1.1.2.1 $\n");
        return 0;
}

void __exit
nullcon_exit(void)
{
	if (tty_unregister_driver(ttynull_driver))
		printk("ttynull: failed to unregister ttynull driver\n");

}

module_init(nullcon_init);
module_exit(nullcon_exit);
