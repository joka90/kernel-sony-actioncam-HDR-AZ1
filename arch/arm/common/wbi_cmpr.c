/*
 * arch/arm/common/wbi_cmpr.c
 *
 * WBI compress sequencer
 *
 * Copyright 2011 Sony Corporation
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
 *
 */
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/snsc_boot_time.h>

#include <asm/cacheflush.h>
#include <asm/mach/warmboot.h>
#include <asm/io.h>
#include <mach/time.h>
#include <mach/noncache.h>
#include <mach/moduleparam.h>
#include "wbi.h"

/*
 * Note:
 *   During compression, DO NOT USE kernel API. (ex. printk, BOOT_TIME).
 *   Because there are side effects on heap. (ex. lock variable)
 */
extern void printascii(const char *string);
extern void printhex8(const ulong);

#define SECTOR_SHIFT 9
/* debug */
#undef WBI_EXEC_DEBUG

static int wbi_nbuf = 0;
module_param_named(nbuf, wbi_nbuf, int, S_IRUSR|S_IWUSR);
static int wbi_cancel_tmo = 100000; /* us */
module_param_named(cancel_tmo, wbi_cancel_tmo, int, S_IRUSR|S_IWUSR);

/*
 * Elapse time
 */
#define WBI_STAT

#define WBISTAT_EXEC		0
#define WBISTAT_FLUSH		1
#define WBISTAT_CMPRWAIT	2
#define WBISTAT_CMPROUT		3
#define N_WBISTAT 		4

#ifdef WBI_STAT
static u32 wbi_elapse[N_WBISTAT];
static u32 wbi_elapse_cmprw;
static void wbistat_init(void)
{
	memset(wbi_elapse, 0, sizeof wbi_elapse);
	wbi_elapse_cmprw = 0;
}
#define WBISTAT_DECL	u32 __t1
#define WBISTAT_START	do { __t1 = mach_read_cycles(); } while (0)
#define WBISTAT_STOP(x)	do { __t1 = mach_read_cycles() - __t1; \
			     wbi_elapse[(x)] += __t1; } while (0)
#define WBISTAT_START2(x) do { (x)->t1 = mach_read_cycles(); } while (0)
#define WBISTAT_STOP2(x) \
	do { (x)->t1 = mach_read_cycles() - (x)->t1; \
	     (x)->t_sum += (x)->t1; \
	     if ((x)->t1 < (x)->t_min) { (x)->t_min = (x)->t1; } \
	     if ((x)->t1 > (x)->t_max) { (x)->t_max = (x)->t1; } \
	     (x)->t_cnt++; \
	} while (0)
#else /* WBI_STAT */
static void wbistat_init(void){}
#define WBISTAT_DECL
#define WBISTAT_START	do { } while (0)
#define WBISTAT_STOP(x)	do { } while (0)
#endif /* WBI_STAT */

/*
 * time stamp
 */
static ulong wbi_stamp_addr = 0;
module_param_named(stamp_addr, wbi_stamp_addr, ulongH, S_IRUSR);
static ulong wbi_stamp_size = 0;
module_param_named(stamp_size, wbi_stamp_size, ulongH, S_IRUSR);

typedef struct {
	u32 t;
	u32 id;
} wbi_stamp_t;
static wbi_stamp_t *wbi_stamp;
static ulong wbi_stamp_end, wbi_stampi;

static void wbistamp_init(void)
{
	wbi_stamp = (wbi_stamp_t *)wbi_stamp_addr;
	wbi_stamp_end = wbi_stamp_size >> 3;
	wbi_stampi = 0;

	if (!wbi_stamp_addr || !wbi_stamp_size)
		wbi_stamp = NULL;
}

static inline void wbistamp(int id)
{
	if (!wbi_stamp)
		return;
	if (wbi_stampi < wbi_stamp_end) {
		wbi_stamp[wbi_stampi].t = mach_read_cycles();
		wbi_stamp[wbi_stampi].id = id;
		wbi_stampi++;
	}
}
/* timestamp ID */
#define WBISTAMP_CM_S(x) ((x) << 1)
#define WBISTAMP_CM_E(x) (((x) << 1) + 1)
#define WBISTAMP_WR_S 10
#define WBISTAMP_WR_E 11
#define WBISTAMP_CO_S 12
#define WBISTAMP_CO_E 13
#define WBISTAMP_CO_W 14
#define WBISTAMP_RD_S 15
#define WBISTAMP_RD_E 16
static char *wbistamp_mnemonic[] = {
	[WBISTAMP_CM_S(0)] = "C0 s",
	[WBISTAMP_CM_E(0)] = "C0 e",
	[WBISTAMP_CM_S(1)] = "C1 s",
	[WBISTAMP_CM_E(1)] = "C1 e",
	[WBISTAMP_CM_S(2)] = "C2 s",
	[WBISTAMP_CM_E(2)] = "C2 e",
	[WBISTAMP_CM_S(3)] = "C3 s",
	[WBISTAMP_CM_E(3)] = "C3 e",
	[WBISTAMP_WR_S]    = "W  s",
	[WBISTAMP_WR_E]    = "W  e",
	[WBISTAMP_CO_S]    = "B  s",
	[WBISTAMP_CO_E]    = "B  e",
	[WBISTAMP_CO_W]    = "B  w",
};
/*---------------------------------------------------------*/

/*
 * queue
 */
typedef struct {
	void *next;
} wbi_queue_elem_t;

typedef struct {
	wbi_queue_elem_t *head, *tail;
} wbi_queue_t;

static void wbi_queue_init(wbi_queue_t *q)
{
	q->head = q->tail = NULL;
}

