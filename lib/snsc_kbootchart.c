/* 2011-02-10: File added and changed by Sony Corporation */
/*
 *  kernel space bootchart with kinspect
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
#include <asm/uaccess.h>

#include "snsc_kinspect.h"

/* #define KBC_DEBUG */

#ifdef KBC_DEBUG
#define KBC_DPRN(fmt, args ...) \
	    printk(KERN_DEBUG "%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);
#else /* KBC_DEBUG */
#define KBC_DPRN(fmt, args ...)
#endif /* KBC_DEBUG */

#define KBC_ERR(fmt, args ...) \
	    printk(KERN_ERR "%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);

#define KBOOTCHART "kbootchart"

#define KBOOTCHART_UNIT_HEADER	  0
#define KBOOTCHART_UNIT_PROC_STAT 1
#define KBOOTCHART_UNIT_PID_STAT  2
#define KBOOTCHART_UNIT_DISK_STAT 3
#define MAX_KBOOTCHART_UNIT	  4


#define KBC_UNIT_HEADER	   "header"
#define KBC_UNIT_PROC_STAT "proc_stat.log"
#define KBC_UNIT_PID_STAT  "proc_ps.log"
#define KBC_UNIT_DISK_STAT "proc_diskstats.log"

#define KBOOTCHARTD_VERSION "0.8"

#define KBC_UNIT_ENABLE_BIT   0
#define KBC_UNIT_FIRST_BIT    1

typedef struct kbc_info {
	kipct_write_t winfo;
	unsigned long attr;
} kbc_info_t;

static kbc_info_t kbc_info_ary[MAX_KBOOTCHART_UNIT];

typedef struct kbc_unitinfo {
	int id;
	const char *name;
} kbc_unitinfo_t;

static kbc_unitinfo_t kbc_unitinfo_data[] =
{
	{
		.id = KBOOTCHART_UNIT_HEADER,
		.name = KBC_UNIT_HEADER
	},
	{
		.id = KBOOTCHART_UNIT_PROC_STAT,
		.name = KBC_UNIT_PROC_STAT
	},
	{
		.id = KBOOTCHART_UNIT_PID_STAT,
		.name = KBC_UNIT_PID_STAT
	},
	{
		.id = KBOOTCHART_UNIT_DISK_STAT,
		.name = KBC_UNIT_DISK_STAT
	}
};

static int s_insmod_end = 0;

#define __RECORD_SEPARATOR__	kipct_sprintf(buf, maxlen, &len, "%\n")

/*
 * prototype
 */
static int survey_param_set(const char *val, struct kernel_param *kp, int id);
static int kbootchart_unit_header(unsigned int index, kipct_cb_arg_t *arg);
static int kbootchart_unit_file(unsigned int index, kipct_cb_arg_t *arg,
			      const char *filename);
static int kbootchart_unit_pid_stat(unsigned int index, kipct_cb_arg_t *arg);
static int kbootchart_cb(kipct_cb_arg_t * arg);
static int kbootchart_info_cb(kipct_cb_arg_t * arg);

static kipct_clctr_rgst_t s_rgst = {
	.clctr_id = 0,
	.clctr_name = KBOOTCHART,
	.cb = kbootchart_cb,
	.infocb = kbootchart_info_cb
};


static int header_survey = 1;
/* survey setter */
static int header_survey_set(const char *val, struct kernel_param *kp)
{
	return survey_param_set(val, kp, KBOOTCHART_UNIT_HEADER);
}
/* survey getter */
static int header_survey_get(char *buffer, struct kernel_param *kp)
{
	KBC_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}
module_param_call(header_survey, header_survey_set,
		  header_survey_get, &header_survey, 0644);
MODULE_INFO(paramtype, "header_survey:int");
MODULE_PARM_DESC(header_survey, "start(1), stop(0)\n");

static int proc_stat_survey = 1;
/* survey setter */
static int proc_stat_survey_set(const char *val, struct kernel_param *kp)
{
	return survey_param_set(val, kp, KBOOTCHART_UNIT_PROC_STAT);
}
/* survey getter */
static int proc_stat_survey_get(char *buffer, struct kernel_param *kp)
{
	KBC_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}
module_param_call(proc_stat_survey, proc_stat_survey_set,
		  proc_stat_survey_get, &proc_stat_survey, 0644);
MODULE_INFO(paramtype, "proc_stat_survey:int");
MODULE_PARM_DESC(proc_stat_survey, "start(1), stop(0)\n");

static int pid_stat_survey = 1;
/* survey setter */
static int pid_stat_survey_set(const char *val, struct kernel_param *kp)
{
	return survey_param_set(val, kp, KBOOTCHART_UNIT_PID_STAT);
}
/* survey getter */
static int pid_stat_survey_get(char *buffer, struct kernel_param *kp)
{
	KBC_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}
module_param_call(pid_stat_survey, pid_stat_survey_set,
		  pid_stat_survey_get, &pid_stat_survey, 0644);
MODULE_INFO(paramtype, "pid_stat_survey:int");
MODULE_PARM_DESC(pid_stat_survey, "start(1), stop(0)\n");

static int diskstats_survey = 1;
/* survey setter */
static int diskstats_survey_set(const char *val, struct kernel_param *kp)
{
	return survey_param_set(val, kp, KBOOTCHART_UNIT_DISK_STAT);
}
/* survey getter */
static int diskstats_survey_get(char *buffer, struct kernel_param *kp)
{
	KBC_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}
module_param_call(diskstats_survey, diskstats_survey_set,
		  diskstats_survey_get, &diskstats_survey, 0644);
MODULE_INFO(paramtype, "diskstats_survey:int");
MODULE_PARM_DESC(diskstats_survey, "start(1), stop(0)\n");

/*
 * internal function
 */
static int
survey_param_set(const char *val, struct kernel_param *kp, int id)
{
	long int stat = 0;
	int retVal = 0;

	if (val) {
		stat = simple_strtol(val, NULL, 0);
		KBC_DPRN("%s:set: val= <%ld>\n", kp->name, stat);
	}
	retVal = param_set_int(val, kp);
	if (retVal == 0) {
		if (stat) {
			set_bit(KBC_UNIT_ENABLE_BIT,
				(void*)(&kbc_info_ary[id].attr));
		}
		else {
			clear_bit(KBC_UNIT_ENABLE_BIT,
				(void*)(&kbc_info_ary[id].attr));
		}
	}
	if (s_insmod_end == 0)
		return 0;

	return retVal;
}


static int
kbc_call_unit(int (*unit)(unsigned int, kipct_cb_arg_t *),
	    int (*unit_file)(unsigned int, kipct_cb_arg_t *, const char*),
	    int id, kipct_cb_arg_t *arg, const char* file)
{
	int retVal = 0;

	if (test_bit(KBC_UNIT_ENABLE_BIT, (void*)(&kbc_info_ary[id].attr))) {
		if (unit) {
			retVal = unit(id, arg);
		}
		else if(unit_file) {
			retVal = unit_file(id, arg, file);
		}
		else {
			return -EINVAL;
		}
		if ((retVal == 0) &&
		    (test_bit(KBC_UNIT_FIRST_BIT, (void*)(&kbc_info_ary[id].attr)
			    ))) {
			clear_bit(KBC_UNIT_ENABLE_BIT,
				  (void*)(&kbc_info_ary[id].attr));
		}
	}

	return retVal;
}


/*
 * callback function
 */
static int
kbootchart_cb(kipct_cb_arg_t * arg)
{
	KBC_DPRN("\n");

	if(!kipct_survey_get())
		return 0;
	if (header_survey) {
		(void)kbc_call_unit(kbootchart_unit_header, NULL,
				  KBOOTCHART_UNIT_HEADER, arg, NULL);
		KBC_DPRN("header end\n");
	}
	if (proc_stat_survey) {
		(void)kbc_call_unit(NULL, kbootchart_unit_file,
				  KBOOTCHART_UNIT_PROC_STAT, arg, "/proc/stat");
		KBC_DPRN("stat end\n");
	}
	if (pid_stat_survey) {
		(void)kbc_call_unit(kbootchart_unit_pid_stat, NULL,
				  KBOOTCHART_UNIT_PID_STAT, arg, NULL);
		KBC_DPRN("pid stat end\n");
	}
	if (diskstats_survey) {
		(void)kbc_call_unit(NULL, kbootchart_unit_file,
				  KBOOTCHART_UNIT_DISK_STAT, arg,
				  "/proc/diskstats");
		KBC_DPRN("disk stat end\n");
	}

	return 0;
}

static int
kbootchart_info_cb(kipct_cb_arg_t * arg)
{
	kipct_buf_t *bufinfo = NULL;
	char *buf = NULL;
	unsigned long maxlen = 0;
	unsigned int len = 0;
	int id = 0;

	KBC_DPRN("\n");

	bufinfo = &arg->bufinfo;
	buf = bufinfo->buf;
	maxlen = bufinfo->max_buf_len;

	(void)kipct_sprintf(buf, maxlen, &len, "%s:%d:%s",
			    "%I", s_rgst.clctr_id,
			    KBOOTCHART);
	for (id = 0; id < MAX_KBOOTCHART_UNIT; id++) {
		(void)kipct_sprintf(buf, maxlen, &len, ":%d:%s",
				    kbc_unitinfo_data[id].id,
				    kbc_unitinfo_data[id].name);
	}
	(void)kipct_sprintf(buf, maxlen, &len, "\n");

	(void)kipct_buf_write(bufinfo->buf, len);

	return 0;
}

static int
kbootchart_unit_header(unsigned int index, kipct_cb_arg_t *arg)
{
	kipct_write_t *info;
	kipct_buf_t *bufinfo = NULL;
	char *buf = NULL;
	unsigned int len = 0;
	unsigned int header_len = 0;
	unsigned int size = 0;
	unsigned long maxlen = 0;
	int retVal = 0;

	KBC_DPRN("\n");

	info = &(kbc_info_ary[index].winfo);
	bufinfo = &arg->bufinfo;
	buf = bufinfo->buf;
	maxlen = bufinfo->max_buf_len;

	KBC_DPRN("buf=%p maxlen=%lX\n", buf, maxlen);

	retVal = kipct_write_header(bufinfo, info, &len);
	if (retVal < 0) { return retVal; }
	header_len = len;
	retVal = kipct_sprintf(buf, maxlen, &len, "header_start %s\n",
				  KBOOTCHARTD_VERSION);
	if (retVal < 0) { return retVal; }
	retVal = __RECORD_SEPARATOR__;
	if (retVal < 0) { return retVal; }
	retVal = kipct_read_file(buf, &len, "/proc/sys/kernel/hostname",
				    maxlen);
	if (retVal < 0) { return retVal; }
	retVal = __RECORD_SEPARATOR__;
	if (retVal < 0) { return retVal; }
	retVal = kipct_read_file(buf, &len, "/proc/version", maxlen);
	if (retVal < 0) { return retVal; }
	retVal = __RECORD_SEPARATOR__;
	if (retVal < 0) { return retVal; }
	retVal = kipct_read_file(buf, &len, "/proc/nlver", maxlen);
	if (retVal < 0) { return retVal; }
	retVal = __RECORD_SEPARATOR__;
	if (retVal < 0) { return retVal; }
	retVal = kipct_read_file(buf, &len, "/proc/cpuinfo", maxlen);
	if (retVal < 0) { return retVal; }
	retVal = __RECORD_SEPARATOR__;
	if (retVal < 0) { return retVal; }
	retVal = kipct_read_file(buf, &len, "/proc/cmdline", maxlen);
	if (retVal < 0) { return retVal; }
	size = len - header_len;
	retVal = kipct_write_tailer(bufinfo, info, &len, size);
	if (retVal < 0) { return retVal; }

	(void)kipct_buf_write(bufinfo->buf, len);
	(info->seq_id)++;

	return retVal;
}


static int
kbootchart_unit_file(unsigned int index, kipct_cb_arg_t *arg,
		   const char *filename)
{
	kipct_write_t *info;
	int retVal = 0;

	KBC_DPRN("\n");

	info = &(kbc_info_ary[index].winfo);

	retVal = kipct_file_write(info, filename);
	if (retVal < 0) {
		return retVal;
	}

	(info->seq_id)++;

	return retVal;
}

static int
kbootchart_unit_pid_stat(unsigned int index, kipct_cb_arg_t *arg)
{
	struct task_struct *task;
	char file_name[64];
	kipct_write_t *info;
	kipct_buf_t *bufinfo = NULL;
	char *buf = NULL;
	unsigned int len = 0;
	unsigned int header_len = 0;
	unsigned int size = 0;
	unsigned long maxlen = 0;
	int retVal = 0;

	KBC_DPRN("\n");

	info = &(kbc_info_ary[index].winfo);
	bufinfo = &arg->bufinfo;
	buf = bufinfo->buf;
	maxlen = bufinfo->max_buf_len;

	KBC_DPRN("\n");

	retVal = kipct_write_header(bufinfo, info, &len);
	if (retVal < 0) { return retVal; }
	header_len = len;
	for_each_process(task) {
		if (task) {
			snprintf(file_name, 32, "/proc/%d/stat", task->tgid);
			retVal = kipct_read_file(buf, &len,
						    file_name, maxlen);
			if (retVal < 0) { return retVal; }
		}
	}
	size = len - header_len;
	retVal = kipct_write_tailer(bufinfo, info, &len, size);
	if (retVal < 0) { return retVal; }

	(void)kipct_buf_write(bufinfo->buf, len);
	(info->seq_id)++;

	return retVal;
}


/*
 * insmod/rmmod
 */

static int init_kbootchart(void)
{
	int retVal = 0;
	int id = 0;

	memset(kbc_info_ary, 0, sizeof(kbc_info_ary));

	retVal = kipct_clctr_register(&s_rgst);
	if (retVal < 0) {
		KBC_ERR("fault: kinspect_clctr_register()\n");
		return -EINVAL;
	}

	/* initialize kbc_info */
	for (id = 0; id < MAX_KBOOTCHART_UNIT; id++) {
		kbc_info_ary[id].winfo.clctr_id = s_rgst.clctr_id;
		kbc_info_ary[id].winfo.unit_id = id;
		kbc_info_ary[id].winfo.seq_id = 0;
		set_bit(KBC_UNIT_ENABLE_BIT, (void*)(&kbc_info_ary[id].attr));
		if (id == KBOOTCHART_UNIT_HEADER) {
			set_bit(KBC_UNIT_FIRST_BIT,
				(void*)(&kbc_info_ary[id].attr));
		}
	}

	s_insmod_end = 1;
	printk(KERN_INFO "kbootchart: insmod\n");
	return 0;
}

static void cleanup_kbootchart(void)
{
	(void)kipct_clctr_unregister(&s_rgst);

	printk(KERN_INFO "kbootchart: rmmod\n");
}

late_initcall(init_kbootchart);
module_exit(cleanup_kbootchart);

#ifndef MODULE
static int __init kbc_header_survey(char *str)
{
	header_survey = simple_strtol(str, NULL, 0);
	return 1;
}
static int __init kbc_proc_stat_survey(char *str)
{
	proc_stat_survey = simple_strtol(str, NULL, 0);
	return 1;
}
static int __init kbc_pid_stat_survey(char *str)
{
	pid_stat_survey = simple_strtol(str, NULL, 0);
	return 1;
}
static int __init kbc_diskstats_survey(char *str)
{
	diskstats_survey = simple_strtol(str, NULL, 0);
	return 1;
}
__setup("kbootchart_header_survey=", kbc_header_survey);
__setup("kbootchart_proc_stat_survey=", kbc_proc_stat_survey);
__setup("kbootchart_pid_stat_survey=", kbc_pid_stat_survey);
__setup("kbootchart_diskstats_survey=", kbc_diskstats_survey);
#endif /* !MODULE */

/*
 * module information
 */

MODULE_DESCRIPTION("kbootchart with kinspect");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sony Corporation");
