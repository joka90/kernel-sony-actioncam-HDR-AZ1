/* 2011-01-20: File added and changed by Sony Corporation */
/*
 *  kinspect - kernel inspect tool
 */
/*
 * Copyright 2007-2009 Sony Corporation.
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * THIS	 SOFTWARE  IS PROVIDED	 ``AS  IS'' AND	  ANY  EXPRESS OR IMPLIED
 * WARRANTIES,	 INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.	 IN
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
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/inet.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
#include <linux/marker.h>
#endif
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include<linux/inet.h>
#include "snsc_kinspect.h"

/* #define KI_DEBUG */

#ifdef KI_DEBUG
#define KI_DPRN(fmt, args ...) \
	    printk(KERN_DEBUG "kipct:%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);
#else /* KI_DEBUG */
#define KI_DPRN(fmt, args ...)
#endif /* KI_DEBUG */

#define KI_INFO(fmt, args ...) \
	    printk(KERN_INFO "kipct:%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);

#define KI_WARN(fmt, args ...) \
	    printk(KERN_WARNING "kipct:%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);

#define KI_ERR(fmt, args ...) \
	    printk(KERN_ERR "kipct:%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);

#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
#define MAX_TIME_COUNT (4096)
#define SURVEY		 0
#define OUTPUT		 1
#define MAX_MT		 3
typedef struct mt {
	unsigned long long diff[MAX_TIME_COUNT];
	unsigned int cnt;
	unsigned int max_cnt;
} mt_t ;
typedef struct mtimes {
	mt_t mt[MAX_MT];
	struct semaphore sem;
	struct proc_dir_entry *proc_file;
} mtimes_t;
static mtimes_t measure;

static void cntup(int id)
{
	mt_t *mt = &measure.mt[id];

	if (mt->max_cnt < MAX_TIME_COUNT) {
		mt->max_cnt = mt->cnt + 1;
	}
	mt->cnt++;
	if (mt->cnt >= MAX_TIME_COUNT) {
		mt->cnt = 0;
		mt->max_cnt = MAX_TIME_COUNT;
	}
}

static int
kipct_proc_read_measure(char *page, char **start, off_t off,
			int count, int *eof, void *data);
static int
kipct_proc_write_measure(struct file *file, const char *buf,
			 unsigned long count, void *data);

#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

#define MAX_NETWORK_CHUNK (1280)
#define MAX_KIPCT_BUF \
	CONFIG_SNSC_KINSPECT_MAX_BUF
#define MAX_KIPCT_STBUF \
	CONFIG_SNSC_KINSPECT_MAX_STORED_BUF
#define DEFAULT_SURVEY_INTERVAL \
	CONFIG_SNSC_KINSPECT_SURVEY_INTERVAL
#define OUTPUT_INFO_INTERVAL \
	CONFIG_SNSC_KINSPECT_INFO_INTERVAL
#define DEFAULT_KEEPALIVE_INTERVAL \
	CONFIG_SNSC_KINSPECT_KEEPALIVE_INTERVAL
#define KIPCT_FBUF 256

#define KIPCT_CLCTR_TGT_INFO  0
#define KIPCT_CLCTR_TGT_INFO_NAME "target_info"

#define KI_SEARCH   0
#define KI_REGISTER 1

#define KIPCT_NOT_MARK 0
#define KIPCT_MARK 1

#define MAX_CMD_SIZE 6
#define MAX_DATA_SIZE 1500

static struct netpoll np = {
	.name = "kinspect",
	.dev_name = "eth0",
	.local_port = 6667,
	.remote_port = 6668,
	.remote_mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00},
	.rx_hook = kinspect_rx,
};

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
void
kipct_out_probe(char *buf, unsigned long len);
#else
typedef struct kipct_probe_data {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
	const char *channel;
#endif
	const char *name;
	const char *format;
	marker_probe_func *probe_func;
	char* buf;
	unsigned long len;
} kipct_probe_data_t;

#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,23)
void kipct_out_probe(const struct marker *mdata,
                     void *private,
		     const char *format, ...);
#else
void kipct_out_probe(const struct marker *mdata,
                     void *probe_private, void *call_private,
		     const char *format, va_list *args);
#endif

static kipct_probe_data_t kipct_probe_ary[] =
{
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
		.channel = "kinspect",
#endif
		.name = "kinspect_output",
		.format = MARK_NOARGS,
		.probe_func = kipct_out_probe,
		.buf = NULL,
		.len = 0
	}
};
#endif
static rx_data_t rxd;
static kipct_t s_kipct;
static int configured = 0;
static int kipct_running;
static int exit_kipct = 0;
static u32 kipt_dsize = 0;
static int info_freq_expire;
static unsigned long long keepalive_rtime = 0;
char rx_cmd[MAX_CMD_SIZE];
char rx_buf[MAX_DATA_SIZE];
static DECLARE_WAIT_QUEUE_HEAD(kipct_task_wq);
static unsigned long survey_interval = DEFAULT_SURVEY_INTERVAL;
static unsigned long output_info_interval = OUTPUT_INFO_INTERVAL;
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
static int clctr_data;
#endif
int kipct_survey = KIPCT_SURVEY_START;
static DECLARE_WAIT_QUEUE_HEAD(kipct_survey_wq);


/*
 * prototype
 */

static int option_setup(char *opt);
static int start_kipct_thread(void);
static int kipct_thread(void *unused);
static int kipct_netpoll_setup(void);
static int do_kipct(void);
static kipct_clctr_t * clctr_search(char *name, int create);
static int clctr_disable(kipct_clctr_rgst_t *rgst);
static int is_clctr_disable(void);
static int kipct_output(char *buf, unsigned int len);
static void kipct_read_host_data(void);
static char* parse_set_start_parameter(char*);
static void send_collector_info(unsigned long);
static void stop_collector_info(unsigned long);
static int kipct_stbuf_output(kipct_stbuf_t *);
static DEFINE_TIMER(keepalive_timer, stop_collector_info, 0, 0);
static DEFINE_TIMER(info_timer, send_collector_info, 0, 0);
/*
 * module parameter
 */

/* netpoll configulation */
static char config[256];
module_param_string(netpoll_info, config, 256, 0);
MODULE_PARM_DESC(netpoll_info, " netpoll_info=[src-port]@[src-ip]/[dev],[tgt-port]@<tgt-ip>/[tgt-macaddr]\n");
static int survey = 1;
/* survey setter */
static int survey_set(const char *val, struct kernel_param *kp)
{
	long int stat = 0;
	int retVal = 0;

	if (val) {
		stat = simple_strtoul(val, NULL, 0);
		KI_DPRN("%s:set: val= <%ld>\n", kp->name, stat);
	}
	retVal = param_set_int(val, kp);
	if (retVal == 0) {
		if (stat) {
			kipct_survey = KIPCT_SURVEY_START;
			wake_up(&kipct_survey_wq);
		}
		else {
			kipct_survey = KIPCT_SURVEY_STOP;
		}
	}

	return retVal;
}

/* survey getter */
static int survey_get(char *buffer, struct kernel_param *kp)
{
	KI_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}

module_param_call(survey, survey_set,
		  survey_get, &survey, 0644);
MODULE_INFO(paramtype, "survey:int");
MODULE_PARM_DESC(survey, "start(1), stop(0)\n");





/*
 * internal function
 */

static int option_setup(char *opt)
{
	configured = !netpoll_parse_options(&np, opt);
	return 0;
}
__setup("netpoll_info=", option_setup);


/* output Information */
static int
kipct_info(void)
{
	char *buf = s_kipct.buf;
	unsigned long maxlen = MAX_KIPCT_BUF;
	unsigned int len = 0;
	int retVal = 0;
	kipct_write_t *info = NULL;
	kipct_buf_t bufinfo;
	unsigned int header_len = 0;
	unsigned int size = 0;

	bufinfo.buf = buf;
	bufinfo.max_buf_len = maxlen;
	info = &s_kipct.winfo;

	KI_DPRN("uts_name=%s\n", utsname()->nodename);

	/* kinspect info clctr */
	retVal = kipct_sprintf(buf, maxlen, &len, "%s:0:kipct:%d:%s\n",
			       "%I", KIPCT_CLCTR_TGT_INFO, KIPCT_CLCTR_TGT_INFO_NAME);
	if (retVal < 0) {
		return retVal;
	}

	(void)kipct_buf_write(buf, len);

	len = 0;
	retVal = kipct_write_header(&bufinfo, info, &len);
	if (retVal < 0) { return retVal; }
	header_len = len;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
	down_read(&uts_sem);
	retVal = kipct_sprintf(buf, maxlen, &len, "hostname=%s\n",
			       system_utsname.nodename);
	up_read(&uts_sem);

#else
	retVal = kipct_sprintf(buf, maxlen, &len, "hostname=%s\n",
			       utsname()->nodename);
#endif
	if (retVal < 0) { return retVal; }
	retVal = kipct_sprintf(buf, maxlen, &len, "%\n");
	if (retVal < 0) { return retVal; }
	retVal = kipct_read_file(buf, &len, "/proc/cpuinfo",
				 maxlen);
	if (retVal < 0) { return retVal; }
	size = len - header_len;
	retVal = kipct_write_tailer(&bufinfo, info, &len, size);
	if (retVal < 0) { return retVal; }

	(void)kipct_buf_write(buf, len);
	(info->seq_id)++;

	return 0;
}

/* Initializes message buffer and kernel thread. */
static int
start_kipct_thread(void)
{
	KI_DPRN("\n");
	s_kipct.buf = kzalloc(MAX_KIPCT_BUF, GFP_KERNEL);
	if (s_kipct.buf == NULL) {
		KI_ERR("kzalloc() failed\n");
		return -ENOMEM;
	}

	KI_DPRN("\n");
	kernel_thread(kipct_thread, NULL, CLONE_FS | CLONE_FILES);
	return 0;
}

/*
 * Main loop for k
 */
static int
kipct_thread(void *unused)
{
	int retVal = 0;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	DECLARE_WAITQUEUE(wait, current);

	KI_DPRN("\n");

	daemonize("kinspect");
	sched_setscheduler(current, SCHED_RR, &param);

	kipct_running = 1;

	add_wait_queue(&kipct_task_wq, &wait);

	for (;;) {
		KI_DPRN("\n");
		if (exit_kipct)
			break;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(survey_interval));
		if (rxd.status_bit)
			kipct_read_host_data();
		if (do_kipct())
			break;
	}
	remove_wait_queue(&kipct_task_wq, &wait);

	KI_DPRN("\n");

	netpoll_cleanup(&np);

	kipct_running = 0;

	return retVal;
}


static int
do_kipct(void)
{
	kipct_clctr_t *p = NULL;
	kipct_cb_arg_t arg;

	int retVal = 0;

#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	unsigned long long start = 0;
	unsigned long long end = 0;
	mt_t *mt;
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

	KI_DPRN("\n");

	arg.bufinfo.buf = s_kipct.buf;
	arg.bufinfo.max_buf_len = MAX_KIPCT_BUF;

	retVal = wait_event_interruptible(kipct_survey_wq,
					  kipct_survey == KIPCT_SURVEY_START);
	if (retVal < 0) {
		return retVal;
	}
	if (down_interruptible(&s_kipct.sem)) {
		return -ERESTARTSYS;
	}

	if (s_kipct.outinfo.network_enable == 0) {
		(void)kipct_netpoll_setup();
	}

	if (configured && (s_kipct.outinfo.stbuf.store != NULL)) {
		KI_DPRN("stbuf output\n");
		kipct_stbuf_output(&s_kipct.outinfo.stbuf);
	}

	if (is_clctr_disable()) {
		kipct_survey = KIPCT_SURVEY_STOP;
		goto err;
	}
#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	start = sched_clock();
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */
	if (info_freq_expire) {
		info_freq_expire = 0;
		(void)kipct_info();
		list_for_each_entry(p, &(s_kipct.clist), list) {
			KI_DPRN("\n");
			if (p->infocb && p->enable == 1) {
				arg.um_arg = p->um_arg;
				arg.priv = p->priv;
				p->infocb(&arg);
			}
		}
	}
	list_for_each_entry(p, &(s_kipct.clist), list) {
		if (p->cb && p->enable == 1) {
			arg.um_arg = p->um_arg;
			arg.priv = p->priv;
			p->cb(&arg);
		}
	}
#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	end = sched_clock();
	if (down_interruptible(&measure.sem)) {
		return -ERESTARTSYS;
	}
	mt = &measure.mt[SURVEY];
	mt->diff[mt->cnt] = end - start;
	cntup(SURVEY);
	up(&measure.sem);
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */
 err:
	up(&s_kipct.sem);

	return 0;
}

static void
kipct_netpoll_send(char *buf, unsigned int len)
{
	int frag, left;
	unsigned long flags = 0;

	spin_lock_irqsave(&rxd.rx_lock, flags);
	for(left = len; left; ) {
		frag = min(left, MAX_NETWORK_CHUNK);
		netpoll_send_udp(&np, buf, frag);
		buf += frag;
		left -= frag;
	}
	spin_unlock_irqrestore(&rxd.rx_lock, flags);
}

static void
output_netpoll(char *buf, unsigned int len, int mark_flag)
{
	unsigned long flags = 0;
#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	unsigned long long start = 0;
	unsigned long long end = 0;
	mt_t *mt;
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

	if (!np.dev)
		return;

	KI_DPRN("buf=%p len=%u\n", buf, len);

#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	start = sched_clock();
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

	if (in_interrupt())
		spin_lock_irqsave(&s_kipct.net_out_lock, flags);
	else
		spin_lock(&s_kipct.net_out_lock);

	if (mark_flag == KIPCT_MARK) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
		kipct_out_probe(buf, len);
#else
		kipct_probe_ary[0].buf = buf;
		kipct_probe_ary[0].len = len;
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,23)
		trace_mark(kinspect_output, MARK_NOARGS);
#else
		trace_mark(kinspect, kinspect_output, MARK_NOARGS);
#endif
#endif
	}
	else {
		kipct_netpoll_send(buf, len);
	}

	if (in_interrupt())
		spin_unlock_irqrestore(&s_kipct.net_out_lock, flags);
	else
		spin_unlock(&s_kipct.net_out_lock);