static void wbi_enqueue(wbi_queue_t *q, wbi_queue_elem_t *e)
{
	/* add to the tail */
	e->next = NULL;
	if (q->tail) {
		q->tail->next = e;
	} else {
		q->head = e;
	}
	q->tail = e;
}

static inline wbi_queue_elem_t *wbi_queue_peek(wbi_queue_t *q)
{
	return q->head;
}

static wbi_queue_elem_t *wbi_dequeue(wbi_queue_t *q)
{
	wbi_queue_elem_t *now;

	now = wbi_queue_peek(q);
	if (!now) {
		return NULL;
	}
	q->head = now->next;
	now->next = NULL;
	if (q->tail == now) {
		q->tail = NULL;
	}
	return now;
}

static void wbi_queue_dump(wbi_queue_t *q)
{
	wbi_queue_elem_t *p = q->head;
	int limit = 32;

	wb_printk("[");
	while (p) {
		if (--limit < 0) {
			wb_printk(" ...");
			break;
		}
		wb_printk(" %p", p);
		p = p->next;
	}
	wb_printk(" ]");
}
/*---------------------------------------------------------*/

/*
 * Task control
 */
typedef struct _wbi_tsk {
	struct _wbi_tsk *next;
	int state;
	const int prio;
	int (* const init)(struct _wbi_tsk *);
	int  (* const handler)(struct _wbi_tsk *);
	const char * const name;
} wbi_tsk_t;
/* task state */
#define TS_IDLE		0
#define TS_RUN		1
#define TS_WAIT		2

/* task priority */
#define N_WBIPRI	3
#if N_WBIPRI > BITS_PER_LONG
#error "N_WBIPRI > BITS_PER_LONG"
#endif

/* task ID */
#define TID_SEND	0 /* write to WBUF */
#define TID_CMPR_I	1 /* send to CMPR */
#define TID_CMPR_W	2 /* wait for finish CMPR */
#define TID_CMPR_O	3 /* write CMPR output to WBUF */
#define TID_WRITER	4 /* write data */

static int wbi_tsk_send(wbi_tsk_t *);
static int wbi_tsk_send_init(wbi_tsk_t *);
static int wbi_tsk_cmpr(wbi_tsk_t *);
static int wbi_tsk_cmpr_init(wbi_tsk_t *);
static int wbi_tsk_cmprwait(wbi_tsk_t *);
static int wbi_tsk_cmprout(wbi_tsk_t *);
static int wbi_tsk_cmprout_init(wbi_tsk_t *);
static int wbi_tsk_wbuf(wbi_tsk_t *);
static int wbi_tsk_wbuf_init(wbi_tsk_t *);
static wbi_tsk_t wbi_tsks[] = {
	[TID_SEND] = {
		.prio = 0,
		.init = wbi_tsk_send_init,
		.handler = wbi_tsk_send,
		.name = "send" },
	[TID_CMPR_I] = {
		.prio = 0,
		.init = wbi_tsk_cmpr_init,
		.handler = wbi_tsk_cmpr,
		.name = "cmpr" },
	[TID_CMPR_W] = {
		.prio = 2,
		.handler = wbi_tsk_cmprwait,
		.name = "cmprwait" },
	[TID_CMPR_O] = {
		.prio = 1,
		.init = wbi_tsk_cmprout_init,
		.handler = wbi_tsk_cmprout,
		.name = "cmprout" },
	[TID_WRITER] = {
		.prio = 2,
		.init = wbi_tsk_wbuf_init,
		.handler = wbi_tsk_wbuf,
		.name = "wbuf" },
};
#define N_WBITSK ((sizeof wbi_tsks) / (sizeof wbi_tsks[0]))

/* run queue */
typedef struct {
	u32 run; /* bitmap of priority 0.. */
	wbi_queue_t q[N_WBIPRI];
} wbi_rq_t;
static wbi_rq_t wbi_rq;

static int wbi_tsk_init(void)
{
	int i, ret;
	wbi_tsk_t *t;

	/* run queue */
	wbi_rq.run = 0;
	for (i = 0; i < N_WBIPRI; i++) {
		wbi_queue_init(&wbi_rq.q[i]);
	}

	/* task table */
	for (i = 0, t = wbi_tsks; i < N_WBITSK; i++, t++) {
		t->next = NULL;
		t->state = TS_IDLE;
		if (t->prio < 0  ||  t->prio >= N_WBIPRI) {
			wb_printk("ERROR:%s:%d:prio=%d\n", __FUNCTION__,
			       i, t->prio);
			return -1;
		}
		if (!t->handler) {
			wb_printk("ERROR:%s:%d:handler=NULL\n", __FUNCTION__,
			       i);
			return -1;
		}
		if (!t->name) {
			wb_printk("ERROR:%s:%d:name=NULL\n", __FUNCTION__,
			       i);
			return -1;
		}
		if (t->init) {
			ret = (*t->init)(t);
			if (ret < 0) {
				return -1;
			}
		}
	}

	return 0;
}

#ifdef WBI_EXEC_DEBUG
static void wbi_rq_dump(void)
{
	wbi_rq_t *rq = &wbi_rq;
	wbi_queue_t *q;
	wbi_tsk_t *t;
	int i;

	wb_printk("run=0x%x\n", rq->run);
	for (i = 0, q = rq->q; i < N_WBIPRI; i++, q++) {
		wb_printk("%d:", i);
		t = (wbi_tsk_t *)wbi_queue_peek(q);
		while (t) {
			wb_printk(" %s", t->name);
			t = t->next;
		}
		wb_printk("\n");
	}
}
#endif

static void wbi_rq_enqueue(wbi_tsk_t *t)
{
	wbi_rq_t *rq = &wbi_rq;

	t->state = TS_RUN;
	wbi_enqueue(&rq->q[t->prio], (wbi_queue_elem_t *)t);
	rq->run |= 1 << t->prio;
}

