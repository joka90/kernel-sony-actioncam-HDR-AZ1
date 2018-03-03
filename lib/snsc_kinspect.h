/* 2010-12-15: File added by Sony Corporation */
/*
 *  kinspect - kernel inspect tool
 */
/*
 * Copyright 2007-2009 Sony Corporation.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation; version 2 of the  License.
 *
 * THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 * WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __SNSC_KINSPECT_H__
#define __SNSC_KINSPECT_H__


#define KIPCT_SURVEY_STOP   0
#define KIPCT_SURVEY_START  1
#define MIN_SURVEY_FREQUENCY 100
#define RX_HOST_DATA 1
#define UDC_HOST_DATA 2
extern int kipct_survey;
extern unsigned long long udc_get_timestamp;
/* structure */

typedef struct kipct_buf {
	char *buf;
	int max_buf_len;
} kipct_buf_t;

typedef struct kipct_cb_arg {
	kipct_buf_t bufinfo;
	void *um_arg;
	void *priv;
} kipct_cb_arg_t;

typedef int (*kipct_cb)(kipct_cb_arg_t * arg);

typedef struct kipct_clctr_rgst {
	int clctr_id;
	char *clctr_name;
	kipct_cb cb;
	kipct_cb infocb;
	void *um_arg;
	void *priv;
} kipct_clctr_rgst_t;

typedef struct kipct_write {
	int clctr_id;
	int unit_id;
	int seq_id;
	unsigned long long timestamp;
} kipct_write_t;

/* collector management */
typedef struct kipct_clctr {
	struct list_head list;
	int clctr_id; /* collector id */
	char *clctr_name; /* collector name */
	kipct_cb cb;
	kipct_cb infocb;
	void *um_arg;
	void *priv;
	int enable;
} kipct_clctr_t;

typedef struct kipct_stbuf {
	char *store;
	spinlock_t buf_lock;
	unsigned long offset;
	unsigned long total;
	unsigned long wtotal;
} kipct_stbuf_t;

typedef struct kipct_outinfo {
	kipct_stbuf_t stbuf;
	int network_enable;
} kipct_outinfo_t;

/* kipct  */
typedef struct kipct {
	struct semaphore sem;
	struct list_head clist;
	int survey; /* big switch */
	int clctr_num;
	char *page_buf;
	spinlock_t page_buf_lock;
	char *buf;
	unsigned int max_buf_str_len;
	spinlock_t net_out_lock;
	kipct_outinfo_t outinfo;
	kipct_write_t winfo;
	struct proc_dir_entry *proc_file;
} kipct_t;

/* rx netpoll structure */
typedef struct rx_data {
	struct netpoll np;
	int status_bit;
	spinlock_t rx_lock;
}rx_data_t;

/* exported function */
int kipct_survey_start(void);
int kipct_survey_stop(void);

int kipct_clctr_register(kipct_clctr_rgst_t *rgst);
int kipct_clctr_unregister(kipct_clctr_rgst_t *rgst);

int kipct_format_write(kipct_write_t *info, const char *format, ...);
int kipct_lformat_write(kipct_write_t *info, const char *format, ...);
int kipct_file_write(kipct_write_t *info, const char* file);
int kipct_buf_write(char *buf, unsigned long len);
loff_t kipct_get_file_size(const char * file);
int kipct_sprintf(char *buf, unsigned int maxlen,
		  unsigned int *lenp, const char *format, ...);
int kipct_write_header(kipct_buf_t *bufinfo, kipct_write_t *info,
		       unsigned int *lenp);
int kipct_write_tailer(kipct_buf_t *bufinfo, kipct_write_t *info,
		       unsigned int *lenp, unsigned int body_length);
int kipct_read_file(char *buf, int *lenp, const char* file,
		    unsigned int max_buf_len);
static int kipct_survey_get(void);
int kipct_parse_host_parameter(char *, int);
void  kinspect_rx(struct netpoll *nps, int source, char *data, int dlen);

inline int kipct_survey_get()
{
	return  kipct_survey;
}

void kipct_set_start_cmd(unsigned long);
void kipct_set_stop_cmd(void);
struct netpoll kipct_get_host_data(void);
void kipct_set_clctrinfo(int);
unsigned long kipct_get_frequency(void);
kipct_t kipct_get_clist(void);
int kipct_get_unit_id(u32, u32);
void kipct_store_recv_data(u32, u32, char*, int );
unsigned long long kipct_get_timestamp(void);
int kipct_get_rconfigure(void);
unsigned long long kipct_keepalive_recv_time(void);

#endif /* __SNSC_KINSPECT_H__ */
