/* 2011-01-20: File added and changed by Sony Corporation */
/*
 * Userland Collector module with kinspect
 */
/*
 * Copyright 2009 Sony Corporation
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation; version 2 of the  License.
 *
 * THIS	SOFTWARE  IS PROVIDED ``AS  IS'' AND	  ANY  EXPRESS OR IMPLIED
 * WARRANTIES,	 INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.	IN
 * NO  EVENT  SHALL   THE AUTHOR  BE	LIABLE FOR ANY	 DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED	 TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN	CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/tty_driver.h>
#include <linux/string.h>
#include <linux/sysrq.h>
#include <linux/smp.h>
#include <linux/netpoll.h>
#include <linux/pid.h>
#include <linux/stat.h>
#include <linux/if_ether.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/inet.h>
#include <linux/ctype.h>
#include <linux/utsname.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16)
#include <linux/marker.h>
#endif

#include "snsc_kinspect.h"
/* #define UDC_DEBUG */

#ifdef UDC_DEBUG
#define UDC_DPRN(fmt, args ...) \
	    printk(KERN_DEBUG "%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);
#else /* UDC_DEBUG */
#define UDC_DPRN(fmt, args ...)
#endif /* UDC_DEBUG */

#define UDC_ERR(fmt, args ...) \
	    printk(KERN_ERR "%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);

#define KIPCT_CLASS_NAME "kinspect"
#define DEFAULT_UNIT_BUFFER_SIZE 4096
#define ON_DEMAND_WRITE 1
#define PERIODIC_WRITE 0
#define DOT_TYPE 1
#define NUMERIC_TYPE 2
#define MAX_NAME_LEN 20
#define to_dev(obj) container_of(obj, struct device, kobj)

static int unit_bufsize = DEFAULT_UNIT_BUFFER_SIZE;
static struct class *s_class_kipct = NULL;
/* Each collector private data */
typedef struct clctr_priv {
	int clctr_id;
	int unit_num;
	int size;
	int type;
}clctr_priv_t;

/* Each unit private data */
typedef struct unit_priv {
	char *send_buf;
	char *recv_buf;
	int unit_buf_size;
	int sbuf_dcount;
	int recv_dcount;
	int send_type;
	kipct_write_t winfo;
	unsigned long long  timestamp;
}unit_priv_t;

/* collector tunit device */
typedef struct unit_device {
	struct list_head ulist;
	struct device ucreate;
	int unit_id;
	char unit_name[MAX_NAME_LEN];
	unit_priv_t upriv;
}unit_device_t;

/*class collector device */
typedef struct clctr_device {
	struct list_head clist;
	struct device ccreate;
	clctr_priv_t  priv;
	kipct_clctr_rgst_t clctr_rgst;
	unit_device_t unithead;
}clctr_device_t;

static clctr_device_t clctrhead;

static unit_device_t* get_unit_device(const char*, clctr_device_t* );
static clctr_device_t* get_clctr_device(const char*);

static void usr_device_release(struct device *dev)
{
	UDC_DPRN("usr device is released - %s\n",dev_name(dev));
}

static struct device user_if = {
	.init_name = "usr",
	.release = usr_device_release,
};

static ssize_t
udc_show_id (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;

	n = sprintf (data, "0\n");
	return n;
}

static ssize_t
udc_show_time (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = kipct_get_timestamp();
	nanosec_rem = do_div(t, 1000000000);
	n = sprintf(data, "%5lu.%06lu \n", (unsigned long)t, nanosec_rem/1000);
	return n;
}

static ssize_t
udc_store_stop (struct class *c, struct class_attribute *attr,
	const char *buf, size_t len)
{
	const char *cur = buf;
	char *delim;
	int retVal = 0;

	if (*cur != ' ') {
		if ((delim = strstr (cur, "stop")) != NULL) {
			retVal = kipct_get_rconfigure();
			if (!retVal) {
				printk(KERN_INFO"Configure the host parameter\n");
				return len;
			}
			if (!kipct_survey_get()) {
				printk (KERN_INFO"kinspect is not running\n");
				return len;
			}
			kipct_survey = KIPCT_SURVEY_STOP;
			kipct_set_stop_cmd();
		}
	}
	return len;
}
static ssize_t
udc_show_stop (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;
	unsigned long freq = 0;
	int retVal = 0;

	retVal = kipct_get_rconfigure();
	if (!retVal) {
		n = sprintf (data, "%lu\n", freq);
		return n;
	}
	if (kipct_survey_get()) {
		freq = kipct_get_frequency ();
		n = sprintf (data, "%lu\n", freq);
	}
	else
		n = sprintf (data, "%lu\n", freq);
	return n;
}

static ssize_t
udc_store_start (struct class *c, struct class_attribute *attr,
	const char *buf, size_t len)
{
	const char *cur = buf;
	unsigned long freq = 0;
	int retVal = 0;

	if (*cur != ' ') {
		retVal = kipct_get_rconfigure();
		if (!retVal) {
			printk(KERN_INFO"Configure the host parameter\n");
			return len;
		}
		freq = simple_strtoul(cur, NULL, 10);
		if (freq < MIN_SURVEY_FREQUENCY) {
			printk(KERN_INFO"survey interval greater than or equal to 100 ms\n");
			return len;
		}
		if (kipct_survey_get()) {
			printk(KERN_INFO"kinspect is running,cannot be changed\n");
			return len;
		}
		kipct_set_start_cmd (freq);
    	}
	return len;
}

static ssize_t
udc_show_start (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;
	unsigned long freq = 0;
	int retVal = 0;

	retVal = kipct_get_rconfigure();
        if (!retVal) {
                n = sprintf (data, "%lu\n", freq);
                return n;
        }
	if (kipct_survey_get()) {
		freq = kipct_get_frequency ();
		n = sprintf (data, "%lu\n", freq);
    	}
	else
		n = sprintf (data, "%lu\n", freq);
	return n;
}

static ssize_t
udc_store_list (struct class *c, struct class_attribute *attr,
	const char *buf, size_t len)
{

	const char *cur = buf;

	if (*cur == '1') {
		kipct_set_clctrinfo (0);
	}
	return len;
}

static ssize_t
udc_show_list (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;
	kipct_clctr_t *p = NULL;
	kipct_t udc_kipct = kipct_get_clist ();

	if (list_empty(&udc_kipct.clist)) {
		return 0;
	}
	list_for_each_entry (p, &(udc_kipct.clist), list) {
		if (p->enable != 1)
			return n;
		n += sprintf (data + n, "%d-%s\n",p->clctr_id, p->clctr_name);
	}
	return n;
}

static ssize_t
udc_store_host (struct class *c, struct class_attribute *attr,
	const char *buf, size_t len)
{
	int ret = 0;

	ret = kipct_parse_host_parameter((char*)buf, UDC_HOST_DATA);
	if (ret)
		return 0;
	return len;
}

static ssize_t
udc_show_host (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;
	struct netpoll udc_np = kipct_get_host_data();

	n += sprintf (data + n, "Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
	     udc_np.remote_mac[0], udc_np.remote_mac[1], udc_np.remote_mac[2],
	     udc_np.remote_mac[3], udc_np.remote_mac[4], udc_np.remote_mac[5]);
	n += sprintf (data + n, "Remote IP %pI4\n", &udc_np.remote_ip);
	n += sprintf (data + n, "Remote port %d\n", udc_np.remote_port);
	return n;
}
static ssize_t
udc_show_keepalive (struct class *c, struct class_attribute *attr,
	char *data)
{
	int n = 0;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = kipct_keepalive_recv_time();
	nanosec_rem = do_div(t, 1000000000);
	n = sprintf(data, "%5lu.%06lu \n", (unsigned long)t, nanosec_rem/1000);
	return n;
}

static struct class_attribute s_class_kipct_attrs[] = {
  __ATTR (id, S_IRUSR, udc_show_id, NULL),
  __ATTR (time, S_IRUSR, udc_show_time, NULL),
  __ATTR (stop, S_IRUSR | S_IWUSR, udc_show_stop, udc_store_stop),
  __ATTR (host, S_IRUSR | S_IWUSR, udc_show_host, udc_store_host),
  __ATTR (list, S_IRUSR | S_IWUSR, udc_show_list, udc_store_list),
  __ATTR (start, S_IRUSR | S_IWUSR, udc_show_start, udc_store_start),
  __ATTR (keepalive, S_IRUSR, udc_show_keepalive, NULL),
};

int kipct_get_unit_id(u32 unit_id, u32 clctr_id)
{
	unit_device_t *up = NULL;
	clctr_device_t *cp = NULL;

	list_for_each_entry(cp, &(clctrhead.clist), clist) {
		if (cp->priv.clctr_id == clctr_id)
			break;
	}
	list_for_each_entry(up, &(cp->unithead.ulist), ulist) {
		if (up->unit_id == unit_id)
			return 1;
	}
	return 0;
}

void
kipct_store_recv_data(u32 clctr_id, u32 unit_id, char *data, int dlen)
{
	unit_device_t *up = NULL;
        clctr_device_t *cp = NULL;
	char *recv_buf = NULL;

        list_for_each_entry(cp, &(clctrhead.clist), clist) {
	        if (cp->priv.clctr_id == clctr_id)
			break;
	}
	list_for_each_entry(up, &(cp->unithead.ulist), ulist) {
		if (up->unit_id == unit_id)
			break;
	}
	recv_buf = up->upriv.recv_buf;
	strncpy(&recv_buf[up->upriv.recv_dcount],data, dlen);
	up->upriv.recv_dcount += dlen;
	if(up->upriv.recv_dcount == up->upriv.unit_buf_size) {
		memset(up->upriv.recv_buf, 0, up->upriv.unit_buf_size);
		up->upriv.recv_dcount = 0;
	}
        return;
}

static ssize_t
unit_data_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buffer, loff_t offset, size_t count)
{
	unit_device_t *up = NULL;
	clctr_device_t *clctrp = NULL;
	struct device *dev = to_dev(kobj);
	struct device *clctr = dev->parent;
	unsigned int size;

	clctrp = get_clctr_device(dev_name(clctr));
	up = get_unit_device(dev_name(dev), clctrp);
	size = up->upriv.recv_dcount;

	if (offset > size)
		return 0;
	if (offset + count > size)
		count = size - offset;
	memcpy(buffer, up->upriv.recv_buf + offset, count);
	return count;
}

static ssize_t
unit_data_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buffer, loff_t offset, size_t count)
{
	struct device *dev = to_dev(kobj);
	struct device *clctr = dev->parent;
	clctr_priv_t *priv = NULL;
	unit_device_t *up = NULL;
	char *send_buf = NULL;
	clctr_device_t *clctrp = NULL;

	UDC_DPRN("unit_data_write dev =%d - %s\n", count, buffer);
	priv = dev_get_drvdata(clctr);

	clctrp = get_clctr_device(dev_name(clctr));
	up = get_unit_device(dev_name(dev), clctrp);
	send_buf = up->upriv.send_buf;
	if (send_buf != NULL) {
		if (up->upriv.sbuf_dcount + count > up->upriv.unit_buf_size) {
			printk (KERN_INFO"Sendbuffer for periodic is full\n");
			return count;
		}
		strncpy(&send_buf[up->upriv.sbuf_dcount], buffer, count);
		up->upriv.sbuf_dcount += count;
	}
	return count;
}

