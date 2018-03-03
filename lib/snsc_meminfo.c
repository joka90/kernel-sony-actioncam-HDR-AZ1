/* 2011-02-10: File added and changed by Sony Corporation */
/*
 *  meminfo with kinspect
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
#include <asm/uaccess.h>

#include "snsc_kinspect.h"

/* #define MI_DEBUG */

#ifdef MI_DEBUG
#define MI_DPRN(fmt, args ...) \
	    printk(KERN_DEBUG "%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);
#else /* MI_DEBUG */
#define MI_DPRN(fmt, args ...)
#endif /* MI_DEBUG */

#define MI_ERR(fmt, args ...) \
	    printk(KERN_ERR "%s(%d): " fmt, \
		   __FUNCTION__, __LINE__, ##args);

#define MEMINFO "meminfo"

#define MEMINFO_UNIT_MEMINFO	0
#ifdef CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO
#define MEMINFO_UNIT_BUDDYINFO	1
#define MAX_MEMINFO_UNIT		2
#else /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */
#define MAX_MEMINFO_UNIT		1
#endif /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */

#define MI_UNIT_MEMINFO	 "meminfo"
#ifdef CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO
#define MI_UNIT_BUDDYINFO	 "buddyinfo"
#endif /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */

#define MI_UNIT_ENABLE_BIT   0

typedef struct mi_info {
	kipct_write_t winfo;
	unsigned long attr;
} mi_info_t;

static mi_info_t mi_info_ary[MAX_MEMINFO_UNIT];

typedef struct mi_unitinfo {
	int id;
	const char *name;
} mi_unitinfo_t;

static mi_unitinfo_t mi_unitinfo_data[] =
{
	{
		.id = MEMINFO_UNIT_MEMINFO,
		.name = MI_UNIT_MEMINFO
	},
#ifdef CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO
	{
		.id = MEMINFO_UNIT_BUDDYINFO,
		.name = MI_UNIT_BUDDYINFO
	},
#endif /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */
};

static int s_insmod_end = 0;

#define __RECORD_SEPARATOR__	kipct_sprintf(buf, maxlen, &len, "%\n")

/*
 * prototype
 */
static int survey_param_set(const char *val, struct kernel_param *kp, int id);

static int meminfo_unit_file(unsigned int index, kipct_cb_arg_t *arg,
			   const char* filename);

static int meminfo_cb(kipct_cb_arg_t * arg);
static int meminfo_info_cb(kipct_cb_arg_t * arg);


static kipct_clctr_rgst_t s_rgst = {
	.clctr_id = 0,
	.clctr_name = MEMINFO,
	.cb = meminfo_cb,
	.infocb = meminfo_info_cb
};


static int meminfo_survey = 1;
/* survey setter */
static int meminfo_survey_set(const char *val, struct kernel_param *kp)
{
	return survey_param_set(val, kp, MEMINFO_UNIT_MEMINFO);
}
/* survey getter */
static int meminfo_survey_get(char *buffer, struct kernel_param *kp)
{
	MI_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}
module_param_call(meminfo_survey, meminfo_survey_set,
		  meminfo_survey_get, &meminfo_survey, 0644);
MODULE_INFO(paramtype, "meminfo_survey:int");
MODULE_PARM_DESC(meminfo_survey, "start(1), stop(0)\n");

#ifdef CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO
static int buddyinfo_survey = 1;
/* survey setter */
static int buddyinfo_survey_set(const char *val, struct kernel_param *kp)
{
	return survey_param_set(val, kp, MEMINFO_UNIT_BUDDYINFO);
}
/* survey getter */
static int buddyinfo_survey_get(char *buffer, struct kernel_param *kp)
{
	MI_DPRN("%s:set: val= <%d>\n", kp->name, *((int *)kp->arg));
	return (param_get_int(buffer, kp));
}
module_param_call(buddyinfo_survey, buddyinfo_survey_set,
		  buddyinfo_survey_get, &buddyinfo_survey, 0644);
MODULE_INFO(paramtype, "buddyinfo_survey:int");
MODULE_PARM_DESC(buddyinfo_survey, "start(1), stop(0)\n");
#endif /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */

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
		MI_DPRN("%s:set: val= <%ld>\n", kp->name, stat);
	}
	retVal = param_set_int(val, kp);
	if (retVal == 0) {
		if (stat) {
			set_bit(MI_UNIT_ENABLE_BIT,
				(void*)(&mi_info_ary[id].attr));
		}
		else {
			clear_bit(MI_UNIT_ENABLE_BIT,
				(void*)(&mi_info_ary[id].attr));
		}
	}
	if (s_insmod_end == 0)
		return 0;

	return retVal;
}


