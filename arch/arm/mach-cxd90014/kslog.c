/*
 * arch/arm/mach-cxd90014/kslog.c
 *
 * kernel simple debug logger
 *
 * Copyright 2012 Sony Corporation
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
#include <linux/module.h>
#include <asm/io.h>
#include <mach/time.h>
#include <mach/kslog.h>

static unsigned long kslog_addr = 0;
module_param_named(addr, kslog_addr, ulongH, S_IRUSR);
static unsigned long kslog_size = 0;
module_param_named(size, kslog_size, ulongH, S_IRUSR);

static DEFINE_RAW_SPINLOCK(kslog_lock);
static char *kslogp = NULL;
static int kslogrest = 0;

void kslog_init(void)
{
	kslogp = (char *)kslog_addr;
	kslogrest = kslog_size;
}

void vksprintf(const char *fmt, va_list args)
{
	ulong t;
	int len;

	if (!kslogrest)
		return;
	raw_spin_lock(&kslog_lock);
	t = read_freerun();
	/* kslogrest should be greater than 0. */
	len = scnprintf(kslogp, kslogrest, "%lu:", mach_cycles_to_usecs(t));
	/* len: [0 .. kslogrest-1] */
	kslogp += len;
	kslogrest -= len;
	/* So, kslogrest is >= 1 */
	len = vscnprintf(kslogp, kslogrest, fmt, args);
	kslogp += len;
	kslogrest -= len;
	if (1 == kslogrest) { /* full ? */
		kslogrest = 0;
	}
	raw_spin_unlock(&kslog_lock);
}

void ksprintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vksprintf(fmt, args);
	va_end(args);
}

void kslog_show(void)
{
	char *p, *s;

	if (!kslog_addr || !kslogp)
		return;
	for (p = (char *)kslog_addr; p < kslogp; p = s + 1) {
		for (s = p; s < kslogp; s++) {
			if ('\n' == *s)
				break;
		}
		if (s == kslogp)
			s--;
		*s = '\0';
		printk(p);
		printk(KERN_CONT "\n");
		p = s + 1;
	}
	printk(KERN_ERR "kslog:rest=%d\n", kslogrest);
}
