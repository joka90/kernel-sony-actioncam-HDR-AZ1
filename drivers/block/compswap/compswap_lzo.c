/* 2012-05-10: File added by Sony Corporation */
/*
 *  LZO Compressor/Decompressor plugin for CompSwap device
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
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/lzo.h>
#include <linux/compswap.h>

struct compswap_lzo_t {
	void *workmem;
};

static int compswap_lzo_compressor_init(void **priv)
{
	struct compswap_lzo_t *lzo;

	lzo = kzalloc(sizeof(struct compswap_lzo_t), GFP_KERNEL);
	if (lzo == NULL) {
		return -ENOMEM;
	}

	lzo->workmem = kzalloc(LZO1X_MEM_COMPRESS, GFP_KERNEL);
	if (lzo == NULL) {
		return -ENOMEM;
	}

	*priv = (void *) lzo;

	return 0;
}

static void compswap_lzo_compressor_exit(void *priv)
{
	struct compswap_lzo_t *lzo = (struct compswap_lzo_t *) priv;

	if (lzo == NULL)
		return;
	if (lzo->workmem)
		kfree(lzo->workmem);
	kfree(lzo);

	return;
}

static int compswap_lzo_compressor_compress(const void *src, size_t src_len,
			void *dst, size_t *dst_len,
			void *priv)
{
	int ret;
	struct compswap_lzo_t *lzo = (struct compswap_lzo_t *) priv;

	ret = lzo1x_1_compress(src, src_len, dst, dst_len, lzo->workmem);

	if (unlikely(ret != LZO_E_OK)) {
		return ret;
	}
	return 0;
}

static int compswap_lzo_decompressor_init(void **priv)
{
	return 0;
}

static int compswap_lzo_decompressor_decompress(const void *src, size_t src_len,
			void *dst, size_t *dst_len,
			void *priv)
{
	int ret;

	ret = lzo1x_decompress_safe(src, src_len, dst, dst_len);

	if (unlikely(ret != LZO_E_OK)) {
		return ret;
	}
	return 0;
}

static void compswap_lzo_decompressor_exit(void *priv)
{
	return;
}

static struct compswap_compressor_ops lzo_compressor_ops = {
	init:		compswap_lzo_compressor_init,
	exit:		compswap_lzo_compressor_exit,
	compress:	compswap_lzo_compressor_compress,
};

static struct compswap_decompressor_ops lzo_decompressor_ops = {
	init:		compswap_lzo_decompressor_init,
	exit:		compswap_lzo_decompressor_exit,
	decompress:	compswap_lzo_decompressor_decompress,
};

int compswap_lzo_register(void)
{
	int ret;

	ret = compswap_compressor_register(&lzo_compressor_ops);
	if (ret < 0) {
		printk("failed to register compressor\n");
		goto out;
	}
	ret = compswap_decompressor_register(&lzo_decompressor_ops);
	if (ret < 0) {
		printk("failed to register decompressor\n");
		compswap_compressor_unregister(&lzo_compressor_ops);
	}
out:
	return ret;
}

void compswap_lzo_unregister(void)
{
	int ret;

	ret = compswap_compressor_unregister(&lzo_compressor_ops);
	if (ret < 0) {
		printk("failed to unregister compressor\n");
		goto out;
	}
	ret = compswap_decompressor_unregister(&lzo_decompressor_ops);
	if (ret < 0) {
		printk("failed to unregister decompressor\n");
	}
out:
	return;
}

static int __init compswap_lzo_init(void)
{
	printk("Register CompSwap LZO compressor/decompressor\n");
	return compswap_lzo_register();
}

static void __exit compswap_lzo_exit(void)
{
	printk("Unegister CompSwap LZO compressor/decompressor\n");
	compswap_lzo_unregister();
}

module_init(compswap_lzo_init);
module_exit(compswap_lzo_exit);

MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION("CompSwap Device LZO Plugin");
MODULE_LICENSE("GPL v2");