#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	end = sched_clock();
	mt = &measure.mt[OUTPUT];
	mt->diff[mt->cnt] = end - start;
	cntup(OUTPUT);
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

}

static void
output_stbuf(char *buf, unsigned int len)
{
	unsigned long flags = 0;
	kipct_stbuf_t *stbuf = &s_kipct.outinfo.stbuf;

	KI_DPRN("buf=%p len=%u\n", buf, len);

	if (in_interrupt())
		spin_lock_irqsave(&stbuf->buf_lock, flags);
	else
		spin_lock(&stbuf->buf_lock);

	if (stbuf->offset + len < stbuf->total) {
		memcpy(stbuf->store + stbuf->offset, buf, len);
		stbuf->offset += len;
	}
	else {
		KI_INFO("stored buffer is full\n");
		if (!configured) {
			kipct_survey = KIPCT_SURVEY_STOP;
			KI_INFO("logging will be stop\n");
		}
	}
	stbuf->wtotal += len;

	if (in_interrupt())
		spin_unlock_irqrestore(&stbuf->buf_lock, flags);
	else
		spin_unlock(&stbuf->buf_lock);

}

/*
 * initialize stored buffer
 */
static void
init_stbuf(kipct_stbuf_t *stbuf)
{
	spin_lock_init(&stbuf->buf_lock);
	stbuf->total = MAX_KIPCT_STBUF;
}