static wbi_tsk_t *wbi_rq_dequeue(wbi_queue_t *q)
{
	wbi_rq_t *rq = &wbi_rq;
	wbi_tsk_t *t;
	uint prio;

	t = (wbi_tsk_t *)wbi_dequeue(q);
	if (!t)
		return NULL;
	if (!wbi_queue_peek(q)) {
		prio = q - rq->q;
		rq->run &= ~(1 << prio);
	}
	t->state = TS_IDLE;
	return t;
}

static inline void wbi_tsk_sched(wbi_tsk_t *t, int state)
{
	if (TS_RUN == state) {
		if (TS_RUN != t->state) {
			wbi_rq_enqueue(t);
		}
	} else {
		t->state = state;
	}
}

static void wbi_wake_tsk(wbi_tsk_t *t)
{
	wbi_tsk_sched(t, TS_RUN);
}

static inline void __wbi_wake_tid(int tid)
{
	wbi_tsk_sched(&wbi_tsks[tid], TS_RUN);
}
extern void __bad_wbi_wake_tid(void); /* for compiletime error detection */
#define wbi_wake_tid(x) \
	(__builtin_constant_p(x) && (x) >= 0 && (x) < N_WBITSK) \
	? __wbi_wake_tid(x) : __bad_wbi_wake_tid()

static void wbi_cmpr_cancel(void);

static int wbi_exec(void)
{
	wbi_rq_t *rq = &wbi_rq;
	wbi_tsk_t *t;
	int prio, ret;
#ifdef WBI_EXEC_DEBUG
	wbi_tsk_t *t_prev = NULL;
	int ret_prev = 0, cnt = 0;
#endif
	WBISTAT_DECL;

	WBISTAT_START;
	while ((prio = __ffs(rq->run)) >= 0) {
		if (wbi_is_canceled() ||  wbi_is_timeout()) {
			wbi_cmpr_cancel();
			return -ECANCELED;
		}
		t = wbi_rq_dequeue(&rq->q[prio]);
		if (!t) {
			wb_printk("ERROR:%s:empty(prio=%d)\n",__FUNCTION__,prio);
			return -1;
		}
		/* execute */
		ret = (*t->handler)(t);
		watchdog_touch();
		if (ret < 0)
			goto err1;
		wbi_tsk_sched(t, ret);
#ifdef WBI_EXEC_DEBUG
		if (t != t_prev || ret != ret_prev) {
			if (cnt > 0) {
				printascii("wbi_exec: (");
				printhex8(cnt);
				printascii(" times)\n");
			}
			printascii("wbi_exec:");
			printascii(t->name);
			printascii(":");
			printhex8(ret);
			printascii("\n");
			t_prev = t;
			ret_prev = ret;
			cnt = 0;
		} else {
			cnt++;
		}
#endif
	}
	WBISTAT_STOP(WBISTAT_EXEC);
	return 0;

 err1:
	wb_printk("ERROR:%s:%s: %d\n", __FUNCTION__, t->name, ret);
	return ret;
}

/*-----------------------  resources ----------------------*/
#define RID_CMPR	0 /* compressor */
#define RID_OBUF	1 /* output buffer of compressor */
#define RID_WBUF	2 /* write buffer for storage */

#define MAX_CMPR	3
#define MAX_OBUF	(MAX_CMPR * 4)
#define N_WBUF		1  /* should be 1 */
#if (MAX_CMPR > BITS_PER_LONG) || (MAX_OBUF > BITS_PER_LONG)
#error "MAX_CMPR or MAX_OBUF > BITS_PER_LONG"
#endif

#define WBI_RES_STAT

typedef struct {
	wbi_queue_t q;
	int n;   /* number of resources */
	u32 busy; /* bitmask */
#ifdef WBI_RES_STAT
	u8 cnt[BITS_PER_LONG];
#endif
} wbi_resource_t;
static wbi_resource_t wbi_res[] = {
	[RID_CMPR] = {
		.n = 1, /* overwritten by wbi_setup */
	},
	[RID_OBUF] = {
		.n = 2, /* overwritten by wbi_setup */
	},
	[RID_WBUF] = {
		.n = N_WBUF,
	},
};
#define N_WBIRES ((sizeof wbi_res) / (sizeof wbi_res[0]))

static int wbi_res_id(wbi_resource_t *p)
{
	return p - wbi_res;
}

static void wbi_res_init(void)
{
	int n;
	wbi_resource_t *p;

	p = wbi_res;
	n = N_WBIRES;
	while (--n >= 0) {
		wbi_queue_init(&p->q);
		p->busy = 0;
#ifdef WBI_RES_STAT
		memset(p->cnt, 0, sizeof p->cnt);
#endif
		p++;
	}
}

static int wbi_res_get(wbi_resource_t *r)
{
	int i;

	if ((i = ffz(r->busy)) >= 0  &&  i < r->n) {
		r->busy |= 1 << i;
#ifdef WBI_RES_STAT
		r->cnt[i]++;
#endif
		return i;
	}
	return -1;
}

static int wbi_res_wait(wbi_resource_t *r, wbi_tsk_t *t)
{
	int ret;

	ret = wbi_res_get(r);
	if (ret < 0) {
		wbi_enqueue(&r->q, (wbi_queue_elem_t *)t);
	}
	return ret;
}

static int wbi_res_put(wbi_resource_t *r, int idx)
{
	u32 mask;
	wbi_tsk_t *t;

	if (idx < 0  ||  idx >= r->n) {
		wb_printk("ERROR:%s:%d:idx=%d\n", __FUNCTION__,
		       wbi_res_id(r), idx);
		return -1;
	}
	mask = 1 << idx;
	if (!(r->busy & mask)) {
		wb_printk("ERROR:%s:%d:idx=%d:already free\n", __FUNCTION__,
		       wbi_res_id(r), idx);
		return -1;
	}
	r->busy &= ~mask;

	while ((t = (wbi_tsk_t *)wbi_queue_peek(&r->q)) != NULL) {
		wbi_wake_tsk(t);
		wbi_dequeue(&r->q);
	}
	return 0;
}

