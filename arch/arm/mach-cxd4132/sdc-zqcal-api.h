/*
 * arch/arm/mach-cxd4132/sdc-zqcal-api.h
 *
 * SDC ZQ calibration API
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

#ifndef __ARCH_CXD4132_SDC_ZQCAL_API_H__
#define __ARCH_CXD4132_SDC_ZQCAL_API_H__

#include <linux/udif/macros.h>
#include <linux/udif/proc.h>

extern void sdc_zqcal_init(void);
extern void sdc_zqcal_register(struct sdc_dev_t *dev);
extern void sdc_zqcal_unregister(struct sdc_dev_t *dev);
#ifdef CONFIG_PM
extern void sdc_zqcal_suspend(struct sdc_dev_t *dev);
extern void sdc_zqcal_resume(struct sdc_dev_t *dev);
#endif /* CONFIG_PM */
#ifdef CONFIG_PROC_FS
extern UDIF_SIZE sdc_zqcal_proc_read(UDIF_PROC_READ *proc, UDIF_SIZE off,
				     struct sdc_dev_t *dev);
#endif /* CONFIG_PROC_FS */

#endif /* __ARCH_CXD4132_SDC_ZQCAL_API_H__ */