static int
init_page_buf(void)
{
	spin_lock_init(&s_kipct.page_buf_lock);
	s_kipct.page_buf = (char*)__get_free_page(GFP_KERNEL | __GFP_ZERO);
	if (s_kipct.page_buf == NULL) {
		KI_ERR("no memory\n");
		return -ENOMEM;
	}
	return 0;
}

static int
kipct_stbuf_output(kipct_stbuf_t *stbuf)
{
	KI_DPRN("\n");

	output_netpoll(stbuf->store, min(stbuf->wtotal, stbuf->total),
		       KIPCT_NOT_MARK);
	if (stbuf->store) {
		kfree(stbuf->store);
		stbuf->store = NULL;
	}
	return 0;
}

/*
 * netpoll setup
 */
static int
kipct_netpoll_setup(void)
{
	int retVal = 0;

	retVal = netpoll_setup(&np);
	if( retVal == 0) {
		s_kipct.outinfo.network_enable = 1;
		memcpy(&rxd.np, &np, sizeof(struct netpoll));
	}
	return retVal;
}


static void
unregister_all(void)
{
	kipct_clctr_t *p = NULL;
	kipct_clctr_t *p_next = NULL;

	if (down_interruptible(&s_kipct.sem)) {
		return;
	}

	if (list_empty(&s_kipct.clist)) {
		up(&s_kipct.sem);
		return;
	}
	list_for_each_entry_safe(p, p_next, &(s_kipct.clist), list) {
		list_del(&p->list);
		kfree(p);
		p = NULL;
	}
	s_kipct.clctr_num = 0;
	up(&s_kipct.sem);
}

/* search clctr and create new clctr */
static kipct_clctr_t *
clctr_search(char *name, int create)
{
	kipct_clctr_t *p = NULL;

	list_for_each_entry(p, &(s_kipct.clist), list) {
		if (strncmp(p->clctr_name, name, strlen(name)) == 0) {
			return p;
		}
	}

	if (create) {
		p = kzalloc(sizeof(kipct_clctr_t), GFP_KERNEL);
		if (p == NULL) {
			KI_ERR("no memory\n");
			return NULL;
		}
		p->clctr_id = s_kipct.clctr_num;
		p->clctr_name = name;
		s_kipct.clctr_num++;
		list_add_tail(&(p->list), &(s_kipct.clist));
		return p;
	}

	return NULL;
}


/* disable clctr */
static int
clctr_disable(kipct_clctr_rgst_t *rgst)
{
	kipct_clctr_t *p = NULL;
	kipct_clctr_t *p_next = NULL;

	if (list_empty(&s_kipct.clist)) {
		return 0;
	}
	list_for_each_entry_safe(p, p_next, &(s_kipct.clist), list) {
		p = clctr_search(rgst->clctr_name, KI_SEARCH);
		if (p == NULL)
			return -EINVAL;
		p->enable = 0;
		list_del(&p->list);
		kfree(p);
		p = NULL;
	}

	return 0;
}

/* check clctr is all disabled */
static int
is_clctr_disable(void)
{
	kipct_clctr_t *p = NULL;

	if (list_empty(&s_kipct.clist)) {
		return 1;
	}

	list_for_each_entry(p, &(s_kipct.clist), list) {
		if (p->enable == 1) {
			return 0;
		}
	}

	return 1;
}

static void
kipct_calc_max_buf_len(void)
{
	char str[100]="";

	snprintf(str, 100, "%u", MAX_KIPCT_BUF);

	s_kipct.max_buf_str_len = strlen(str);

	KI_DPRN("max_buf_str_len=%u\n", s_kipct.max_buf_str_len);
}

static int
kipct_output(char *buf, unsigned int len)
{
	if (configured && (s_kipct.outinfo.network_enable == 1)) {
		KI_DPRN("net\n");
		output_netpoll(buf, len, KIPCT_MARK);
	}
	else {
		KI_DPRN("memory\n");
		if (s_kipct.outinfo.stbuf.store == NULL) {
			s_kipct.outinfo.stbuf.store =
				kzalloc(s_kipct.outinfo.stbuf.total,
					GFP_ATOMIC);
			if (s_kipct.outinfo.stbuf.store == NULL) {
				KI_ERR("no memory available\n")
				return -ENOMEM;
			}
		}
		output_stbuf(buf, len);
	}

	return 0;
}



/*
 * proc filesystem
 */
static void *kipct_store_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= 1) {
		return NULL;
	}
	return &s_kipct;
}

static void *kipct_store_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void kipct_store_seq_stop(struct seq_file *s, void *v)
{
	; /* nothing to do */
}

