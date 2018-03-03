/* 2013-02-21: File added by Sony Corporation */
/*
 *  pmmem feature
 *
 *  Copyright 2013 Sony Corporation
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
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/snsc_pmmem.h>
#include <linux/snsc_nblargs.h>
#ifdef CONFIG_SNSC_SSBOOT
#include <linux/ssboot.h>
#endif

#ifndef CONFIG_SNSC_PMMEM_MAX
#define CONFIG_SNSC_PMMEM_MAX	16
#endif

#define PMMEM_NBLARGS_KEY	"pmmem"

/*
 * Free pmmem reserve region
 */
void pmmem_free(void)
{
#ifdef CONFIG_SNSC_PMMEM_USE_NBLARGS
	struct nblargs_entry na;
	char key[NBLARGS_KEY_LEN];
	int i;

	for(i = 0; i < CONFIG_SNSC_PMMEM_MAX; i++) {
		snprintf(key, NBLARGS_KEY_LEN,"%s%u", PMMEM_NBLARGS_KEY, i);
		if (nblargs_get_key(key, &na) < 0){
			return;
		}

		nblargs_free_key(key);

#ifdef CONFIG_SNSC_SSBOOT
		if (ssboot_region_unregister(nbl_to_pa(na.addr), na.size) < 0){
			panic("pmmem_free: failed to unregister ssboot discard region.\n");
		}
#endif
	}
#endif
	return;
}

void __init pmmem_init(void)
{
#ifdef CONFIG_SNSC_PMMEM_USE_NBLARGS
	struct nblargs_entry na;
	char key[NBLARGS_KEY_LEN];
	int i;

	for(i = 0; i < CONFIG_SNSC_PMMEM_MAX; i++) {
		snprintf(key, NBLARGS_KEY_LEN,"%s%u", PMMEM_NBLARGS_KEY, i);
		if (nblargs_get_key(key, &na) < 0) {
			return;
		}

#ifdef CONFIG_SNSC_SSBOOT
		if (ssboot_region_register_discard(nbl_to_pa(na.addr), na.size) < 0){
			panic("pmmem_init: failed to register ssboot discard region.\n");
		}
#endif
	}
#endif
	return;
}