static struct bin_attribute s_bin_attr = {
	.attr = {.name = "data", .mode = 0644},
	.size = 0,
	.read = unit_data_read,
	.write = unit_data_write,
};

static ssize_t
unit_show_id (struct device *dev, struct device_attribute *attr, char *buf)
{

	int n = 0;
	struct device *clctr = dev->parent;
	clctr_device_t *clctrp = NULL;
	unit_device_t *up = NULL;

	clctrp = get_clctr_device(dev_name(clctr));
	up = get_unit_device(dev_name(dev), clctrp);
	n = sprintf (buf, "%d\n", up->unit_id);
	return n;
}

static ssize_t
unit_store_send(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	const char *cur = buf;
	unit_device_t *up = NULL;
	unsigned int type = 0;
	unsigned long long timestamp = 0;
	struct device *clctr = dev->parent;
	clctr_device_t *clctrp = NULL;
	clctr_priv_t *priv = NULL;
	char *send_buf = NULL;

	priv = dev_get_drvdata(clctr);
	if (*cur !='.' &&  !isdigit(*cur))
		printk(KERN_INFO"Character Allowed [.(period)| [0-9]+]\n");
	if (*cur == '.') {
		type = DOT_TYPE;
		if (priv->type == ON_DEMAND_WRITE)
			timestamp = kipct_get_timestamp();
		if (priv->type == PERIODIC_WRITE)
			timestamp = 0;
	}
	else if (isdigit(*cur)) {
		type = NUMERIC_TYPE;
		timestamp = simple_strtoull (cur, NULL, 10);
	}
	clctrp = get_clctr_device(dev_name(clctr));
	up = get_unit_device(dev_name(dev), clctrp);
	up->upriv.send_type = type;
	up->upriv.timestamp = timestamp;
	up->upriv.winfo.timestamp = up->upriv.timestamp;
	send_buf = up->upriv.send_buf;

	if (priv->type == ON_DEMAND_WRITE) {
		if (up->upriv.sbuf_dcount != 0) {
			kipct_lformat_write(&up->upriv.winfo, "%s", send_buf);
			memset(send_buf, 0, up->upriv.sbuf_dcount);
			up->upriv.sbuf_dcount = 0;
		}
		up->upriv.winfo.seq_id++;
		up->upriv.send_type = 0;
	}
	return count;
}