static int kipct_store_seq_show(struct seq_file *s, void *v)
{
	kipct_t *ki = (kipct_t *)v;
	kipct_stbuf_t *stbuf = NULL;
	unsigned long flags = 0;

	stbuf = &(ki->outinfo.stbuf);

	spin_lock_irqsave(&stbuf->buf_lock, flags);
	if (stbuf->store != NULL) {
		seq_printf(s, "%s\n",stbuf->store);
	}
	spin_unlock_irqrestore(&stbuf->buf_lock, flags);

	return 0;
}

static struct seq_operations kipct_store_seq_ops = {
	.start = kipct_store_seq_start,
	.next  = kipct_store_seq_next,
	.stop  = kipct_store_seq_stop,
	.show  = kipct_store_seq_show
};

static int kipct_store_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &kipct_store_seq_ops);
}

static struct file_operations kipct_store_proc_ops = {
	.owner	 = THIS_MODULE,
	.open	 = kipct_store_proc_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = seq_release
};

static int
kipct_create_proc(void)
{
	struct proc_dir_entry *root_dir = NULL;
	struct proc_dir_entry *store_entry = NULL;
#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	struct proc_dir_entry *measure_entry = NULL;
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

	/* make "/proc/kipct" dir */
	root_dir = proc_mkdir("kinspect", 0);
	if (root_dir == NULL) {
		KI_ERR("fault proc_mkdir no memory\n");
		return -ENOMEM;
	}
	s_kipct.proc_file = root_dir;


#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	/* read: measure */
	measure_entry = create_proc_read_entry("measure", 0, root_dir,
					       kipct_proc_read_measure,
					       &measure);
	if (measure_entry != NULL) {
		measure_entry->write_proc = kipct_proc_write_measure;
	}
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

	/* read: stored buffer */
	store_entry = create_proc_entry("storedbuf", 0, root_dir);
	if (store_entry != NULL) {
		store_entry->proc_fops = &kipct_store_proc_ops;
	}

	return 0;
}

static int
kipct_remove_proc(void)
{
#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	remove_proc_entry("measure", s_kipct.proc_file);
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */
	remove_proc_entry("storedbuf", s_kipct.proc_file);

	remove_proc_entry("kinspect", 0);

	return 0;
}

#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
static int
kipct_proc_read_measure(char *page, char **start, off_t off,
			int count, int *eof, void *data)
{
	mtimes_t *m;
	unsigned int len = 0;
	unsigned int invalid_num = 1;
	int mc = 0;
	mt_t *mt;
	unsigned long long avg_diff[MAX_MT];

	int i = 0;

	for (mc = 0; mc < MAX_MT; mc++) {
		avg_diff[mc] = 0;
	}

	m = (mtimes_t *)data;

	if (down_interruptible(&m->sem)) {
		return -ERESTARTSYS;
	}


	for (mc = 0; mc < MAX_MT; mc++) {
		mt = &(m->mt[mc]);
		if (mt->max_cnt == 1) {
			up(&m->sem);
			return 0;
		}
		for (i = 1; i < mt->max_cnt; i++) {
			avg_diff[mc] += mt->diff[i];
		}
		do_div(avg_diff[mc], mt->max_cnt - invalid_num);
	}

	len += sprintf(page + len, "====================================="
		       "==================\n");
	len += sprintf(page + len, "%4s %16s \n", "cnt", "survey");
	len += sprintf(page + len, "-------------------------------------"
		       "------------------\n");
	len += sprintf(page + len, "%4u %16llu \n",
		       m->mt[SURVEY].max_cnt, avg_diff[SURVEY]);
	len += sprintf(page + len, "====================================="
		       "==================\n");
	len += sprintf(page + len, "%4s %16s \n", "cnt", "output");
	len += sprintf(page + len, "-------------------------------------"
		       "------------------\n");
	len += sprintf(page + len, "%4u %16llu \n",
		       m->mt[OUTPUT].max_cnt, avg_diff[OUTPUT]);


	up(&m->sem);

	return len;
}
static int
kipct_proc_write_measure(struct file *file, const char *buf,
			 unsigned long count, void *data)
{
	mtimes_t *m;
	int i = 0;

	m = (mtimes_t *)data;

	if (down_interruptible(&m->sem)) {
		return -ERESTARTSYS;
	}

	if (count == 0) {
		return -EINVAL;
	}

	for (i = 0; i < MAX_MT; i++) {
		memset(&(m->mt[i]), 0, sizeof(mt_t));
	}
	up(&m->sem);

	return count;
}

#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */

/*
 * external function
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
void
kipct_out_probe(char *buf, unsigned long len)
{
	kipct_netpoll_send(buf, len);
}
#endif
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,23)
void
kipct_out_probe(const struct marker *mdata,
                void *private,
		const char *format, ...)
{
	kipct_probe_data_t *pdata = (kipct_probe_data_t *)mdata->private;

	KI_DPRN("pdata(%p) buf=%p len=%lX\n", pdata, pdata->buf, pdata->len);

	kipct_netpoll_send(pdata->buf, pdata->len);
}
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
void
kipct_out_probe(const struct marker *mdata,
                void *probe_private, void *call_private,
		const char *format, va_list *args)
{
	kipct_probe_data_t *pdata = (kipct_probe_data_t *)mdata->single.probe_private;

	KI_DPRN("pdata(%p) buf=%p len=%lX\n", pdata, pdata->buf, pdata->len);

	kipct_netpoll_send(pdata->buf, pdata->len);
}
#endif

int
kipct_clctr_register(kipct_clctr_rgst_t *rgst)
{
	kipct_clctr_t *p = NULL;
	int retVal = 0;

	if (down_interruptible(&s_kipct.sem)) {
		return -ERESTARTSYS;
	}

	p = clctr_search(rgst->clctr_name, KI_REGISTER);
	if (p == NULL) {
		retVal = -ENOMEM;
		goto err;
	}
	p->enable = 1;
	rgst->clctr_id = p->clctr_id;
	p->cb = rgst->cb;
	p->infocb = rgst->infocb;
	p->um_arg = rgst->um_arg;
	p->priv = rgst->priv;
	kipct_survey = KIPCT_SURVEY_START;
	wake_up(&kipct_survey_wq);
	send_collector_info(0);
 err:
	up(&s_kipct.sem);

	return retVal;
}
int
kipct_clctr_unregister(kipct_clctr_rgst_t *rgst)
{
	int retVal = 0;

	if (down_interruptible(&s_kipct.sem)) {
		return -ERESTARTSYS;
	}

	retVal = clctr_disable(rgst);

	up(&s_kipct.sem);

	return retVal;
}

int
kipct_format_write(kipct_write_t *info, const char *format, ...)
{
	char buf[KIPCT_FBUF]="";
	kipct_buf_t bufinfo;
	int retVal = 0;
	unsigned int len = 0;
	unsigned int size = 0;
	unsigned int header_len = 0;
	va_list args;

	bufinfo.buf = buf;
	bufinfo.max_buf_len = KIPCT_FBUF;

	if(!kipct_survey_get())
		return 0;
	/* write header */
	retVal = kipct_write_header(&bufinfo, info, &len);
	if (retVal < 0) {
		KI_ERR("fault: write_header\n");
		return retVal;
	}
	header_len = len;

	/* write body */
	if (len > bufinfo.max_buf_len)
		return -EINVAL;

	va_start(args, format);
	len += vsnprintf(bufinfo.buf + len,
			 bufinfo.max_buf_len - len, format, args);
	va_end(args);

	/* write tailer */
	size = len - header_len;
	retVal = kipct_write_tailer(&bufinfo, info, &len, size);
	if (retVal < 0) {
		return retVal;
	}

	/* output */
	retVal = kipct_output(bufinfo.buf, len);

	return retVal;
}