static int
mi_call_unit(int (*unit_file)(unsigned int, kipct_cb_arg_t *, const char*),
	    int id, kipct_cb_arg_t *arg, const char* file)
{
	int retVal = 0;

	if (test_bit(MI_UNIT_ENABLE_BIT, (void*)(&mi_info_ary[id].attr))) {
		if (unit_file) {
			retVal = unit_file(id, arg, file);
		}
		else {
			return -EINVAL;
		}
	}

	return retVal;
}


/*
 * callback function
 */
static int
meminfo_cb(kipct_cb_arg_t * arg)
{
	MI_DPRN("\n");
	if(!kipct_survey_get())
		return 0;
	if (meminfo_survey) {
		(void)mi_call_unit(meminfo_unit_file,
				 MEMINFO_UNIT_MEMINFO, arg, "/proc/meminfo");
		MI_DPRN("meminfo end\n");
	}
#ifdef CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO
	if (buddyinfo_survey) {
		(void)mi_call_unit(meminfo_unit_file,
				 MEMINFO_UNIT_BUDDYINFO, arg, "/proc/buddyinfo");
	MI_DPRN("buddyinfo end\n");
	}
#endif /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */

	return 0;
}

static int
meminfo_info_cb(kipct_cb_arg_t * arg)
{
	kipct_buf_t *bufinfo = NULL;
	char *buf = NULL;
	unsigned long maxlen = 0;
	unsigned int len = 0;
	int id = 0;

	MI_DPRN("\n");

	bufinfo = &arg->bufinfo;
	buf = bufinfo->buf;
	maxlen = bufinfo->max_buf_len;

	(void)kipct_sprintf(buf, maxlen, &len, "%s:%d:%s",
			    "%I", s_rgst.clctr_id,
			    MEMINFO);
	for (id = 0; id < MAX_MEMINFO_UNIT; id++) {
		(void)kipct_sprintf(buf, maxlen, &len, ":%d:%s",
				    mi_unitinfo_data[id].id,
				    mi_unitinfo_data[id].name);
	}
	(void)kipct_sprintf(buf, maxlen, &len, "\n");

	(void)kipct_buf_write(buf, len);

	return 0;
}

static int
meminfo_unit_file(unsigned int index, kipct_cb_arg_t *arg,
		const char *filename)
{
	kipct_write_t *info;
	int retVal = 0;

	MI_DPRN("\n");

	info = &(mi_info_ary[index].winfo);

	retVal = kipct_file_write(info, filename);
	if (retVal < 0) {
		return retVal;
	}

	(info->seq_id)++;

	return retVal;
}



/*
 * insmod/rmmod
 */

static int init_meminfo(void)
{
	int retVal = 0;
	int id = 0;

	memset(mi_info_ary, 0, sizeof(mi_info_ary));

	retVal = kipct_clctr_register(&s_rgst);
	if (retVal < 0) {
		MI_ERR("fault: kinspect_clctr_register()\n");
		return -EINVAL;
	}

	/* initialize mi_info */
	for (id = 0; id < MAX_MEMINFO_UNIT; id++) {
		mi_info_ary[id].winfo.clctr_id = s_rgst.clctr_id;
		mi_info_ary[id].winfo.unit_id = id;
		mi_info_ary[id].winfo.seq_id = 0;
		set_bit(MI_UNIT_ENABLE_BIT, (void*)(&mi_info_ary[id].attr));
	}

	s_insmod_end = 1;
	printk(KERN_INFO "kipct-meminfo: insmod\n");
	return 0;
}

static void cleanup_meminfo(void)
{
	(void)kipct_clctr_unregister(&s_rgst);

	printk(KERN_INFO "kipct-meminfo: rmmod\n");
}

late_initcall(init_meminfo);
module_exit(cleanup_meminfo);

#ifndef MODULE
static int __init mi_meminfo_survey(char *str)
{
	meminfo_survey = simple_strtol(str, NULL, 0);
	return 1;
}
__setup("meminfo_meminfo_survey=", mi_meminfo_survey);
#ifdef CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO
static int __init mi_buddyinfo_survey(char *str)
{
	buddyinfo_survey = simple_strtol(str, NULL, 0);
	return 1;
}
__setup("meminfo_buddyinfo_survey=", mi_buddyinfo_survey);
#endif /* CONFIG_SNSC_KINSPECT_MEMINFO_BUDDYINFO */
#endif /* !MODULE */

/*
 * module information
 */

MODULE_DESCRIPTION("meminfo kinspect collector");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sony Corporation");