#ifdef WBI_RES_STAT
static void wbi_res_stat(void)
{
	int i, j;
	wbi_resource_t *p;

	for (i = 0, p = wbi_res; i < N_WBIRES; i++, p++) {
		wb_printk("RES%d: n=%d,", i, p->n);
		for (j = 0; j < p->n; j++) {
			wb_printk(" %u", p->cnt[j]);
		}
		wb_printk("\n");
	}
}
#endif

static void wbi_res_dump(void)
{
	int i;
	wbi_resource_t *p;

	for (i = 0, p = wbi_res; i < N_WBIRES; i++, p++) {
		wb_printk("RES%d: n=%d, busy=0x%x,", i, p->n, p->busy);
		wbi_queue_dump(&p->q);
		wb_printk("\n");
	}
}
/*---------------------------------------------------------*/

/*
 * WBUF: write buffer
 */
typedef struct {
	wbi_queue_t q;
	ulong addr, size;
	int put, get, end;
	int cnt;
	ulong blksize;
	ulong n_sector;
	ulong sector;
#ifdef WBI_STAT
	/* statistics */
	u32 t1, t_min, t_max, t_sum, t_cnt;
	int cnt_max;
	ulong cp_hist[4];
#endif
} wbi_wbuf_t;
static wbi_wbuf_t wbi_wbuf;

/* virt addr of WBUF */
module_param_named(waddr, wbi_wbuf.addr, ulongH, S_IRUSR|S_IWUSR);
module_param_named(wsize, wbi_wbuf.size, ulongH, S_IRUSR|S_IWUSR);

static int wbi_wbuf_inc(wbi_wbuf_t *w, int n)
{
	if (w->put + n > w->end) {
		wb_printk("ERROR:%s:put=%d,n=%d\n", __FUNCTION__, w->put, n);
		return -1;
	}
	w->cnt += n;
	w->put += n;
	if (w->put == w->end) {
		w->put = 0;
	}
	/* statistics */
	if (w->cnt > w->cnt_max) {
		w->cnt_max = w->cnt;
	}
	return 0;
}

static void wbi_wbuf_dec(wbi_wbuf_t *w, int n)
{
	w->cnt -= n;
	w->get += n;
	if (w->get == w->end) {
		w->get = 0;
	}
}

static int wbi_wbuf_enqueue(ulong p, ulong size)
{
	wbi_wbuf_t *w = &wbi_wbuf;
	ulong room, r, n, total;

	room = w->end - w->cnt;
	if (!room)
		return 0;

	total = 0;
	if (w->put >= w->get) {
		r = w->end - w->put;
		n = min(size, r);
		memcpy((void *)w->addr + w->put, (void *)p, n);
		wbi_wbuf_inc(w, n);
		total += n;
		p += n;
		size -= n;
		room -= n;
	}
	if (room && size) {
		r = w->get - w->put;
		n = min(size, r);
		memcpy((void *)w->addr + w->put, (void *)p, n);
		wbi_wbuf_inc(w, n);
		total += n;
	}
	if (w->cnt >= w->blksize) {
		wbi_wake_tid(TID_WRITER);
	}
#ifdef WBI_STAT
	/* statistics */
	if (total < 128)
		w->cp_hist[0]++;
	else if (total < SZ_4K)
		w->cp_hist[1]++;
	else if (total < SZ_512K)
		w->cp_hist[2]++;
	else
		w->cp_hist[3]++;
#endif

	return total;
}

static void wbi_wbuf_align(void)
{
	wbi_wbuf_t *w = &wbi_wbuf;
	int n;

	/* round up to wb_default_device->phys_sector_size */
	n = RU_SIZE(w->put, wbheader.sector_size) - w->put;
	memset((void *)w->addr + w->put, 0, n);
	wbi_wbuf_inc(w, n);
	if (w->cnt >= w->blksize) {
		wbi_wake_tid(TID_WRITER);
	}
}

static int wbi_wbuf_flush(void)
{
	wbi_wbuf_t *w = &wbi_wbuf;
	int n, ret;

	if (!w->cnt)
		return 0;

	if (wbi_is_canceled())
		return -ECANCELED;

	n = w->blksize - w->cnt;
	if (n > 0) {
		memset((void *)w->addr + w->put, 0, n);
		if (wbi_wbuf_inc(w, n) < 0)
			return -1;
	}

	wbistamp(WBISTAMP_WR_S);
	WBISTAT_START2(w);
	ret = wb_default_device->write_sector(wb_default_device, (void *)w->addr + w->get, w->sector, w->n_sector);
	WBISTAT_STOP2(w);
	wbistamp(WBISTAMP_WR_E);
	watchdog_touch();

	w->sector += w->n_sector;
	wbi_wbuf_dec(w, w->blksize);
	return ret;
}