int
kipct_lformat_write(kipct_write_t *info, const char *format, ...)
{
	kipct_buf_t bufinfo;
	int retVal = 0;
	unsigned int len = 0;
	va_list args;
	unsigned long flags = 0;
	unsigned int size = 0;
	unsigned int header_len = 0;

	bufinfo.buf = s_kipct.page_buf;
	bufinfo.max_buf_len = PAGE_SIZE;

	if(!kipct_survey_get())
		return 0;
	va_start(args, format);
	if (in_interrupt())
		spin_lock(&s_kipct.page_buf_lock);
	else
		spin_lock_irqsave(&s_kipct.page_buf_lock, flags);


	/* write header */
	retVal = kipct_write_header(&bufinfo, info, &len);
	if (retVal < 0) {
		KI_ERR("fault: write_header\n");
		return retVal;
	}
	header_len = len;

	/* write body */
	if (len > bufinfo.max_buf_len)
		return -EINVAL;

	va_start(args, format);
	len += vsnprintf(bufinfo.buf + len,
			 bufinfo.max_buf_len - len, format, args);
	va_end(args);

	/* write tailer */
	size = len - header_len;
	retVal = kipct_write_tailer(&bufinfo, info, &len, size);
	if (retVal < 0) {
		return retVal;
	}

	/* output */
	retVal = kipct_output(bufinfo.buf, len);

	if (in_interrupt())
		spin_unlock(&s_kipct.page_buf_lock);
	else
		spin_unlock_irqrestore(&s_kipct.page_buf_lock, flags);
	va_end(args);

	return retVal;
}

int
kipct_file_write(kipct_write_t *info, const char* file)
{
	kipct_buf_t bufinfo;
	int retVal = 0;
	unsigned int size = 0;
	unsigned int len = 0;
	unsigned int header_len = 0;

	bufinfo.buf = s_kipct.buf;
	bufinfo.max_buf_len = MAX_KIPCT_BUF;

	if(!kipct_survey_get())
		return 0;
	/* write header */
	retVal = kipct_write_header(&bufinfo, info, &len);
	if (retVal < 0) {
		return retVal;
	}
	header_len = len;

	/* write file */
	retVal = kipct_read_file(bufinfo.buf, &len, file,
				 bufinfo.max_buf_len);
	if (retVal < 0) {
		return retVal;
	}

	/* write tailer */
	size = len - header_len;
	retVal = kipct_write_tailer(&bufinfo, info, &len, size);
	if (retVal < 0) {
		return retVal;
	}

	/* output */
	retVal = kipct_output(bufinfo.buf, len);

	return retVal;
}

int
kipct_buf_write(char *buf, unsigned long len)
{
	if(!kipct_survey_get())
		return 0;
	return kipct_output(buf, len);
}


loff_t
kipct_get_file_size(const char * file)
{
	int retVal = 0;
	struct file *filp = NULL;
	loff_t size = 0;
	struct inode *inode = NULL;

	filp = filp_open(file, O_RDONLY, 0);
	if (filp == NULL || IS_ERR((void*)filp)) {
		KI_DPRN("fault: open %s\n", file);
		return -ENOENT;
	}

	inode = filp->f_dentry->d_inode;
	size = i_size_read(inode->i_mapping->host);

	retVal = filp_close(filp, current->files);
	if (retVal < 0) {
		return retVal;
	}

	return size;
}

int
kipct_sprintf(char *buf, unsigned int maxlen,
	      unsigned int *lenp, const char *format, ...)
{
	unsigned int len = *lenp;
	va_list args;

	if (len > maxlen) {
                KI_ERR("too many output. len(0x%08X) > maxlen(0x%08X), "
                       "kinspect thread buffer size is 0x%08X\n",
                       len, maxlen, MAX_KIPCT_BUF);
		return -EINVAL;
        }

	va_start(args, format);
	len += vsnprintf(buf + len, maxlen - len, format, args);
	va_end(args);

	*lenp = len;

	return 0;
}

int
kipct_write_header(kipct_buf_t *bufinfo, kipct_write_t *info,
		   unsigned int *lenp)
{
	if(info->timestamp)
		return kipct_sprintf(bufinfo->buf, bufinfo->max_buf_len,
                             lenp,"%s%d:%d\n%llu,%u,%0*u\n",
                             "%S", info->clctr_id, info->unit_id,info->timestamp,
                             info->seq_id, s_kipct.max_buf_str_len, 0);
	else
		return kipct_sprintf(bufinfo->buf, bufinfo->max_buf_len,
			     lenp,"%s%d:%d\n%llu,%u,%0*u\n",
			     "%S", info->clctr_id, info->unit_id, sched_clock(),
			     info->seq_id, s_kipct.max_buf_str_len, 0);
}

int
kipct_write_tailer(kipct_buf_t *bufinfo, kipct_write_t *info,
		   unsigned int *lenp, unsigned int body_length)
{
	int retVal = 0;
	int header_body_pos = 0;

	header_body_pos =
		*lenp - body_length - (s_kipct.max_buf_str_len + 1);

	if (header_body_pos <= 0) {
		KI_ERR("fault: length is invalid")
		return -EINVAL;
	}
	KI_DPRN("header_body_pos=%d\n", header_body_pos);

	retVal = kipct_sprintf(bufinfo->buf, bufinfo->max_buf_len,
			       &header_body_pos,
			       "%0*u", s_kipct.max_buf_str_len, body_length);
	if (retVal < 0) {
		return retVal;
	}
	if (header_body_pos > bufinfo->max_buf_len) {
		return -EINVAL;
	}
	*(bufinfo->buf + header_body_pos) = '\n';

	return kipct_sprintf(bufinfo->buf, bufinfo->max_buf_len, lenp,
			     "%s%d:%d\n",
			     "%E", info->clctr_id, info->unit_id);
}

