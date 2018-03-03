/* 2012-02-07: File added and changed by Sony Corporation */
/*
 *  Snapshot Boot Core - internal header
 *
 *  Copyright 2008,2009,2010 Sony Corporation
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
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _SSBOOT_INTERNAL_H
#define _SSBOOT_INTERNAL_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/bootmem.h>
#include <linux/list.h>
#include <linux/ssboot.h>

/*
 * Internal definitions
 */
/* Limitted Path Length */
#define SSBOOT_PATH_MAX		1024

/* extra free pages to keep */
#define SSBOOT_EXTRA_FREE_PAGES		(2 * SSBOOT_PG_1MB)

/* page bitmap */
typedef struct ssboot_pgbmp_entry {
	struct list_head list;
	unsigned long    start_pfn;
	unsigned long    num_pages;
	unsigned long    *bitmap;
} ssboot_pgbmp_entry_t;

typedef struct ssboot_pgbmp {
	struct list_head     bmp_list;
	ssboot_pgbmp_entry_t *curr_entry;
	unsigned long        curr_pfn;
	unsigned long        num_set;
} ssboot_pgbmp_t;

/* extra region */
#define SSBOOT_EXREG_NUM_STATIC		32

#define SSBOOT_EXREG_TYPE(attr)		((attr) & 0x0f)
#define SSBOOT_EXREG_NORMAL		(1 << 0)
#define SSBOOT_EXREG_CRITICAL		(1 << 1)
#define SSBOOT_EXREG_DISCARD		(1 << 2)
#define SSBOOT_EXREG_WORK		(1 << 3)

#define SSBOOT_EXREG_OWNER(attr)	((attr) & 0xf0)
#define SSBOOT_EXREG_KERNEL		(1 << 4)
#define SSBOOT_EXREG_USER		(1 << 5)

/* page statistics */
typedef enum ssboot_pgstat_item {
	SSBOOT_PGSTAT_KERNEL,
	SSBOOT_PGSTAT_KERNEL_RO,
	SSBOOT_PGSTAT_MODULE_RO,
	SSBOOT_PGSTAT_USER,
	SSBOOT_PGSTAT_ANON,
	SSBOOT_PGSTAT_CACHE,
	SSBOOT_PGSTAT_SWAPCACHE,
	SSBOOT_PGSTAT_RESERVED,
	SSBOOT_PGSTAT_OTHER,
	SSBOOT_PGSTAT_NUM
} ssboot_pgstat_item_t;

/*
 * Global settings
 */
/* memory map */
#define SSBOOT_MEMMAP_NUM		32

typedef struct ssboot_memmap_t {
	int num_region;
	struct {
		u_int64_t     phys_addr;
		unsigned long virt_addr;
		size_t        len;
	} region[SSBOOT_MEMMAP_NUM];
} ssboot_memmap_t;

/* image creation mode */
typedef enum ssboot_imgmode {
	SSBOOT_IMGMODE_NULL,
	SSBOOT_IMGMODE_MIN,
	SSBOOT_IMGMODE_MAX,
	SSBOOT_IMGMODE_OPTIMIZE,
	SSBOOT_IMGMODE_NUM
} ssboot_imgmode_t;

/* operation mode */
typedef enum ssboot_opmode {
	SSBOOT_OPMODE_NORMAL,
	SSBOOT_OPMODE_SHUTDOWN,
	SSBOOT_OPMODE_REBOOT,
	SSBOOT_OPMODE_NUM
} ssboot_opmode_t;

/* resume mode */
typedef enum ssboot_resmode {
	SSBOOT_RESMODE_NORMAL,
	SSBOOT_RESMODE_PROFILE,
	SSBOOT_RESMODE_REWRITE,
	SSBOOT_RESMODE_NUM
} ssboot_resmode_t;

/* core state */
typedef enum ssboot_state {
	SSBOOT_STATE_IDLE,
	SSBOOT_STATE_PREPARE,
	SSBOOT_STATE_SNAPSHOT,
	SSBOOT_STATE_WRITING,
	SSBOOT_STATE_RESUMED,
	SSBOOT_STATE_ERROR,
	SSBOOT_STATE_NUM
} ssboot_state_t;

