/* 2013-02-19: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot - Sample image optimizer
 *
 *  Copyright 2012 Sony Corporation
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
#include <linux/module.h>
#include <linux/swap.h>
#include <linux/ssboot.h>

/*
 * Image optimizer operations
 */
static int
ssboot_sample_optimize(void *priv)
{
	unsigned long free, progress = 0;

	ssboot_info("optimizing image...\n");

	/* free all memory */
	ssboot_info("Freeing all memory.");
	do {
		/* free 10MB per loop */
		free = __shrink_all_memory(10 * SSBOOT_PG_1MB, 1, 1);
		/*
		 * __shrink_all_memory() may return non 0 value even
		 * if no pages are reclaimed. Therefore summation of
		 * the return value does not indicate actual reclaimed
		 * pages, and a "." does not mean that 10MB memory is
		 * reclaimed.
		 */
		progress += free;
		if (progress >= (10 * SSBOOT_PG_1MB)) {
			printk(".");
			progress = 0;
		}
	} while (free > 0);
	printk("done\n");

	/*
	 * To include anonymous page into image, use get_user_pages()
	 * for the page.
	 */

	/*
	 * To include page cache into image, use read_mapping_page()
	 * for the page.
	 */

	return 0;
}

static int
ssboot_sample_start_profiling(void *priv)
{
	int *is_profiling = (int*)priv;

	ssboot_info("start profiling...\n");

	*is_profiling = 1;

	return 0;
}

static int
ssboot_sample_stop_profiling(void *priv)
{
	int *is_profiling = (int*)priv;

	ssboot_info("stop profiling...\n");

	*is_profiling = 0;

	return 0;
}

static int
ssboot_sample_is_profiling(void *priv)
{
	int *is_profiling = (int*)priv;

	return *is_profiling;
}

/*
 * Initialization
 */
static ssboot_optimizer_t ssboot_sample_optimizer = {
	.optimize		= ssboot_sample_optimize,
	.start_profiling	= ssboot_sample_start_profiling,
	.stop_profiling		= ssboot_sample_stop_profiling,
	.is_profiling		= ssboot_sample_is_profiling,
};

static int __init
ssboot_sample_optimizer_init_module(void)
{
	int ret;
	int *is_profiling;

	/* allocate memory to store profiling state */
	is_profiling = kzalloc(sizeof(int), GFP_KERNEL);
	if (is_profiling == NULL) {
		return -ENOMEM;
	}
	ssboot_sample_optimizer.priv = (void*)is_profiling;

	/* register sample image optimizer */
	ret = ssboot_optimizer_register(&ssboot_sample_optimizer);
	if (ret < 0) {
		return ret;
	}
	ssboot_dbg("registered sample image optimizer\n");

	return 0;
}

static void __exit
ssboot_sample_optimizer_cleanup_module(void)
{
	int ret;

	/* unregister sample image optimizer */
	ret = ssboot_optimizer_unregister(&ssboot_sample_optimizer);
	if (ret < 0) {
		return;
	}
	ssboot_dbg("unregistered sample image optimizer\n");

	/* free memory to store profiling state */
	kfree(ssboot_sample_optimizer.priv);
}

module_init(ssboot_sample_optimizer_init_module);
module_exit(ssboot_sample_optimizer_cleanup_module);

MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION("Sample image optimizer");
MODULE_LICENSE("GPL v2");