int
kipct_read_file(char *buf, int *lenp, const char* file,
		unsigned int max_buf_len)
{
	struct file *filp = NULL;
	int len = *lenp;
	int sz, offset;
	int retVal = 0;

	filp = filp_open(file, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		KI_DPRN("filp_open(%s) failed, it will be retried.\n", file);
		retVal = -EINVAL;
		goto err1;
	}

	offset = 0;
	do {
		if (len > max_buf_len) {
			KI_ERR("buffer full len=%d max=%u\n",
			       len, max_buf_len);
			retVal = -EINVAL;
			goto err2;
		}
		sz = kernel_read(filp, offset, buf + len, max_buf_len - len);
		if (sz < 0) {
			KI_ERR("read(%s) failed\n", file);
			retVal = -EINVAL;
			goto err2;
		}
		offset += sz;
		len += sz;
	} while (sz != 0);

err2:
	filp_close(filp, current->files);
	*lenp = len;
err1:
	return retVal;
}

int
kipct_survey_start(void)
{
	kipct_survey = KIPCT_SURVEY_START;
	return 0;
}

int
kipct_survey_stop(void)
{
	kipct_survey = KIPCT_SURVEY_STOP;
	return 0;
}

/*
 * insmod/rmmod
 */

static int init_kipct(void)
{
	int retVal = 0;
	int id = 0;

#ifdef CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE
	memset(&measure, 0, sizeof(mtimes_t));
	sema_init(&measure.sem, 1);
#endif /* CONFIG_SNSC_KINSPECT_DEBUG_MEASURE_MODE */
	memset(&s_kipct, 0, sizeof(kipct_t));
	sema_init(&s_kipct.sem, 1);
	INIT_LIST_HEAD(&s_kipct.clist);
	s_kipct.clctr_num = 1;
	s_kipct.winfo.clctr_id = 0;
	s_kipct.winfo.unit_id = KIPCT_CLCTR_TGT_INFO;
	s_kipct.winfo.seq_id = 0;
	retVal = init_page_buf();
	if (retVal < 0) {
		return retVal;
	}
	init_stbuf(&s_kipct.outinfo.stbuf);
	kipct_calc_max_buf_len();
	mod_timer(&info_timer,(jiffies + msecs_to_jiffies(OUTPUT_INFO_INTERVAL)));
	mod_timer(&keepalive_timer,(jiffies + msecs_to_jiffies(DEFAULT_KEEPALIVE_INTERVAL)));
	spin_lock_init(&rxd.rx_lock);
	spin_lock_init(&s_kipct.net_out_lock);
	for (id = 0; id < ARRAY_SIZE(kipct_probe_ary); id++) {
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,23)
		retVal = marker_probe_register(kipct_probe_ary[id].name,
					       kipct_probe_ary[id].format,
					       kipct_probe_ary[id].probe_func,
					       &kipct_probe_ary[id]);
		if (retVal) {
			KI_ERR("fault: marker_probe_register ret=%d\n",
			       retVal);
			return retVal;
		}
                retVal = marker_arm(kipct_probe_ary[id].name);
		if (retVal) {
			KI_ERR("fault: marker_arm ret=%d\n",
				retVal);
			return retVal;
		}
#else
		retVal = marker_probe_register(kipct_probe_ary[id].channel,
					       kipct_probe_ary[id].name,
					       kipct_probe_ary[id].format,
					       kipct_probe_ary[id].probe_func,
					       &kipct_probe_ary[id]);
		if (retVal) {
			KI_ERR("fault: marker_probe_register ret=%d\n",
			       retVal);
			return retVal;
		}
#endif
	}

	if(strlen(config))
		option_setup(config);

	if(!configured) {
		KI_INFO("netpoll_info is not configured\n");
	}

	if (start_kipct_thread())
		return -EINVAL;

	retVal = kipct_create_proc();
	if (retVal < 0) {
		KI_ERR("fault create_proc()\n");
		return retVal;
	}

	printk(KERN_INFO "kinspect: insmod\n");
	return 0;
}

static void cleanup_kipct(void)
{
	int id = 0;

	exit_kipct = 1;
	while (kipct_running)
		schedule();
	unregister_all();
	kipct_remove_proc();

	for (id = 0; id < ARRAY_SIZE(kipct_probe_ary); id++) {
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,23)
		marker_probe_unregister(kipct_probe_ary[id].name);
#else
		marker_probe_unregister(kipct_probe_ary[id].channel,
					       kipct_probe_ary[id].name,
					       kipct_probe_ary[id].probe_func,
					       &kipct_probe_ary[id]);
#endif
	}

	if (s_kipct.buf) {
		kfree(s_kipct.buf);
		s_kipct.buf = NULL;
	}
	if (s_kipct.page_buf) {
		free_page((unsigned long)s_kipct.page_buf);
		s_kipct.page_buf = NULL;
	}

	printk(KERN_INFO "kinspect: rmmod\n");
}

static void send_collector_info(unsigned long info_freq_ms)
{
	info_freq_expire = 1;
	if (!info_freq_ms)
		info_freq_ms = OUTPUT_INFO_INTERVAL;
	mod_timer(&info_timer,(jiffies + msecs_to_jiffies(info_freq_ms)));
}

static void stop_collector_info(unsigned long keepalive_freq_ms)
{
	if (!kipct_survey_get()) {
		printk(KERN_INFO"kinspect is not running\n");
		return;
	}
	kipct_survey = KIPCT_SURVEY_STOP;
	del_timer_sync(&info_timer);
	return;
}

static void kipct_read_host_data()
{
	unsigned long flags = 0;

	spin_lock_irqsave(&rxd.rx_lock, flags);
	memcpy (&np, &rxd, sizeof(struct netpoll));
	rxd.status_bit = 0;
	spin_unlock_irqrestore(&rxd.rx_lock, flags);
}