/* WBUF: write task */
static int wbi_tsk_wbuf_init(wbi_tsk_t *tsk)
{
	wbi_wbuf_t *w = &wbi_wbuf;

	if (!w->addr || !w->size) {
		wb_printk("ERROR:%s: addr=0x%lx, size=0x%lx\n", __FUNCTION__,
		       w->addr, w->size);
		return -1;
	}
	wbi_queue_init(&w->q);
	/* block size */
	w->blksize = wb_default_device->sector_size;
	w->n_sector = w->blksize >> SECTOR_SHIFT;
	w->sector = 0;
	if (w->size < w->blksize) {
		wb_printk("ERROR:%s: size=0x%lx, blksize=0x%lx\n",
		       __FUNCTION__, w->size, w->blksize);
		return -1;
	}
	/* ring buffer */
	w->put = w->get = 0;
	w->cnt = 0;
	/* ASSUME: blksize is power of 2. */
	w->end = w->size & ~(w->blksize - 1);
#ifdef WBI_STAT
	/* statistics */
	w->t_min = ~0;
	w->t_max = 0;
	w->t_sum = 0;
	w->t_cnt = 0;
	w->cnt_max = 0;
	memset(w->cp_hist, 0, sizeof w->cp_hist);
#endif
	return 0;
}

static int wbi_tsk_wbuf(wbi_tsk_t *tsk)
{
	wbi_wbuf_t *w = &wbi_wbuf;
	wbi_tsk_t *t;
	int ret;

	if (w->cnt < w->blksize)
		return TS_IDLE;

	wbistamp(WBISTAMP_WR_S);
	WBISTAT_START2(w);
	ret = wb_default_device->write_sector(wb_default_device, (void *)w->addr + w->get, w->sector, w->n_sector);
	WBISTAT_STOP2(w);
	wbistamp(WBISTAMP_WR_E);
	if (ret < 0) {
		wb_printk("ERROR:%s: cannot write = %d\n", __FUNCTION__, ret);
		return ret;
	}

	w->sector += w->n_sector;
	wbi_wbuf_dec(w, w->blksize);

	t = (wbi_tsk_t *)wbi_dequeue(&w->q);
	if (t) {
		wbi_wake_tsk(t);
	}

	if (w->cnt >= w->blksize)
		return TS_RUN;
#ifdef WBI_STAT
	if (TS_RUN == wbi_tsks[TID_CMPR_W].state) {
		wbi_elapse_cmprw = mach_read_cycles();
	}
#endif
	return TS_IDLE;
}

static void wbi_wbuf_wait(wbi_tsk_t *t)
{
	if (wbi_wbuf.q.head == (wbi_queue_elem_t *)t) {
		wb_printk("ERROR:%s:already in queue.\n", __FUNCTION__);
		return;
	}
	wbi_enqueue(&wbi_wbuf.q, (wbi_queue_elem_t *)t);
}

static void wbi_wbuf_dump(void)
{
	wbi_wbuf_t *w = &wbi_wbuf;

	wb_printk("wbuf: addr=0x%lx,size=0x%lx,blksize=0x%lx,n_sector=%lu,sector=%lu\n",
	       w->addr,w->size,w->blksize,w->n_sector,w->sector);
	wb_printk("      put=%d,get=%d,end=%d,cnt=%d,",
	       w->put, w->get, w->end, w->cnt);
	wbi_queue_dump(&w->q);
	wb_printk("\n");
}
/*---------------------------------------------------------*/

/*
 * send_data task
 */
typedef struct {
	char *data;
	ulong size;
	int own;
} wbi_sendreq_t;
static wbi_sendreq_t wbi_sendreq;

static int wbi_tsk_send_init(wbi_tsk_t *tsk)
{
	wbi_sendreq_t *p = &wbi_sendreq;

	p->size = 0;
	p->own = 0;
	return 0;
}

static int wbi_tsk_send(wbi_tsk_t *tsk)
{
	int w, n;
	wbi_sendreq_t *p = &wbi_sendreq;

	if (!p->size)
		return TS_IDLE;
	if (!p->own) {
		w = wbi_res_wait(&wbi_res[RID_WBUF], tsk);
		if (w < 0) {
			return TS_WAIT;
		}
		p->own = 1;
	}
	n = wbi_wbuf_enqueue((ulong)p->data, p->size);
	p->data += n;
	p->size -= n;
	if (p->size > 0) {
		wbi_wbuf_wait(tsk);
		return TS_WAIT;
	}
	if (wbi_res_put(&wbi_res[RID_WBUF], 0) < 0)
		return -1;
	p->own = 0;
	return TS_IDLE;
}

static void wbi_tsk_send_dump(void)
{
	wbi_sendreq_t *p = &wbi_sendreq;

	wb_printk("tsk_send: size=%lu, own=%d\n", p->size, p->own);
}
/*---------------------------------------------------------*/

/*
 * CMPRBUF: compressor output buffer
 */
typedef struct _wbi_cmprbuf {
	struct _wbi_cmprbuf *next;
	ulong dst, dlen;
	/* wb_section */
	int s_idx;
	ulong s_addr;
	ulong s_virt;
	ulong s_olen;
	ulong s_cksum;
	/* state */
	struct wbi_lzp_hdr *head;
	struct wbi_lzp_entry *ent, *entend;
	ulong total, virt, entrest;
	ulong __unused[2];
} wbi_cmprbuf_t;
static wbi_cmprbuf_t wbi_cmprbufs[MAX_OBUF];

static int wbi_cmprbufidx(wbi_cmprbuf_t *p)
{
	return p - wbi_cmprbufs;
}
/*---------------------------------------------------------*/

/*
 * serializer
 */
typedef struct {
	wbi_queue_t q;
	int own;
	struct wb_section *section;
} wbi_cmprout_t;
static wbi_cmprout_t wbi_cmprout;

static void wbi_cmprout_enqueue(wbi_cmprbuf_t *req)
{
	wbi_enqueue(&wbi_cmprout.q, (wbi_queue_elem_t *)req);
	if (TS_IDLE == wbi_tsks[TID_CMPR_O].state) {
		wbi_wake_tid(TID_CMPR_O);
	}
}

static int wbi_cmprout_dequeue(void)
{
	if (!wbi_dequeue(&wbi_cmprout.q)) {
		wb_printk("ERROR:%s:empty list.\n", __FUNCTION__);
		return -1;
	}
	return 0;
}
/*---------------------------------------------------------*/