/* current settings */
typedef struct ssboot_core {
	ssboot_memmap_t  memmap;
	ssboot_imgmode_t imgmode;
	ssboot_opmode_t  opmode;
	ssboot_resmode_t resmode;
	ssboot_state_t   state;
	ssboot_image_t   image;
	struct list_head writers;
	ssboot_optimizer_t
			 *optimizer;
} ssboot_core_t;

extern ssboot_core_t ssboot;

/*
 * Function prototypes
 */
/* image.c */
int ssboot_alloc_page_bitmap(void);
int ssboot_free_page_bitmap(void);
int ssboot_create_page_bitmap(void);
int ssboot_copy_critical_pages(void);
void ssboot_free_copied_pages(void);
int ssboot_wait_io_completion(void);
void ssboot_lock_page_cache(void);
void ssboot_unlock_page_cache(void);
int ssboot_create_section_list(void);
void ssboot_free_section_list(void);
int ssboot_shrink_image(void);
void ssboot_show_image_info(void);
void ssboot_set_swapfile(const char *filename);
const char* ssboot_get_swapfile(void);
int ssboot_swapoff(void);
int ssboot_swapon(void);
int ssboot_swap_set_ro(void);
int ssboot_optimizer_is_available(void);
int ssboot_optimizer_optimize(void);
int ssboot_optimizer_start_profiling(void);
int ssboot_optimizer_stop_profiling(void);

/* pgbmp.c */
void ssboot_pgbmp_init(ssboot_pgbmp_t *pgbmp);
ssboot_pgbmp_t* ssboot_pgbmp_alloc(ssboot_memmap_t *memmap);
void ssboot_pgbmp_free(ssboot_pgbmp_t *pgbmp);
void ssboot_pgbmp_set(ssboot_pgbmp_t *pgbmp, unsigned long pfn);
void ssboot_pgbmp_clear(ssboot_pgbmp_t *pgbmp, unsigned long pfn);
int ssboot_pgbmp_test(ssboot_pgbmp_t *pgbmp, unsigned long pfn);
void ssboot_pgbmp_set_region(ssboot_pgbmp_t *pgbmp,
			     unsigned long pfn, unsigned long num);
void ssboot_pgbmp_clear_region(ssboot_pgbmp_t *pgbmp,
			       unsigned long pfn, unsigned long num);
void ssboot_pgbmp_find_first(ssboot_pgbmp_t *pgbmp, unsigned long *pfn);
void ssboot_pgbmp_find_next(ssboot_pgbmp_t *pgbmp, unsigned long *pfn);
unsigned long ssboot_pgbmp_num_set(ssboot_pgbmp_t *pgbmp);
unsigned long ssboot_pgbmp_size(ssboot_pgbmp_t *pgbmp);

/* exreg.c */
int ssboot_exreg_register(unsigned long attr,
			  unsigned long pfn, unsigned long num);
int ssboot_exreg_unregister(unsigned long attr,
			    unsigned long pfn, unsigned long num);
unsigned long ssboot_exreg_find_first(unsigned long attr,
				      unsigned long *pfn, unsigned long *num);
unsigned long ssboot_exreg_find_next(unsigned long attr,
				     unsigned long *pfn, unsigned long *num);

/* memmap.c */
int __init ssboot_memmap_init(void);

/* pgstat.c */
#ifdef CONFIG_SNSC_SSBOOT_PAGE_STAT
void ssboot_pgstat_init(void);
void ssboot_pgstat_inc(ssboot_pgstat_item_t item);
void ssboot_pgstat_dec(ssboot_pgstat_item_t item);
void ssboot_pgstat_show(void);
#else
#define ssboot_pgstat_init()		do { } while (0)
#define ssboot_pgstat_inc(item)		do { } while (0)
#define ssboot_pgstat_dec(item)		do { } while (0)
#define ssboot_pgstat_show()		do { } while (0)
#endif

#endif /* _SSBOOT_INTERNAL_H */
