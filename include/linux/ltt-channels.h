/* 2013-08-20: File added by Sony Corporation */
#ifndef _LTT_CHANNELS_H
#define _LTT_CHANNELS_H

/*
 * Copyright (C) 2008 Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Dynamic tracer channel allocation.

 * Dual LGPL v2.1/GPL v2 license.
 */

#include <linux/limits.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/ltt-core.h>

#define EVENTS_PER_CHANNEL	65536

/*
 * Forward declaration of locking-specific per-cpu buffer structure.
 */
struct ltt_chanbuf;
struct ltt_trace;
struct ltt_serialize_closure;
struct ltt_probe_private_data;

/* Serialization callback '%k' */
typedef size_t (*ltt_serialize_cb)(struct ltt_chanbuf *buf, size_t buf_offset,
				   struct ltt_serialize_closure *closure,
				   void *serialize_private,
				   unsigned int stack_pos_ctx,
				   int *largest_align,
				   const char *fmt, va_list *args);

struct ltt_probe_private_data {
	struct ltt_trace *trace;	/*
					 * Target trace, for metadata
					 * or statedump.
					 */
	ltt_serialize_cb serializer;	/*
					 * Serialization function override.
					 */
	void *serialize_private;	/*
					 * Private data for serialization
					 * functions.
					 */
};

struct ltt_chan_alloc {
	unsigned long buf_size;		/* Size of the buffer */
	unsigned long sb_size;		/* Sub-buffer size */
	unsigned int sb_size_order;	/* Order of sub-buffer size */
	unsigned int n_sb_order;	/* Number of sub-buffers per buffer */
	int extra_reader_sb:1;		/* Bool: has extra reader subbuffer */
	struct ltt_chanbuf *buf;	/* Channel per-cpu buffers */

	struct kref kref;		/* Reference count */
	unsigned long n_sb;		/* Number of sub-buffers */
	struct dentry *parent;		/* Associated parent dentry */
	struct dentry *ascii_dentry;	/* Text output dentry */
	struct ltt_trace *trace;	/* Associated trace */
	char filename[NAME_MAX];	/* Filename for channel files */
};

struct ltt_chan {
	struct ltt_chan_alloc a;		/* Parent. First field. */
	unsigned int overwrite:1;
	unsigned int active:1;
	unsigned long commit_count_mask;	/*
						 * Commit count mask, removing
						 * the MSBs corresponding to
						 * bits used to represent the
						 * subbuffer index.
						 */
	unsigned long switch_timer_interval;
};

struct ltt_channel_setting {
	unsigned int sb_size;
	unsigned int n_sb;
	struct kref kref;	/* Number of references to structure content */
	struct list_head list;
	unsigned int index;	/* index of channel in trace channel array */
	u16 free_event_id;	/* Next event ID to allocate */
	char name[PATH_MAX];
};

int ltt_channels_register(const char *name);
int ltt_channels_unregister(const char *name, int compacting);
int ltt_channels_set_default(const char *name,
			     unsigned int subbuf_size,
			     unsigned int subbuf_cnt);
const char *ltt_channels_get_name_from_index(unsigned int index);
int ltt_channels_get_index_from_name(const char *name);
int ltt_channels_trace_ref(void);
struct ltt_chan *ltt_channels_trace_alloc(unsigned int *nr_channels,
					  unsigned int overwrite,
					  unsigned int active);
void ltt_channels_trace_free(struct ltt_chan *channels,
			     unsigned int nr_channels);
void ltt_channels_trace_set_timer(struct ltt_chan *chan,
				  unsigned long interval);

int _ltt_channels_get_event_id(const char *channel, const char *name);
int ltt_channels_get_event_id(const char *channel, const char *name);
void _ltt_channels_reset_event_ids(void);

#endif /* _LTT_CHANNELS_H */
