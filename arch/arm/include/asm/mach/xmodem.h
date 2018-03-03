/*
 * include/asm-arm/mach/xmodem.h
 *
 * xmodem device header for warmboot
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

#ifndef __XMODEM_H__
#define __XMODEM_H__

#if defined(CONFIG_WBI_DEV_XMODEM)
#include <asm/mach/warmboot.h>

struct xmodem_device {
	char *name;
	struct wb_device *wb_device;
	void (*open)(struct xmodem_device *);
	int (*inb)(struct xmodem_device *, unsigned char *);
	int (*outb)(struct xmodem_device *, unsigned char);
	void (*close)(struct xmodem_device *);
	void *data;
};

int xmodem_register_device(struct xmodem_device *);
int xmodem_unregister_device(struct xmodem_device *);
#else
#define xmodem_register_device(dev) do{}while(0);
#define xmodem_unregister_device(dev) do{}while(0);
#endif

#endif

