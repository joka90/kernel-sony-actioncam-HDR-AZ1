/* 2011-03-01: File added and changed by Sony Corporation */
/*
 *  Sony NSC NBLArgs
 *
 *  Copyright 2001-2005 Sony Corporation.
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
 *  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifndef __SNSC_NBLARGS_H__
#define __SNSC_NBLARGS_H__

#include <linux/types.h>

#ifdef __KERNEL__

#ifdef __mips__
#include <asm/nblargs.h>
#endif
#ifdef __powerpc__
#include <asm/nblargs.h>
#endif
#ifdef __arm__
#include <asm/nblargs.h>
#endif

#ifdef CONFIG_SNSC_NBLARGS_KEY_LEN
#  define NBLARGS_KEY_LEN   CONFIG_SNSC_NBLARGS_KEY_LEN
#else
#  define NBLARGS_KEY_LEN   32
#endif

#ifdef CONFIG_SNSC_NBLARGS_PARAM_LEN
#  define NBLARGS_PARAM_LEN CONFIG_SNSC_NBLARGS_PARAM_LEN
#else
#  define NBLARGS_PARAM_LEN 32
#endif

struct nblargs_entry {
	char      key[NBLARGS_KEY_LEN];
	u_int32_t keylen;	/* if 0 then this whole entry is invalid */
	u_int32_t addr;
	u_int32_t size;
	char      param[NBLARGS_PARAM_LEN];
	u_int32_t paramlen;	/* if 0 then param is invalid */
};

#define NBLARGS_MAGIC 0x4e626c41 /* "NblA" */

struct nblargs {
	u_int32_t magic;
	struct nblargs_entry *pHead;
	struct nblargs_entry *pNext;
	struct nblargs_entry *pMax;
	struct nblargs_entry first_entry;
};



#ifdef CONFIG_SNSC_NBLARGS_NUM_TABLE
#  define NBLARGS_NUM_TABLE CONFIG_SNSC_NBLARGS_NUM_TABLE
#else
#  define NBLARGS_NUM_TABLE 50
#endif

int nblargs_init(void);
int nblargs_get_key(char *key, struct nblargs_entry *na);
void nblargs_free_key(char *key);
int nblargs_add(struct nblargs_entry *e, int cnt);

#endif	/* __KERNEL__ */
#endif /* __SNSC_NBLARGS_H__ */
