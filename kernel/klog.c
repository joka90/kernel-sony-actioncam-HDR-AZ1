/*
 * kernel/klog.c
 *
 * memory console driver
 * logging printk() to physical memory
 *
 * Copyright 2013 Sony Corporation
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

#include <linux/module.h>
#include <linux/console.h>
#include <linux/proc_fs.h>
#include <linux/sysdev.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <asm/io.h>

#define KLOG_SIZE_MIN 0x1000 /* 4 KB */

static unsigned long klog_addr = 0;
module_param_named(addr, klog_addr, ulongH, S_IRUSR);
static unsigned long klog_size = 0;
module_param_named(size, klog_size, ulongH, S_IRUSR);

#define KLOG_NAME	"klog"
#define KLOG_PROCFILE	KLOG_NAME

#define KLOG_MAGIC_VAL 0x676f6c6b /* klog */

/* header offset */
#define KLOG_MAGIC	0
#define KLOG_OFFSET	4
#define KLOG_FLAG	8
#define KLOG_START	12

/* flag bit */
enum klog_flag_t {
	KLOG_FLAG_LOOPED = 0,
};

static unsigned int klog_buf_size;

#define KLOG_IOADDR(off)	((void __iomem *)((u8 __iomem *)klog->base + (off)))

#define KLOG_PR_INFO(fmt, args...)	printk(KERN_INFO KLOG_NAME ": " fmt, ##args)
#define KLOG_PR_ERR(fmt, args...)	printk(KERN_ERR KLOG_NAME ": " fmt, ##args)

struct klog_t {
	void __iomem *base;
	struct klog_ops_t *ops;
	raw_spinlock_t lock;
};

struct klog_ops_t {
	ssize_t (*write)(struct klog_t *klog, const char *buf, off_t *pos, size_t count);
	ssize_t (*read)(struct klog_t *klog, char *buf, off_t *pos, size_t count);
};

enum klog_dir_t {
	KLOG_DIR_READ = 0,
	KLOG_DIR_WRITE,
};

/*
 * transfer operation
 * read: src == NULL
 * write: dst == NULL
 */
static size_t klog_transfer(struct klog_t *klog,
			    char *dst, const char *src, off_t start, off_t *pos,
			    size_t count, size_t total)
{
	size_t tail = 0;
	off_t offset = (start + *pos) % total; /* mask */
	enum klog_dir_t dir = src ? KLOG_DIR_WRITE : KLOG_DIR_READ;

	if (offset + count >= total) {
		/* buffer looped */
		tail = total - offset;

		if (dir == KLOG_DIR_WRITE) {
			memcpy_toio(KLOG_IOADDR(KLOG_START + offset), src, tail);
			__set_bit(KLOG_FLAG_LOOPED, (volatile unsigned long __force *)KLOG_IOADDR(KLOG_FLAG));
			src += tail;
		} else {
			memcpy_fromio(dst, KLOG_IOADDR(KLOG_START + offset), tail);
			dst += tail;
		}

		*pos += tail;
		count -= tail;
		offset = 0;
	}

	if (dir == KLOG_DIR_WRITE) {
		memcpy_toio(KLOG_IOADDR(KLOG_START + offset), src, count);
		iowrite32(offset + count, KLOG_IOADDR(KLOG_OFFSET));
	} else {
		memcpy_fromio(dst, KLOG_IOADDR(KLOG_START + offset), count);
	}

	*pos += count;

	return tail + count;
}

static ssize_t klog_write(struct klog_t *klog, const char *buf, off_t *pos, size_t count)
{
	u32 start;
	unsigned long flags;

	if (count > klog_buf_size)
		return -ENOSPC;

	raw_spin_lock_irqsave(&klog->lock, flags);

	start = ioread32(KLOG_IOADDR(KLOG_OFFSET));
	count = klog_transfer(klog, NULL, buf, start, pos, count, klog_buf_size);

	raw_spin_unlock_irqrestore(&klog->lock, flags);

	return count;
}

static ssize_t klog_read(struct klog_t *klog, char *buf, off_t *pos, size_t count)
{
	u32 total, start;
	unsigned long flags;

	raw_spin_lock_irqsave(&klog->lock, flags);

	if (test_bit(KLOG_FLAG_LOOPED, (volatile unsigned long __force *)KLOG_IOADDR(KLOG_FLAG))) {
		total = klog_buf_size;
		start = ioread32(KLOG_IOADDR(KLOG_OFFSET));
	} else {
		total = ioread32(KLOG_IOADDR(KLOG_OFFSET));
		start = 0;
	}

	if (*pos >= total) {
		count = 0; /* EOF */
		goto out;
	}

	if (count >= total - *pos)
		count = total - *pos;

	count = klog_transfer(klog, buf, NULL, start, pos, count, total);

out:
	raw_spin_unlock_irqrestore(&klog->lock, flags);

	return count;
}

static struct klog_ops_t klog_ops = {
	.write	= klog_write,
	.read	= klog_read,
};

static struct klog_t klog_dev = {
	.base	= NULL,
	.ops	= &klog_ops,
	.lock	= __RAW_SPIN_LOCK_UNLOCKED(klog_dev.lock),
};

#define KLOG_ERR_MSG "klog write error\n"
static void klog_console_write(struct console *co, const char *buf, unsigned int count)
{
	struct klog_t *klog = co->data;
	off_t pos = 0;

	if (klog->ops->write(klog, buf, &pos, count) < 0) {
		size_t len = strlen(KLOG_ERR_MSG);
		if (len < klog_buf_size) {
			pos = 0;
			klog->ops->write(klog, KLOG_ERR_MSG, &pos, len);
		}
	}
}