/*
 * CMPROUT: send to writer
 */
static int wbi_tsk_cmprout_init(wbi_tsk_t *tsk)
{
	wbi_cmprout_t *l = &wbi_cmprout;

	wbi_queue_init(&l->q);
	l->own = 0;
	l->section = NULL;
	return 0;
}

static int wbi_tsk_cmprout(wbi_tsk_t *tsk)
{
	wbi_cmprout_t *l = &wbi_cmprout;
	wbi_cmprbuf_t *p;
	int w, n;
	WBISTAT_DECL;

	if (!wbi_queue_peek(&l->q))
		return TS_IDLE;
	if (!l->own) {
		w = wbi_res_wait(&wbi_res[RID_WBUF], tsk);
		if (w < 0) {
			return TS_WAIT;
		}
		l->own = 1;
	}

	WBISTAT_START;
	while ((p = (wbi_cmprbuf_t *)wbi_queue_peek(&l->q)) != NULL) {
		wbistamp(WBISTAMP_CO_S);
		while (p->ent < p->entend) {
			n = wbi_wbuf_enqueue(p->virt, p->entrest);
			p->virt += n;
			p->entrest -= n;
			if (p->entrest > 0) {
				WBISTAT_STOP(WBISTAT_CMPROUT);
				wbistamp(WBISTAMP_CO_W);
				wbi_wbuf_wait(tsk);
				return TS_WAIT;
			} else {
				p->total += p->ent->sz;
				p->ent++;
				p->virt = (unsigned long)p->head + (unsigned long)p->ent->off;
				p->entrest = p->ent->sz;
			}
		}
		/* section finish */
		wbi_wbuf_align();
		if (p->total) {
			l->section->rlen = RU_SECTOR_SIZE(p->total);
		} else {
			l->section->rlen = p->s_olen;
		}
		wbheader.r_data_size += l->section->rlen;
		l->section->addr = p->s_addr;
		l->section->cksum = p->s_cksum;
		l->section->flag = 0;
		l->section->olen = p->s_olen;
		l->section->virt = p->s_virt;
		INIT_CKSUM(l->section->meta_cksum);
		wbi_calc_cksum(&l->section->meta_cksum, (void *)l->section,
			   sizeof(*l->section)-sizeof(unsigned long));
		l->section++;
		if (wbi_res_put(&wbi_res[RID_OBUF], wbi_cmprbufidx(p)) < 0)
			return -1;
		wbi_cmprout_dequeue();
		wbistamp(WBISTAMP_CO_E);
	}
	if (wbi_res_put(&wbi_res[RID_WBUF], 0) < 0)
		return -1;
	l->own = 0;
	WBISTAT_STOP(WBISTAT_CMPROUT);
	return TS_IDLE;
}
/*---------------------------------------------------------*/

/*
 * CMPR: compressor
 */
typedef struct {
	int obuf;
	ulong work, wlen;
#ifdef WBI_STAT
	/* statistics */
	u32 t1, t_min, t_max, t_sum, t_cnt;
#endif
} wbi_cmpr_t;
static wbi_cmpr_t wbi_cmprs[MAX_CMPR];

/* Compressor JOB queue */
typedef struct {
	struct wb_section *section;
	int nsect;
} wbi_cmprreq_t;
static wbi_cmprreq_t wbi_cmprreq;

/* CMPR: issue compress task */
static int wbi_tsk_cmpr_init(wbi_tsk_t *tsk)
{
	wbi_cmprreq_t *req = &wbi_cmprreq;

	req->nsect = 0;
#ifdef WBI_STAT
	{ /* statistics */
		int i;
		wbi_cmpr_t *p;
		for (i = 0, p = wbi_cmprs; i < MAX_CMPR; i++, p++) {
			p->t_min = ~0;
			p->t_max = 0;
			p->t_sum = 0;
			p->t_cnt = 0;
		}
	}
#endif
	return 0;
}

static int wbi_tsk_cmpr(wbi_tsk_t *tsk)
{
	wbi_cmprreq_t *req = &wbi_cmprreq;
	int c, b, ret;
	struct wb_section *s;
	wbi_cmpr_t *cmprp;
	wbi_cmprbuf_t *obufp;
	ulong phys, len, *cksum;

	if (!req->nsect)
		return TS_IDLE;

	c = wbi_res_wait(&wbi_res[RID_CMPR], tsk);
	if (c < 0) {
		return TS_WAIT;
	}
	b = wbi_res_wait(&wbi_res[RID_OBUF], tsk);
	if (b < 0) {
		if (wbi_res_put(&wbi_res[RID_CMPR], c) < 0)
			return -1;
		return TS_WAIT;
	}
	cmprp = &wbi_cmprs[c];
	obufp = &wbi_cmprbufs[b];

	cmprp->obuf = b;
	s = req->section;
	/* copy wb_section */
	obufp->s_idx = wbi_section_idx(s);
	obufp->s_addr = s->addr;
	obufp->s_virt = s->virt;
	obufp->s_olen = s->olen;
	obufp->s_cksum = s->cksum;

	phys = s->addr;
	len = s->olen;
	cksum = &obufp->s_cksum;

	wbi_recalc_cksum(cksum, (unsigned long *)phys, len);
	wbistamp(WBISTAMP_CM_S(c));
	WBISTAT_START2(cmprp);
	ret = wbi_default_compressor->deflate(
		c, phys, len,
		obufp->dst, obufp->dlen,
		cmprp->work, cmprp->wlen);
	if (ret < 0) {
		wb_printk("section%d:deflate error: %d\n", obufp->s_idx, ret);
		return -1;
	}

	wbi_wake_tid(TID_CMPR_W);

	req->nsect--;
	req->section++;
	if (req->nsect > 0)
		return TS_RUN;
	return TS_IDLE;
}