int kipct_parse_host_parameter(char *cur, int type)
{
	char *delim;
	__be32 remote_ip;
	u16 remote_port;
	u8 remote_mac[ETH_ALEN];
	unsigned long flags = 0;

	if ((delim = strchr (cur, ' ')) == NULL)
		goto parse_failed;
	*delim = 0;
	remote_ip = in_aton (cur);
	cur = delim + 1;
	KI_DPRN("Host IP %pI4\n", &remote_ip);
	/* MAC address */
	if ((delim = strchr (cur, ':')) == NULL)
		goto parse_failed;
	*delim = 0;
 	remote_mac[0] = simple_strtol (cur, NULL, 16);
	cur = delim + 1;
	if ((delim = strchr (cur, ':')) == NULL)
		goto parse_failed;
	*delim = 0;
	remote_mac[1] = simple_strtol (cur, NULL, 16);
	cur = delim + 1;
	if ((delim = strchr (cur, ':')) == NULL)
		goto parse_failed;
	*delim = 0;
	remote_mac[2] = simple_strtol (cur, NULL, 16);
	cur = delim + 1;
	if ((delim = strchr (cur, ':')) == NULL)
		goto parse_failed;
	*delim = 0;
	remote_mac[3] = simple_strtol (cur, NULL, 16);
	cur = delim + 1;
	if ((delim = strchr (cur, ':')) == NULL)
		goto parse_failed;
	*delim = 0;
	remote_mac[4] = simple_strtol (cur, NULL, 16);
	cur = delim + 1;
	remote_mac[5] = simple_strtol (cur, NULL, 16);
	cur = delim + 2;
	cur++;
	KI_DPRN("Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
                remote_mac[0], remote_mac[1], remote_mac[2],
                remote_mac[3], remote_mac[4], remote_mac[5]);
	cur++;
	if (type == RX_HOST_DATA) {
		if (*cur == ' ') {
			cur++;
			if ((delim = strchr(cur, '\0')) == NULL)
				goto parse_failed;
		}
	} else if (type == UDC_HOST_DATA) {
		if ((delim = strchr (cur, '\n')) == NULL)
			goto parse_failed;
	} else {
		printk(KERN_INFO"Invalid type of Receive data\n");
		goto parse_failed;
	}
	/* dst port */
	*delim = 0;
	remote_port = simple_strtol (cur, NULL, 10);
	cur = delim + 1;
	KI_DPRN("remote_port ->%d\n",remote_port);
	spin_lock_irqsave(&rxd.rx_lock, flags);
	rxd.np.remote_port = remote_port;
	rxd.np.remote_ip = remote_ip;
	memcpy(&rxd.np.remote_mac, &remote_mac, sizeof(rxd.np.remote_mac));
	rxd.status_bit = 1;
	configured = 1;
	spin_unlock_irqrestore(&rxd.rx_lock, flags);
	return 0;

parse_failed:
	printk(KERN_INFO "parsing failed\n");
	return 1;
}

static char* parse_set_start_parameter(char *cur)
{
	char *delim;
	unsigned long new_survey_interval;
	unsigned long info_interval;

	if (*cur != '0') {
		if ((delim = strchr(cur, ' ')) == NULL)
			if ((delim = strchr(cur, '\0')) == NULL)
				goto parse_failed;
		new_survey_interval = simple_strtoul(cur, NULL, 10);
		if (new_survey_interval < MIN_SURVEY_FREQUENCY) {
			printk(KERN_INFO"Survey frequency should be greater"
				" than or equal to 100ms\n");
			goto parse_failed;
		}
		survey_interval = new_survey_interval;
		cur = delim + 1;
	} else {
		if ((delim = strchr(cur, ' ')) == NULL)
			goto parse_failed;
		survey_interval = DEFAULT_SURVEY_INTERVAL;
		cur = delim + 1;
	}
	if (isdigit(*cur)) {
		if (*cur != '0') {
			info_interval = simple_strtoul(cur, NULL, 10);
			if (info_interval < survey_interval) {
				printk(KERN_INFO"Info frequency should be greater"
					" than or equal to Survey frequency\n");
				goto parse_failed;
			}
			output_info_interval = info_interval;
		} else
			output_info_interval = OUTPUT_INFO_INTERVAL;
		cur = delim + 1;
	}
	return cur;
parse_failed:
	return NULL;
}
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
static int kipct_get_clctr_id(u32 clctr_id)
{
	kipct_clctr_t *p = NULL;

	list_for_each_entry(p, &(s_kipct.clist), list) {
		if (p->clctr_id == clctr_id)
			return 1;
	}
	return 0;
}
#endif
static char* parse_rx_header(char *cur)
{
	char magic[MAX_CMD_SIZE];
	char *delim;
	u16 reserved_bits = 0;
	u32 cltr_id = 0, unit_id = 0;
	int retVal = 0, sizeof_mg;
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
	int dlen = 0;
#endif

	if (*cur != ' ') {
		if ((delim = strchr(cur, ' ')) == NULL)
			goto parse_failed;
		strlcpy(magic, cur,sizeof(magic));
		sizeof_mg = delim - cur;
		if ((retVal = strncmp(magic, "kipt", sizeof_mg)) != 0)
			goto parse_failed;
		cur = delim + 1;
		*delim = 0;
		if ((delim = strchr(cur, ' ')) == NULL)
			goto parse_failed;
		if (!isdigit(*cur)) {
			printk(KERN_INFO"Invalid clctr id\n");
			goto parse_failed;
		}
		if ((cltr_id = simple_strtoul(cur, NULL, 10)) != 0) {
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
			retVal = kipct_get_clctr_id(cltr_id);
			if (!retVal) {
				printk(KERN_INFO"Invalid clctr id\n");
				goto parse_failed;
			}
		}
#else
			goto parse_failed;
		}
#endif
		cur = delim + 1;
		*delim = 0;
		if ((delim = strchr(cur, ' ')) == NULL)
			goto parse_failed;
		if (!isdigit(*cur)) {
			printk(KERN_INFO"Invalid unit id\n");
			goto parse_failed;
		}
		if ((unit_id = simple_strtoul(cur, NULL, 10)) != 0) {
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
			if (cltr_id == 0) {
				printk(KERN_INFO"Invalid unit id\n");
				goto parse_failed;
			}
			retVal = kipct_get_unit_id(unit_id, cltr_id);
			if (!retVal) {
				printk(KERN_INFO"Invalid unit id\n");
				goto parse_failed;
			} else
				clctr_data = 1;
		} else if (!unit_id && cltr_id) {
			printk(KERN_INFO"Invalid unit id\n");
			goto parse_failed;
		}
#else
			goto parse_failed;
		}
#endif
		cur = delim + 1;
		*delim = 0;
		if ((delim = strchr(cur, ' ')) == NULL)
			goto parse_failed;
		if (!isdigit(*cur) || *cur == '0') {
			printk(KERN_INFO"Invalid size\n");
			goto parse_failed;
		}
		kipt_dsize = simple_strtoul(cur, NULL, 10);
		if (kipt_dsize > MAX_DATA_SIZE) {
			printk(KERN_INFO"Data size should be less than 1500 bytes\n");
			goto parse_failed;
		}
		cur = delim + 1;
		*delim = 0;
		if ((delim = strchr(cur, ' ')) == NULL)
			goto parse_failed;
		if (!isdigit(*cur)) {
			printk(KERN_INFO"Invalid reserved bits\n");
			goto parse_failed;
		}
		if ((reserved_bits = simple_strtoul(cur, NULL, 10)) != 0)
			goto parse_failed;
		cur = delim + 1;
	}
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
	if (clctr_data == 1) {
		delim = strchr(cur, '\n');
		dlen = delim - cur;
		if (dlen == kipt_dsize)
			kipct_store_recv_data(cltr_id, unit_id, cur,dlen);
		else {
			printk(KERN_INFO"Invalid size of the Data Message\n");
			goto parse_failed;
		}
	}
#endif
	return cur;