static struct console klog_console = {
	.name	= KLOG_NAME,
	.write	= klog_console_write,
	.read	= NULL,
	.device	= NULL,
	.setup	= NULL,
	.flags	= CON_ENABLED | CON_PRINTBUFFER,
	.index	= 0,
	.data	= &klog_dev,
};

static int klog_proc_read(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	struct klog_t *klog = data;
	off_t pos = offset;

	count = klog->ops->read(klog, page, &pos, count);

	*start = (char *)(pos - offset);

	if (*start >= page) {
		KLOG_PR_ERR("too large *start: 0x%08x, (page = 0x%08x)\n", (unsigned int)*start, (unsigned int)page);
		*start = NULL;
		count = -EFAULT;
	}

	return count;
}

static inline void klog_init_header(struct klog_t *klog)
{
	iowrite32(0, KLOG_IOADDR(KLOG_OFFSET));
	iowrite32(0, KLOG_IOADDR(KLOG_FLAG));
}

static inline void klog_init_magic(struct klog_t *klog)
{
	iowrite32(KLOG_MAGIC_VAL, KLOG_IOADDR(KLOG_MAGIC));
}

static inline u32 klog_read_magic(struct klog_t *klog)
{
	return ioread32(KLOG_IOADDR(KLOG_MAGIC));
}

static inline int klog_wrong_nbit(u32 x, u32 y)
{
	u32 v = x ^ y;
	int n = 0;

	while (v) {
		n += (v & 0x1);
		v >>= 1;
	}

	return n;
}

#define KLOG_MAGIC_WRONG_BIT_THRES 3
static inline int klog_check_header(struct klog_t *klog, int *wrong)
{
	u32 magic = ioread32(KLOG_IOADDR(KLOG_MAGIC));
	u32 offset = ioread32(KLOG_IOADDR(KLOG_OFFSET));

	*wrong = klog_wrong_nbit(magic, KLOG_MAGIC_VAL);

	if (*wrong < KLOG_MAGIC_WRONG_BIT_THRES &&
	    offset < klog_buf_size)
		return 1;
	else
		return 0;
}

static void klog_salvage(struct klog_t *klog)
{
	int wrong;

	if (klog_check_header(klog, &wrong)) {
		KLOG_PR_INFO("keep logging buffer: 0x%08lx@0x%08lx, magic wrong nbit: %d\n", klog_size, klog_addr, wrong);
	} else {
		KLOG_PR_INFO("initialize logging buffer: 0x%08lx@0x%08lx, magic wrong nbit: %d(0x%08lx)\n", klog_size, klog_addr, wrong, (unsigned long)klog_read_magic(klog));
		klog_init_header(klog);
	}

	if (wrong)
		klog_init_magic(klog);
}

static inline int klog_alloc(struct klog_t *klog)
{
	if (!(klog->base = ioremap_nocache(klog_addr, klog_size))) {
		KLOG_PR_ERR("cannot ioremap(0x%08lx, 0x%08lx)\n", klog_addr, klog_size);
		return -1;
	}
	klog_buf_size = klog_size - KLOG_START;

	klog_salvage(klog);

	return 0;
}

static inline void klog_free(struct klog_t *klog)
{
	iounmap(klog->base);
}

static void klog_resume(void)
{
#ifdef CONFIG_WARM_BOOT_IMAGE
	if (PM_SUSPEND_DISK == pm_get_state() && !pm_is_mem_alive()) {
		/* To keep exception log */
		klog_salvage(&klog_dev);
	}
#endif /* CONFIG_WARM_BOOT_IMAGE */
}

static struct sysdev_class klog_sysclass = {
	.name		= KLOG_NAME,
};

static struct syscore_ops klog_sysclass_ops = {
#ifdef CONFIG_PM
	.resume		= klog_resume,
#endif
};

static struct sys_device klog_sysdev = {
	.id		= 0,
	.cls		= &klog_sysclass,
};

static int __init klog_init(void)
{
	int ret = 0;

	if (!klog_addr) {
		KLOG_PR_ERR("klog.addr is not valid.\n");
		return -ENOMEM;
	}
	if (klog_size < KLOG_SIZE_MIN) {
		KLOG_PR_ERR("Too small klog.size: specify more than 4KB.\n");
		return -ENOMEM;
	}
	if (klog_alloc(&klog_dev) < 0)
		return -ENOMEM;

	if (!create_proc_read_entry(KLOG_PROCFILE, 0, NULL, klog_proc_read, &klog_dev)) {
		KLOG_PR_ERR("cannot create proc entry: %s\n", KLOG_PROCFILE);
		ret = -ENOMEM;
		goto err1;
	}
	ret = sysdev_class_register(&klog_sysclass);
	if (ret) {
		KLOG_PR_ERR("sysdev_class_register failed.\n");
		goto err2;
	}
	register_syscore_ops(&klog_sysclass_ops);

	ret = sysdev_register(&klog_sysdev);
	if (ret) {
		KLOG_PR_ERR("sysdev_register failed.\n");
		goto err3;
	}

	/* return type is void */
	register_console(&klog_console);

	return 0;

 err3:
	sysdev_class_unregister(&klog_sysclass);
 err2:
	remove_proc_entry(KLOG_PROCFILE, NULL);
 err1:
	klog_free(&klog_dev);
	return ret;
}

static void __exit klog_exit(void)
{
	unregister_console(&klog_console);

	sysdev_unregister(&klog_sysdev);
	sysdev_class_unregister(&klog_sysclass);
	unregister_syscore_ops(&klog_sysclass_ops);
	remove_proc_entry(KLOG_PROCFILE, NULL);

	klog_free(&klog_dev);
}

module_init(klog_init);
module_exit(klog_exit);

MODULE_LICENSE("GPL");