static void wbi_tsk_cmpr_dump(void)
{
	wbi_cmprreq_t *p = &wbi_cmprreq;

	wb_printk("tsk_cmpr: nsect=%d\n", p->nsect);
}
/*---------------------------------------------------------*/

/* CMPR: output task */
static int wbi_cmprfinish(int c)
{
	int b, ret;
	wbi_cmprbuf_t *p;

	b = wbi_cmprs[c].obuf;
	p = &wbi_cmprbufs[b];
	ret = wbi_default_compressor->deflate_finish(
		c, p->dst, p->dlen);
	if (wbi_res_put(&wbi_res[RID_CMPR], c) < 0)
		return -1;
	if (ret < 0) {
		wb_printk("section%d: deflate_finish error: %d\n",
			  p->s_idx, ret);
		return ret;
	}

	/* add to output queue */
	p->head = (struct wbi_lzp_hdr *)PHYS_TO_NONCACHE(p->dst);
	p->ent = (struct wbi_lzp_entry *)((unsigned long)p->head + sizeof(struct wbi_lzp_hdr));
	p->entend = (struct wbi_lzp_entry *)((unsigned long)p->ent + p->head->tbl1_sz);
	p->virt = (unsigned long)p->head + (unsigned long)p->ent->off;
	p->entrest = p->ent->sz;
	p->total = 0;
	wbi_cmprout_enqueue(p);
	return 0;
}

static int wbi_tsk_cmprwait(wbi_tsk_t *tsk)
{
	int i, ncore, ret;
	u32 busy, mask;

	busy = wbi_res[RID_CMPR].busy;
	if (!busy)
		return TS_IDLE;

	ncore = wbi_res[RID_CMPR].n;
	for (i = 0, mask = 1; i < ncore; i++, mask <<= 1) {
		if (!(busy & mask))
			continue;
		if (wbi_default_compressor->deflate_poll(i)) {
			WBISTAT_STOP2(&wbi_cmprs[i]);
			wbistamp(WBISTAMP_CM_E(i));
			ret = wbi_cmprfinish(i);
			if (ret < 0)
				return ret;
		}
	}
	return (wbi_res[RID_CMPR].busy) ? TS_RUN:TS_IDLE;
#if 0
//#ifdef WBI_STAT
	if (wbi_elapse_cmprw > 0) {
		wbi_elapse_cmprw = mach_read_cycles() - wbi_elapse_cmprw;
		wbi_elapse[WBISTAT_CMPRWAIT] += wbi_elapse_cmprw;
		wbi_elapse_cmprw = 0;
	}
#endif
}

static void wbi_cmpr_cancel(void)
{
	int i, ncore;
	u32 busy, mask;
	ulong expire;

	busy = wbi_res[RID_CMPR].busy;
	if (!busy)
		return;

	ncore = wbi_res[RID_CMPR].n;
	/* request to cancel */
	for (i = 0, mask = 1; i < ncore; i++, mask <<= 1) {
		if (!(busy & mask))
			continue;
		wbi_default_compressor->deflate_cancel(i);
	}

	/* wait for cancel */
	expire = mach_read_cycles() + mach_usecs_to_cycles(wbi_cancel_tmo);
	while (busy) {
		if (time_after_eq(mach_read_cycles(), expire)) {
			wb_printk(KERN_ERR "WBI:%s:timeout\n", __FUNCTION__);
			break;
		}
		for (i = 0, mask = 1; i < ncore; i++, mask <<= 1) {
			if (!(busy & mask))
				continue;
			if (wbi_default_compressor->deflate_poll(i)) {
				busy &= ~mask;
			}
		}
		watchdog_touch();
	}
	/* finish */
	busy = wbi_res[RID_CMPR].busy;
	for (i = 0, mask = 1; i < ncore; i++, mask <<= 1) {
		if (!(busy & mask))
			continue;
		wbi_default_compressor->deflate_end(i);
	}
}

static void wbi_dump(void)
{
	wbi_res_dump();
	wbi_wbuf_dump();
	wbi_tsk_cmpr_dump();
	wbi_tsk_send_dump();
}

static int wbi_compress(struct wb_section *p, int n)
{
	int ret;

	/* compress and write */
	wbi_cmprreq.section = p;
	wbi_cmprreq.nsect = n;
	wbi_wake_tid(TID_CMPR_I);
	ret = wbi_exec();
	if (ret < 0) {
		wbi_dump();
		return ret;
	}
	if (wbi_cmprreq.nsect) {
		wb_printk("ERROR:%s:nsect=%d\n", __FUNCTION__, wbi_cmprreq.nsect);
		wbi_dump();
		return -1;
	}
	return ret;
}
/*---------------------------------------------------------*/

/*------------------------ API ----------------------------*/
int wbi_setup(ulong work, ulong *wlenp, ulong dst, ulong *dlenp)
{
	ulong wlen = *wlenp, dlen = *dlenp;
	int ncore, i, nbuf, ret;

	/* elapse time */
	wbistat_init();
	/* time stamp */
	wbistamp_init();

	/* set number of cores */
	ncore = wbi_default_compressor->ncore;
	if (ncore > MAX_CMPR) {
		wb_printk("WBI:ncore=%d, MAX_CMPR=%d\n", ncore, MAX_CMPR);
		ncore = MAX_CMPR;
	}
	wbi_res[RID_CMPR].n = ncore;

	/* set work area */
	wlen /= ncore;
	for (i = 0; i < ncore; i++) {
		wbi_cmprs[i].work = work;
		wbi_cmprs[i].wlen = wlen;
		work += wlen;
	}

	/* set output buffer area */
	nbuf = ncore << 1;
	if (wbi_nbuf > 0  &&  wbi_nbuf <= MAX_OBUF) { /* override */
		nbuf = wbi_nbuf;
	}
	wbi_res[RID_OBUF].n = nbuf;
	dlen /= nbuf;
	for (i = 0; i < nbuf; i++) {
		wbi_cmprbufs[i].dst = dst;
		wbi_cmprbufs[i].dlen = dlen;
		dst += dlen;
	}

	/* resource */
	wbi_res_init();

	/* task */
	ret = wbi_tsk_init();

	*wlenp = wlen;
	*dlenp = dlen;
	return ret;
}

