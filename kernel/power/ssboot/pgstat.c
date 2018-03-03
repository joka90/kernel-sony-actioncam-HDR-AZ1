/* 2012-02-07: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot Core - page statistics
 *
 *  Copyright 2008,2009,2010 Sony Corporation
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
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <asm/page.h>
#include "internal.h"

/* page statistics data */
static unsigned long pgstat[SSBOOT_PGSTAT_NUM];

/* string list for output */
static const char * const pgstat_str[SSBOOT_PGSTAT_NUM] = {
	[SSBOOT_PGSTAT_KERNEL]	  = "Kernel    ",
	[SSBOOT_PGSTAT_KERNEL_RO] = "Kernel R/O",
	[SSBOOT_PGSTAT_MODULE_RO] = "Module R/O",
	[SSBOOT_PGSTAT_USER]      = "User      ",
	[SSBOOT_PGSTAT_ANON]      = "Anon      ",
	[SSBOOT_PGSTAT_CACHE]     = "Cache     ",
	[SSBOOT_PGSTAT_SWAPCACHE] = "Swapcache ",
	[SSBOOT_PGSTAT_RESERVED]  = "Reserved  ",
	[SSBOOT_PGSTAT_OTHER]     = "Other     ",
};

void
ssboot_pgstat_inc(ssboot_pgstat_item_t item)
{
	/* sanity check */
	if (item >= SSBOOT_PGSTAT_NUM) {
		return;
	}

	/* update page statistics */
	pgstat[item]++;
}

void
ssboot_pgstat_dec(ssboot_pgstat_item_t item)
{
	/* sanity check */
	if (item >= SSBOOT_PGSTAT_NUM) {
		return;
	}

	/* update page statistics */
	if (pgstat[item] > 0) {
		pgstat[item]--;
	}
}

void
ssboot_pgstat_show(void)
{
	int i;

	/* show page statistics */
	ssboot_info("Page Statistics:\n");
	for (i = 0; i < SSBOOT_PGSTAT_NUM; i++) {
		ssboot_info("  %s : %5ld pages (%7ld kB)\n",
			    pgstat_str[i], pgstat[i],
			    pgstat[i] * PAGE_SIZE / 1024);
	}
}

void
ssboot_pgstat_init(void)
{
	/* initialize page statistics */
	memset(pgstat, 0, sizeof(pgstat));
}