static struct device_attribute unit_attrs[] = {
 __ATTR (id, S_IRUGO, unit_show_id, NULL),
 __ATTR (send, S_IWUGO ,NULL, unit_store_send),
};

static int
unit_buf_allocate(unit_priv_t *upriv)
{
	upriv->send_buf = kzalloc(upriv->unit_buf_size, GFP_KERNEL);
	if (upriv->send_buf == NULL) {
		UDC_ERR("kzalloc() failed\n");
		return -ENOMEM;
	}
	upriv->recv_buf = kzalloc(upriv->unit_buf_size, GFP_KERNEL);
	if (upriv->recv_buf == NULL) {
		UDC_ERR("kzalloc() failed\n");
		return -ENOMEM;
	}
	return 0;
}

static void ucreate_release(struct device *dev)
{
	UDC_DPRN("unit is released - %s\n",dev_name(dev));
}

static int udc_validate_device_name(const char *buf)
{
	const char *cur = buf;
	int buflen = 0;

	buflen = strlen(buf) - 1;
	while (buflen) {
		if ((isalnum(*cur) || *cur == '_' )) {
			cur ++;
			buflen--;
		} else
			return -1;
	}
	return 0;
}

static ssize_t
unit_create(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	clctr_priv_t *priv = NULL;
	clctr_device_t *clctrp = NULL;
	unit_device_t *up;
	int  i, retVal = 0;
	int size_uattr = 0, size_dattr = 0;

	UDC_DPRN("count=%d buf=%s\n", count, buf);
	/* Check the string length less than 20 bytes*/
	if (count > MAX_NAME_LEN) {
		UDC_ERR("String length should be less than 20 bytes\n");
		return count;
	}
	retVal = udc_validate_device_name(buf);
	if(retVal) {
		printk(KERN_INFO"Character allowed[A-Z,a-z,0-9,_]\n");
		return count;
	}
	clctrp = get_clctr_device(dev_name(dev));
	/*Search if the unit device is present*/
	up = get_unit_device((char*)buf, clctrp);
	if (up != NULL) {
		UDC_ERR("unit device already present\n");
		return count;
	}
	/* Allocate the memory for structure unit_device_t*/
	up = kzalloc(sizeof(unit_device_t), GFP_KERNEL);
	if (up == NULL) {
		UDC_ERR("no memory\n");
		return count;
        }
	UDC_DPRN("unit=%p\n", &up->ucreate);
	/* Unit device is available*/
	strlcpy((char *)up->ucreate.init_name, buf, strlen(buf));
	up->ucreate.parent = dev;
	up->ucreate.release = ucreate_release;
	/* Get the collector privite data */
	priv = dev_get_drvdata(dev);
	strlcpy(up->unit_name, buf, strlen(buf));
	up->upriv.winfo.clctr_id = priv->clctr_id;
	up->unit_id = ++priv->unit_num;
	up->upriv.winfo.unit_id = up->unit_id;
	up->upriv.unit_buf_size = priv->size;
	/* Allocate 2 send and 1 receive buffer*/
	retVal = unit_buf_allocate(&up->upriv);
	if (retVal) {
		UDC_ERR("unit buffer allocation failed %d", retVal);
		return retVal;
	}
	/* Register the unit device */
	retVal = device_register(&up->ucreate);
	if (retVal) {
		UDC_ERR("device_register failed %d\n", retVal);
		return retVal;
        }
	/* Add the unit device to the list */
	list_add(&(up->ulist), &(clctrp->unithead.ulist));
	size_uattr = sizeof(unit_attrs);
	size_dattr = sizeof(struct device_attribute);
	/* Create the device unit file */
	for (i = 0; i < size_uattr / size_dattr; i++) {
		retVal = device_create_file(&up->ucreate, &unit_attrs[i]);
		if (retVal) {
			UDC_ERR("unit device_create_file %d", retVal);
			return retVal;
		}
	}
	retVal = sysfs_create_bin_file(&up->ucreate.kobj, &s_bin_attr);
	if (retVal){
		UDC_ERR("sysfs_create_bin_file failed %d\n", retVal);
		return retVal;
	}
	return count;
}