int wbi_send_data(void *b, ulong len)
{
	int ret;

	wbi_sendreq.data = b;
	wbi_sendreq.size = len;
	wbi_wake_tid(TID_SEND);
	ret = wbi_exec();
	if (ret < 0)
		return ret;
	if (wbi_sendreq.size) {
		wb_printk("ERROR:%s:size=%lu\n", __FUNCTION__, wbi_sendreq.size);
		wbi_dump();
		return -1;
	}
	return ret;
}

void wbi_align_sector(void)
{
	wbi_wbuf_align();
}

int wbi_flush(void)
{
	return wbi_wbuf_flush();
}

int wbi_send_sections(struct wb_header *header, struct wb_section *s, ulong n)
{
	int ret, i;
	struct wb_section *p;
	WBISTAT_DECL;

	/*send section data*/
	header->r_data_size = 0;

	/* section output pointer */
	wbi_cmprout.section = s;

	/* clean all Caches */
	WBISTAT_START;
	flush_cache_all();  /* L1 D-Cache */
#ifdef CONFIG_OUTER_CACHE
#ifdef CONFIG_EJ_FLUSH_ENTIRE_L2CACHE
	outer_clean_all_unlocked(); /* L2 Cache */
#endif
#endif
	WBISTAT_STOP(WBISTAT_FLUSH);

	/* compress and write */
	ret = wbi_compress(s, n);
	if (ret) {
		goto err1;
	}

	for (p = s, i = 0; p < wbi_cmprout.section; p++, i++) {
		wb_printk("WBI: %3d %08lx:%08lx:%08lx %08lx<->%08lx\n",
			  i, p->addr, p->cksum, p->flag,
			  p->olen, p->rlen);
		watchdog_touch();
	}

	/* write meta data */
	ret = wbi_send_data(s, sizeof(*s) * n);
	if (ret) {
		goto err2;
	}
	wbi_wbuf_align();

	return 0;

 err1:
	wb_printk("%s: compress error: %d\n", __FUNCTION__, ret);
	return ret;
 err2:
	wb_printk("%s: write meta data error: %d\n", __FUNCTION__, ret);
	return ret;
}

int wbi_rewrite_header(struct wb_header *header)
{
	wbi_wbuf_t *w = &wbi_wbuf;
	int ret;

	/* ASSUME: header size <= sector_size */
	memset((void *)w->addr, 0, header->sector_size);
	memcpy((void *)w->addr, header, sizeof (*header));

	wbistamp(WBISTAMP_WR_S);
	ret = wb_default_device->write_sector(wb_default_device,
					      (void *)w->addr, 0, 1);
	wbistamp(WBISTAMP_WR_E);
	watchdog_touch();
	return ret;
}

int wbi_read_header(struct wb_header *header)
{
	wbi_wbuf_t *w = &wbi_wbuf;
	int ret;

	wbistamp(WBISTAMP_RD_S);
	ret = wb_default_device->read_sector(wb_default_device,
					      (void *)w->addr, 0, 1);
	wbistamp(WBISTAMP_RD_E);
	watchdog_touch();

	/* ASSUME: header size <= sector_size */
	memcpy((void *)header, (void *)w->addr, sizeof (*header));

	return ret;
}

void wbi_stat(void)
{
	int i;
#ifdef WBI_STAT
	wbi_wbuf_t *w = &wbi_wbuf;
	wbi_cmpr_t *c;

	for (i = 0, c = wbi_cmprs; i < MAX_CMPR; i++, c++) {
		wb_printk("cmpr%d: %d times, %d[us] (%d,%d)\n", i, c->t_cnt,
		       mach_cycles_to_usecs(c->t_sum),
		       mach_cycles_to_usecs(c->t_min),
		       mach_cycles_to_usecs(c->t_max));
	}
	wb_printk("write: %d times, %d[us] (%d,%d), %d\n", w->t_cnt,
	       mach_cycles_to_usecs(w->t_sum),
	       mach_cycles_to_usecs(w->t_min),
	       mach_cycles_to_usecs(w->t_max),
	       w->cnt_max);
	wb_printk("memcpy: %lu %lu %lu %lu\n", w->cp_hist[0],
	       w->cp_hist[1], w->cp_hist[2], w->cp_hist[3]);
	wb_printk("total:");
	for (i = 0; i < N_WBISTAT; i++) {
		wb_printk("%d ", mach_cycles_to_usecs(wbi_elapse[i]));
	}
	wb_printk("[us]\n");
#endif /* WBI_STAT */
#ifdef WBI_RES_STAT
	wbi_res_stat();
#endif

	if (wbi_stamp) {
		u32 t0 = wbi_stamp[0].t;

		for (i = 0; i < wbi_stampi; i++) {
			wb_printk("%s: %d\n", wbistamp_mnemonic[wbi_stamp[i].id],
			       mach_cycles_to_usecs(wbi_stamp[i].t - t0));
		}
		if (wbi_stampi == wbi_stamp_end) {
			wb_printk("WBISTAMP overflow.\n");
		} else {
			wb_printk("WBISTAMP finish.\n");
		}
	}
}
