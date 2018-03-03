/*
 * drivers/udif/cdev.c
 *
 * UDM
 *
 * Copyright 2012,2013 Sony Corporation
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
#include <linux/fs.h>
#include <linux/udif/cdev.h>
#include <linux/udif/boottime.h>
#include <linux/udif/macros.h>

#define UDIF_DECLARE_CDEV_READ(name, b, c, p) \
UDIF_CDEV_READ name = { \
	.uaddr = b, \
	.count = c, \
	.pos = p, \
}

#define UDIF_DECLARE_CDEV_WRITE(name, b, c, p) \
UDIF_CDEV_WRITE name = { \
	.uaddr = b, \
	.count = c, \
	.pos = p, \
}

#define UDIF_DECLARE_CDEV_LSEEK(name, w, o, p) \
UDIF_CDEV_LSEEK name = { \
	.whence = w, \
	.offset = o, \
	.pos = p, \
}

#define FILE_DEVNO(x)	((x)->f_dentry->d_inode->i_rdev)
#define INODE2DEV(x)	container_of((x)->i_cdev, UDIF_CDEV, cdev)

static int cdev_open(struct inode *inode, struct file *filp)
{
	UDIF_CDEV *dev = INODE2DEV(inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);
	UDIF_ERR err;

	/* avoid to exec rmmod while openning driver */
	if (unlikely(!try_module_get(dev->owner))) {
		UDIF_PERR("Unable get module %s\n", dev->node->name);
		return UDIF_ERR_IO;
	}

	filp->private_data = dev->data;

	err = UDIF_CALL_FN(dev, open, UDIF_ERR, UDIF_ERR_OK, &file);

	if (err != UDIF_ERR_OK) {
		module_put(dev->owner);
	}

	return err;
}

static int cdev_close(struct inode *inode, struct file *filp)
{
	UDIF_CDEV *dev = INODE2DEV(inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);
	UDIF_ERR err;

	err = UDIF_CALL_FN(dev, close, UDIF_ERR, UDIF_ERR_OK, &file);

	module_put(dev->owner);

	return err;
}

static long cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	UDIF_CDEV *dev = INODE2DEV(filp->f_dentry->d_inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);
	UDIF_DECLARE_IOCTL(ictl, cmd, arg);

	return UDIF_CALL_FN(dev, ioctl, UDIF_ERR, UDIF_ERR_NOSYS, &file, &ictl);
}

static ssize_t cdev_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	UDIF_CDEV *dev = INODE2DEV(filp->f_dentry->d_inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);
	UDIF_DECLARE_CDEV_READ(rd, buf, count, ppos);

	return UDIF_CALL_FN(dev, read, UDIF_SSIZE, UDIF_ERR_NOSYS, &file, &rd);
}

static ssize_t cdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	UDIF_CDEV *dev = INODE2DEV(filp->f_dentry->d_inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);
	UDIF_DECLARE_CDEV_WRITE(wr, buf, count, ppos);

	return UDIF_CALL_FN(dev, write, UDIF_SSIZE, UDIF_ERR_NOSYS, &file, &wr);
}

static loff_t cdev_lseek(struct file *filp, loff_t offset, int whence)
{
	UDIF_CDEV *dev = INODE2DEV(filp->f_dentry->d_inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);
	UDIF_DECLARE_CDEV_LSEEK(sk, whence, offset, &filp->f_pos);

	return UDIF_CALL_FN(dev, lseek, UDIF_LOFF, UDIF_ERR_NOSYS, &file, &sk);
}

static int cdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	UDIF_CDEV *dev = INODE2DEV(filp->f_dentry->d_inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);

	return UDIF_CALL_FN(dev, mmap, UDIF_ERR, UDIF_ERR_NOSYS, &file, vma);
}

static unsigned int cdev_poll(struct file *filp, poll_table *poll)
{
	UDIF_CDEV *dev = INODE2DEV(filp->f_dentry->d_inode);
	UDIF_DECLARE_FILE(file, FILE_DEVNO(filp), &filp->private_data);

	return UDIF_CALL_FN(dev, poll, UDIF_UINT, UDIF_POLLERR, &file, poll);
}

static struct file_operations cdev_fops = {
	.open	= cdev_open,
	.release = cdev_close,
	.read	= cdev_read,
	.write	= cdev_write,
	.llseek	= cdev_lseek,
	.mmap	= cdev_mmap,
	.poll	= cdev_poll,
	.unlocked_ioctl	= cdev_ioctl,
};

UDIF_ERR udif_cdev_register(UDIF_CDEV *cdev)
{
	int ret;
	UDIF_DEVNO devno;

	UDIF_PARM_CHK(!cdev, "invalid cdev", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!cdev->node, "invalid node", UDIF_ERR_PAR);
	UDIF_PARM_CHK(cdev->node->type != UDIF_TYPE_CDEV, "nodetype is not cdev", UDIF_ERR_PAR);

	devno = UDIF_MKDEV(cdev->node->major, cdev->node->first_minor);

	ret = register_chrdev_region(devno, cdev->node->nr_minor, cdev->node->name);
	if (unlikely(ret)) {
		UDIF_PERR("%s: failed register_chdev_region(), ret = %d\n", cdev->node->name, ret);
		return UDIF_ERR_NOMEM;
	}

	cdev_init(&cdev->cdev, &cdev_fops);

	ret = cdev_add(&cdev->cdev, devno, cdev->node->nr_minor);
	if (unlikely(ret)) {
		UDIF_PERR("%s: failed cdev_add(), ret = %d\n", cdev->node->name, ret);
		unregister_chrdev_region(devno, cdev->node->nr_minor);
		return UDIF_ERR_NOMEM;
	}

	return UDIF_ERR_OK;
}

UDIF_ERR udif_cdev_unregister(UDIF_CDEV *cdev)
{
	UDIF_DEVNO devno;

	UDIF_PARM_CHK(!cdev, "invalid cdev", UDIF_ERR_PAR);
	UDIF_PARM_CHK(!cdev->node, "invalid node", UDIF_ERR_PAR);
	UDIF_PARM_CHK(cdev->node->type != UDIF_TYPE_CDEV, "nodetype is not cdev", UDIF_ERR_PAR);

	devno = UDIF_MKDEV(cdev->node->major, cdev->node->first_minor);

	cdev_del(&cdev->cdev);
	unregister_chrdev_region(devno, cdev->node->nr_minor);

	return UDIF_ERR_OK;
}

EXPORT_SYMBOL(udif_cdev_register);
EXPORT_SYMBOL(udif_cdev_unregister);