static void unit_buf_free(unit_device_t *up)
{
	kfree(up->upriv.send_buf);
	kfree(up->upriv.recv_buf);
}

static ssize_t
unit_delete(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int i;
	unit_device_t *up = NULL;
	unit_device_t *up_next = NULL;
	int size_uattr = 0, size_dattr = 0;
	clctr_device_t *clctrp = NULL;
	clctr_priv_t *priv = NULL;

	clctrp = get_clctr_device(dev_name(dev));
	priv = dev_get_drvdata(dev);
	list_for_each_entry_safe(up, up_next, &(clctrp->unithead.ulist), ulist) {
		if (strncmp(up->ucreate.init_name, buf, strlen(up->ucreate.init_name)) == 0) {
			sysfs_remove_bin_file(&up->ucreate.kobj, &s_bin_attr);
			size_uattr = sizeof(unit_attrs);
			size_dattr = sizeof(struct device_attribute);
			for (i = 0; i < size_uattr / size_dattr; i++) {
				device_remove_file(&up->ucreate, &unit_attrs[i]);
			}
			device_unregister(&up->ucreate);
			priv->unit_num--;
			unit_buf_free(up);
			list_del(&up->ulist);
			kfree(up);
			up = NULL;
                }
        }
	dev_set_drvdata(dev, priv);
	return count;
}

