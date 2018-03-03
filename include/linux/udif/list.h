/*
 * include/linux/udif/list.h
 *
 *
 * Copyright 2010 Sony Corporation
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
#ifndef __UDIF_LIST_H__
#define __UDIF_LIST_H__

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/prefetch.h>
#include <linux/udif/types.h>
#include <linux/udif/macros.h>

typedef struct UDIF_LIST {
	struct list_head list;
} UDIF_LIST;

#define UDIF_LIST_HEAD_INIT(name) { LIST_HEAD_INIT((name).list) }
#define UDIF_LIST_HEAD(name) \
	UDIF_LIST name = UDIF_LIST_HEAD_INIT(name)

#define udif_list_head_init(l)	UDIF_PARM_CHK_FN(l, INIT_LIST_HEAD, &(l)->list)
#define udif_list_add(l,h)	UDIF_PARM_CHK_FN(l && h, list_add, &(l)->list, &(h)->list)
#define udif_list_add_tail(l,h)	UDIF_PARM_CHK_FN(l && h, list_add_tail, &(l)->list, &(h)->list)
#define udif_list_del(l)	UDIF_PARM_CHK_FN(l, list_del, &(l)->list)
#define udif_list_empty(l)	((l) ? list_empty(&(l)->list) : -1)

#define __udif_list_entry(pos, type, member)		\
	list_entry(list_entry(pos, UDIF_LIST, list), type, member)

#define udif_list_entry(pos, type, member)	list_entry(pos, type, member)

#define udif_list_entry_next(pos, type, member)		\
	__udif_list_entry(*(&(pos)->list.next), type, member)

#define udif_list_entry_prev(pos, type, member)		\
	__udif_list_entry(*(&(pos)->list.prev), type, member)


#define udif_list_for_each_entry(pos, head, member)			\
	for (pos = udif_list_entry_next(head, typeof(*pos), member);	\
	     prefetch(&pos->member.list.next), &pos->member != (head); 	\
	     pos = udif_list_entry_next(&pos->member, typeof(*pos), member))

#define udif_list_for_each_entry_safe(pos, n, head, member)		\
	for (pos = udif_list_entry_next(head, typeof(*pos), member),	\
	       n = udif_list_entry_next(&pos->member, typeof(*pos), member);\
	     &pos->member != (head); 					\
	     pos = n,							\
	     n = udif_list_entry_next(&n->member, typeof(*n), member))

#define udif_list_for_each_entry_reverse(pos, head, member)		\
	for (pos = udif_list_entry_prev(head, typeof(*pos), member);	\
	     prefetch(&pos->member.list.prev), &pos->member != (head); 	\
	     pos = udif_list_entry_prev(&pos->member, typeof(*pos), member))

#define udif_list_for_each_entry_safe_reverse(pos, n, head, member)	\
	for (pos = udif_list_entry_prev(head, typeof(*pos), member),	\
	       n = udif_list_entry_prev(&pos->member, typeof(*pos), member);\
	     &pos->member != (head); 					\
	     pos = n,							\
	     n = udif_list_entry_prev(&n->member, typeof(*n), member))

#endif /* __UDIF_LIST_H__ */
