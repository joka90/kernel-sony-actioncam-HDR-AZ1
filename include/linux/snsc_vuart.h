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

#ifndef _SNSC_VUART_H
#define _SNSC_VUART_H

#include <linux/console.h>

#define SNSC_VUART_MAJOR	CONFIG_SNSC_VUART_MAJOR_NUM
#define SNSC_VUART_MINOR	CONFIG_SNSC_VUART_MINOR_NUM
#define SNSC_VUART_NUM		CONFIG_SNSC_VUART_NR_UARTS
#define SNSC_VUART_CONSOLE	CONFIG_SNSC_VUART_CONSOLE

#define SNSC_VUART_HEADER_SIZE	4
#define SNSC_VUART_HEADER_FLAG	0xff

static inline int snsc_is_vuart_driver(struct tty_driver *driver)
{
	return (driver->major == SNSC_VUART_MAJOR
		&& driver->minor_start == SNSC_VUART_MINOR);
}

static inline int snsc_vuart_header_buf(char *p_vuart_buf, int count)
{
	p_vuart_buf[0] = SNSC_VUART_HEADER_FLAG;
	p_vuart_buf[1] = SNSC_VUART_HEADER_FLAG;
	p_vuart_buf[2] = (count >> 8) & 0xff;
	p_vuart_buf[3] = count & 0xff;

	return SNSC_VUART_HEADER_SIZE;
}

/* Searching for multiplexed console device */
static inline struct console *snsc_vuart_multiplexed_console(void)
{
	struct console *con = NULL;
	char con_name[16];

	console_lock();
	for_each_console(con) {
		if (con->flags & CON_ENABLED) {
			snprintf(con_name, sizeof(con_name),
				"%s%d", con->name, con->index);
			if (!strncmp(con_name, SNSC_VUART_CONSOLE,
				sizeof(con_name)))
				break;
		}
	}
	console_unlock();
	return con;
}

int snsc_vuart_bind_buddy(struct tty_struct *v_tty);
void snsc_vuart_unbind_buddy(void);

#endif