static DEVICE_ATTR(ucreate, 0200, NULL, unit_create);
static DEVICE_ATTR(udelete, 0200, NULL, unit_delete);

/* Show the collector id */
static ssize_t
clctr_show_id (struct device *dev, struct device_attribute *attr, char *buf)
{
	int n = 0;
	clctr_priv_t *priv = NULL;

	priv = dev_get_drvdata(dev);
	n = sprintf (buf, "%d\n", priv->clctr_id);
	return n;
}

static ssize_t
clctr_store_size (struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	clctr_priv_t *priv = NULL;

	priv = dev_get_drvdata(dev);
	if (*buf != '0' || *buf != ' ') {
		priv->size = simple_strtol (buf, NULL, 10);
		dev_set_drvdata(dev, priv);
	}
	return count;

}

static ssize_t
clctr_show_size (struct device *dev, struct device_attribute *attr, char *buf)
{
	int n = 0;
	clctr_priv_t *priv = NULL;

	priv = dev_get_drvdata(dev);
	n = sprintf (buf, "%d\n", priv->size);
	return n;
}

static ssize_t
clctr_store_type (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{

	clctr_priv_t *priv = NULL;

	priv = dev_get_drvdata(dev);

	if (*buf == '1')
		priv->type = ON_DEMAND_WRITE;
	else if (*buf == '0')
		priv->type = PERIODIC_WRITE;
	dev_set_drvdata(dev, priv);
	return count;
}
static clctr_device_t* get_clctr_device(const char *data)
{
	clctr_device_t *cp = NULL;

	list_for_each_entry(cp, &(clctrhead.clist), clist) {
		if (strncmp(cp->ccreate.init_name, data, strlen(cp->ccreate.init_name)) == 0)
			return cp;
	}

	return NULL;
}
static unit_device_t* get_unit_device(const char *data, clctr_device_t *clctrp)
{
	unit_device_t *up = NULL;

	list_for_each_entry(up, &(clctrp->unithead.ulist), ulist) {
		if (strncmp(up->ucreate.init_name, data, strlen(up->ucreate.init_name)) == 0)
			return up;
	}

	return NULL;
}

static struct device_attribute clctr_attrs[] = {
 __ATTR (id, S_IRUGO, clctr_show_id, NULL),
 __ATTR (size, S_IWUGO | S_IRUGO, clctr_show_size, clctr_store_size),
 __ATTR (type, S_IWUGO, NULL, clctr_store_type),
};

static int
clctr_common_info_cb(kipct_cb_arg_t *arg)
{

	clctr_priv_t *priv = NULL;
	struct device *dev = NULL;
	kipct_buf_t *bufinfo = NULL;
	char *buf = NULL;
	unsigned long maxlen = 0;
	unsigned int len = 0;
	unit_device_t *up = NULL;
	clctr_device_t *clctrp = NULL;

        dev = (struct device *)(arg->um_arg);
	clctrp = get_clctr_device(dev_name(dev));
	if (list_empty(&clctrp->unithead.ulist))
		return 0;
	priv = dev_get_drvdata(dev);
	bufinfo = &arg->bufinfo;
	buf = bufinfo->buf;
	maxlen = bufinfo->max_buf_len;
	(void)kipct_sprintf(buf, maxlen, &len, "%s:%d:%s",
				"%I", priv->clctr_id, dev_name(dev));
	list_for_each_entry(up,&(clctrp->unithead.ulist), ulist) {
		(void)kipct_sprintf(buf, maxlen, &len, ":%d:%s",
				up->unit_id,up->unit_name);
        }
	(void)kipct_sprintf(buf, maxlen, &len, "\n");
	(void)kipct_buf_write(buf, len);
	return 0;
}
static int
clctr_unit_common_cb(struct device *dev, void *data)
{

	clctr_priv_t *priv = NULL;
	clctr_device_t *clctrp = NULL;
	unit_device_t *up = NULL;
	char *send_buf = NULL;
	struct device *clctr = dev->parent;

	priv = (clctr_priv_t *)data;
	clctrp = get_clctr_device(dev_name(clctr));
	up = get_unit_device(dev_name(dev), clctrp);
	send_buf = up->upriv.send_buf;

	if (priv->type == ON_DEMAND_WRITE)
		return 0;
	if (up->upriv.send_type == DOT_TYPE || up->upriv.send_type == NUMERIC_TYPE) {
		if (up->upriv.sbuf_dcount != 0 && priv->type == PERIODIC_WRITE) {
			kipct_lformat_write(&up->upriv.winfo, "%s", send_buf);
			memset(send_buf, 0, up->upriv.sbuf_dcount);
			up->upriv.sbuf_dcount = 0;
		}
		up->upriv.winfo.seq_id++;
		up->upriv.send_type = 0;
	}
	return 0;
}

static int
clctr_common_cb(kipct_cb_arg_t * arg) {

	struct device *dev = NULL;
	clctr_priv_t *priv = NULL;
	unit_device_t *up = NULL;
	clctr_device_t *clctrp = NULL;

	dev = (struct device *)(arg->um_arg);
	priv = dev_get_drvdata(dev);
	clctrp = get_clctr_device(dev_name(dev));
	if (list_empty(&clctrp->unithead.ulist))
		return 0;
	list_for_each_entry(up, &(clctrp->unithead.ulist), ulist)
		clctr_unit_common_cb(&up->ucreate, priv);
        return 0;
}

static void ccreate_release(struct device *dev)
{
	UDC_DPRN("collector is released - %s\n", dev_name(dev));
}

static int clctr_device_search (const char *data)
{
	clctr_device_t *newp = NULL;

	list_for_each_entry(newp, &(clctrhead.clist), clist) {
		if (strncmp(newp->ccreate.init_name, data,
				strlen(newp->ccreate.init_name)) == 0)
			return 1;
	}
	return 0;
}

static ssize_t
clctr_create(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	clctr_device_t *p;
	int size_cattr = 0, size_dattr = 0;
	int i, retVal = 0;

	UDC_DPRN("count=%d buf=%s\n", count, buf);
	/*Check the string length less than 20 bytes*/
	if (count > MAX_NAME_LEN) {
		UDC_ERR("String length should be less than 20 bytes\n");
		return count;
	}
	retVal = udc_validate_device_name(buf);
	if(retVal) {
		printk(KERN_INFO"Character allowed[A-Z,a-z,0-9,_]\n");
		return count;
	}
	/*Search if the collector device is present*/
	retVal = clctr_device_search(buf);
	if (retVal) {
		UDC_ERR("device already present\n");
		return count;
	}
	/*Allocate the memory for structure clctr_device_t*/
	p = kzalloc(sizeof(clctr_device_t), GFP_KERNEL);
	if (p == NULL) {
		UDC_ERR("no memory\n");
		return count;
	}
	/* Initialize the device */
	strlcpy((char *)p->ccreate.init_name, buf, strlen(buf));
	p->ccreate.parent = &user_if;
	p->ccreate.release = ccreate_release;
	p->priv.size = unit_bufsize;
	p->priv.type = PERIODIC_WRITE;
	dev_set_drvdata(&(p->ccreate), &p->priv);

	/* Register the device */
	retVal = device_register(&(p->ccreate));
	if (retVal) {
		UDC_ERR("device_register failed %d\n", retVal);
		goto err;
	}

	size_cattr = sizeof(clctr_attrs);
	size_dattr = sizeof(struct device_attribute);
	/* Create the device files */
	for (i = 0; i < size_cattr / size_dattr; i++) {
		retVal = device_create_file(&p->ccreate, &clctr_attrs[i]);
		if (retVal) {
			UDC_ERR("device_create_file %d", retVal);
			goto err1;
		}
	}

	retVal = device_create_file(&p->ccreate, &dev_attr_ucreate);
	if (retVal) {
		UDC_ERR("device_create_file failed %d\n", retVal);
		goto err1;
	}

	retVal = device_create_file(&p->ccreate, &dev_attr_udelete);
	if (retVal) {
		UDC_ERR("device_create_file failed %d\n", retVal);
		goto err2;
	}

	list_add(&(p->clist),&(clctrhead.clist));
	/* Allocate memory for the collector name */
	p->clctr_rgst.clctr_name = kzalloc(strlen(buf), GFP_KERNEL);
	if (p->clctr_rgst.clctr_name == NULL) {
		UDC_ERR("no memory\n");
		goto err3;
	}

	p->clctr_rgst.um_arg = (void*)(&p->ccreate);
	p->clctr_rgst.cb = clctr_common_cb;
	p->clctr_rgst.infocb = clctr_common_info_cb;
	strncpy(p->clctr_rgst.clctr_name, buf, strlen(buf)-1);

	/* Register the collector */
	retVal = kipct_clctr_register(&p->clctr_rgst);
	if (retVal < 0) {
		UDC_ERR("fault: kinspect_clctr_register()\n");
		goto err4;
	}

	p->priv.clctr_id = p->clctr_rgst.clctr_id;
	INIT_LIST_HEAD(&p->unithead.ulist);
	return count;

err4:
	kfree(p->clctr_rgst.clctr_name);
err3:
	device_remove_file(&p->ccreate, &dev_attr_udelete);
err2:
	device_remove_file(&p->ccreate, &dev_attr_ucreate);
err1:
	while (--i >= 0)
		device_remove_file(&p->ccreate, &clctr_attrs[i]);
	device_unregister(&p->ccreate);
err:
	kfree(p);
	return count;
}

static ssize_t
clctr_delete(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int i;
	int size_cattr = 0, size_dattr = 0;
	clctr_device_t *p = NULL;
	clctr_device_t *p_next = NULL;

	list_for_each_entry_safe(p, p_next, &clctrhead.clist, clist) {
		if (strncmp(p->ccreate.init_name, buf, strlen(p->ccreate.init_name)) == 0) {
			(void)kipct_clctr_unregister(&p->clctr_rgst);
			size_cattr = sizeof(clctr_attrs);
			size_dattr = sizeof(struct device_attribute);
			for (i = 0; i < size_cattr/size_dattr; i++) {
				device_remove_file(&p->ccreate, &clctr_attrs[i]);
        		}
			device_remove_file(&p->ccreate, &dev_attr_ucreate);
			device_remove_file(&p->ccreate, &dev_attr_udelete);
        		device_unregister(&p->ccreate);
			list_del(&p->clist);
			kfree(p);
		}
	}
	return count;
}
static DEVICE_ATTR(ccreate, 0200, NULL, clctr_create);
static DEVICE_ATTR(cdelete, 0200, NULL, clctr_delete);

/*
 * insmod/rmmod
 */

static int
init_udcollector (void)
{
	int retVal = 0;
	int i = 0;
	int size_cattr = 0, size_kattr = 0;

	s_class_kipct = kzalloc (sizeof (struct class), GFP_KERNEL);
	if (!s_class_kipct) {
		UDC_ERR ("no memory");
		return -ENOMEM;
	}
	s_class_kipct->name = KIPCT_CLASS_NAME;
	s_class_kipct->owner = THIS_MODULE;
	retVal = class_register (s_class_kipct);
	if (retVal)
		goto err;

	size_cattr = sizeof(struct class_attribute);
	size_kattr = sizeof(s_class_kipct_attrs);
	for (i = 0; i < size_kattr / size_cattr; i++) {
		retVal = class_create_file (s_class_kipct, &s_class_kipct_attrs[i]);
		if (retVal){
			UDC_ERR ("class_create_file %d", retVal);
			goto err1;
		}
   	}
	user_if.class = s_class_kipct;
	dev_set_uevent_suppress(&user_if, 1);
	retVal = device_register(&user_if);
	if (retVal) {
		UDC_ERR("device_register failed %d\n", retVal);
		goto err1;
	}

	retVal = device_create_file(&user_if, &dev_attr_ccreate);
	if (retVal) {
		UDC_ERR("device_create_file failed %d\n", retVal);
		goto err2;
	}

	retVal = device_create_file(&user_if, &dev_attr_cdelete);
	if (retVal) {
		UDC_ERR("device_create_file failed %d\n", retVal);
		goto err3;
	}

	INIT_LIST_HEAD(&clctrhead.clist);
	printk (KERN_INFO "kipct-udc: insmod\n");
	return 0;

err3:
	device_remove_file (&user_if, &dev_attr_ccreate);
err2:
	device_unregister (&user_if);
err1:
	while(--i >= 0)
		class_remove_file (s_class_kipct, &s_class_kipct_attrs[i]);
	class_unregister (s_class_kipct);
err:
	kfree (s_class_kipct);
	s_class_kipct = NULL;
	return retVal;
}

static void
cleanup_udcollector (void)
{
	int count = 0;
	int size_cattr = 0, size_kattr = 0;

	sysfs_remove_bin_file (&(user_if.kobj), &s_bin_attr);
	device_remove_file (&user_if, &dev_attr_cdelete);
	device_remove_file (&user_if, &dev_attr_ccreate);
	device_unregister (&user_if);
	size_cattr = sizeof(struct class_attribute);
	size_kattr = sizeof(s_class_kipct_attrs);
	count = size_kattr / size_cattr;
	while(count-- >= 0)
		class_remove_file (s_class_kipct, &s_class_kipct_attrs[count]);
	class_unregister (s_class_kipct);
	kfree (s_class_kipct);
	printk(KERN_INFO "kipct-udc: rmmod\n");
}

late_initcall (init_udcollector);
module_exit (cleanup_udcollector);
/*
 * module information
 */
MODULE_DESCRIPTION ("Kinspect userland collector module ");
MODULE_AUTHOR ("Sony Corporation");
MODULE_LICENSE ("GPL");