parse_failed:
	return NULL;

}
void kinspect_rx(struct netpoll *nps, int source, char *data, int dlen)
{
	char *p_data = data;
	char *cur = NULL;
	char *delim = NULL;
	int retVal, ret;
	int len = 0;

	KI_DPRN("kinspect_rx netpoll api\n");
	if (dlen > MAX_DATA_SIZE) {
		printk(KERN_INFO"Packet size should be less than 1500 bytes");
		return;
	}
	/* parsing the header*/
	if ((cur = parse_rx_header(p_data)) == NULL) {
		printk(KERN_INFO"Invalid header format\n");
		return;
	}
	p_data = cur;
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
	if (!clctr_data) {
#endif
		strlcpy(rx_buf, p_data, (kipt_dsize + 1));
		rx_buf[kipt_dsize + 1] = '\0';
		if ((delim = strchr(rx_buf, ' ')) == NULL) {
			if((delim = strchr(rx_buf, '\0')) == NULL)
				goto  parse_failed;
		}
		len = delim - rx_buf;
		strlcpy(rx_cmd, rx_buf, len + 1);
		if (!configured) {
			if ((retVal = strcmp(rx_cmd, "host")) == 0) {
				ret = kipct_parse_host_parameter((char*)&rx_buf[len + 1], RX_HOST_DATA);
				if (ret)
					goto parse_failed;
				KI_INFO(" Host IP,MAC Address and Port number is configured\n");
				return;
			} else {
				printk(KERN_INFO"Configure the host parameter\n");
				return;
			}
		}
		if ((retVal = strcmp(rx_cmd, "start")) == 0) {
			if(kipct_survey_get()) {
				printk(KERN_INFO"kinspect is running,cannot be changed\n");
				return;
			}
			if (!isdigit(rx_buf[len +1])) {
				survey_interval = DEFAULT_SURVEY_INTERVAL;
				output_info_interval = OUTPUT_INFO_INTERVAL;
			} else {
				cur = parse_set_start_parameter((char*)&rx_buf[len + 1]);
				if (cur == NULL)
					goto parse_failed;
			}
			kipct_survey = KIPCT_SURVEY_START;
			KI_DPRN("survey_interval- %lu\n", survey_interval);
			KI_DPRN("output_info_interval - %lu\n", output_info_interval);
			mod_timer(&info_timer,(jiffies + msecs_to_jiffies(output_info_interval)));
			mod_timer(&keepalive_timer,(jiffies + msecs_to_jiffies(DEFAULT_KEEPALIVE_INTERVAL)));
			wake_up(&kipct_survey_wq);
			return;
		}
		if ((retVal = strcmp(rx_cmd, "stop")) == 0) {
			if (!kipct_survey_get()) {
				printk(KERN_INFO"kinspect is not running\n");
				return;
			}
			kipct_survey = KIPCT_SURVEY_STOP;
			del_timer_sync(&info_timer);
			del_timer_sync(&keepalive_timer);
			return;
		}
		if ((retVal = strcmp(rx_cmd, "list")) == 0) {
			if (!kipct_survey_get()) {
				printk(KERN_INFO"kinspect is not running\n");
				return;
			}
			send_collector_info(0);
			return;
		}
		if ((retVal = strcmp(rx_cmd, "host")) == 0) {
			ret = kipct_parse_host_parameter((char*)&rx_buf[len + 1], RX_HOST_DATA);
			if (ret)
				goto parse_failed;
			return;
		}
		if ((retVal = strcmp(rx_cmd, "keepalive")) == 0) {
			if (!kipct_survey_get()) {
				printk(KERN_INFO"kinspect is not running\n");
				return;
			}
			keepalive_rtime = sched_clock();
			mod_timer(&keepalive_timer,(jiffies + msecs_to_jiffies(DEFAULT_KEEPALIVE_INTERVAL)));
			return;
		}
#ifdef CONFIG_SNSC_KINSPECT_USERLAND_COLLECTOR_MODULE
	} else
		return;
#endif
parse_failed:
	KI_ERR("Parsing failed\n");
	printk(KERN_INFO"Message to the Collector0(Core):\n"
		"start [[survey-freq-in-ms] [info-freq-in-ms]]\n"
		"stop \n"
		"host <IP Address> <MAC-addess> <remote-port>\n"
		"list\n"
		"keepalive\n");

	return;
}
unsigned long long kipct_get_timestamp(void)
{
	return sched_clock();
}

struct netpoll kipct_get_host_data()
{
	return np;
}

unsigned long kipct_get_frequency()
{
	return  survey_interval;
}

void kipct_set_start_cmd(unsigned long freq)
{
	if (freq)
		survey_interval = freq;
	else
		survey_interval = DEFAULT_SURVEY_INTERVAL;
	kipct_survey = KIPCT_SURVEY_START;
	mod_timer(&info_timer,(jiffies + msecs_to_jiffies(output_info_interval)));
	mod_timer(&keepalive_timer,(jiffies + msecs_to_jiffies(DEFAULT_KEEPALIVE_INTERVAL)));
	wake_up(&kipct_survey_wq);
}

void kipct_set_stop_cmd(void)
{
	del_timer_sync(&info_timer);
	del_timer_sync(&keepalive_timer);
}

void kipct_set_clctrinfo(int clctr_info)
{
	send_collector_info(0);
}

struct kipct kipct_get_clist(void)
{
	return s_kipct;
}

int kipct_get_rconfigure(void)
{
	return configured;
}

unsigned long long kipct_keepalive_recv_time(void)
{
	return keepalive_rtime;
}

late_initcall(init_kipct);
module_exit(cleanup_kipct);

/*
 * module information
 */

EXPORT_SYMBOL(kipct_survey_start);
EXPORT_SYMBOL(kipct_survey_stop);
EXPORT_SYMBOL(kipct_clctr_register);
EXPORT_SYMBOL(kipct_clctr_unregister);
EXPORT_SYMBOL(kipct_format_write);
EXPORT_SYMBOL(kipct_lformat_write);
EXPORT_SYMBOL(kipct_file_write);
EXPORT_SYMBOL(kipct_buf_write);
EXPORT_SYMBOL(kipct_get_file_size);
EXPORT_SYMBOL(kipct_sprintf);
EXPORT_SYMBOL(kipct_write_header);
EXPORT_SYMBOL(kipct_write_tailer);
EXPORT_SYMBOL(kipct_read_file);
EXPORT_SYMBOL(kinspect_rx);
EXPORT_SYMBOL(kipct_parse_host_parameter);
EXPORT_SYMBOL(kipct_survey);
EXPORT_SYMBOL(kipct_get_frequency);
EXPORT_SYMBOL(kipct_set_start_cmd);
EXPORT_SYMBOL(kipct_set_stop_cmd);
EXPORT_SYMBOL(kipct_get_host_data);
EXPORT_SYMBOL(kipct_get_clist);
EXPORT_SYMBOL(kipct_set_clctrinfo);
EXPORT_SYMBOL(kipct_get_timestamp);
EXPORT_SYMBOL(kipct_get_rconfigure);
EXPORT_SYMBOL(kipct_keepalive_recv_time);
MODULE_DESCRIPTION("kernel inspect tool");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sony Corporation");
