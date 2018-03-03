/*
 * include/linux/udif/boottime.h
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
#ifndef __UDIF_BOOT_TIME_H__
#define __UDIF_BOOT_TIME_H__

#ifdef CONFIG_SNSC_BOOT_TIME
#include <linux/snsc_boot_time.h>
#endif
#include <linux/kernel.h>

#define MAX_COMMENT 64

#define UDIF_TAGID_BEGIN	0
#define UDIF_TAGID_END		1

#ifdef CONFIG_SNSC_BOOT_TIME
static inline void udif_timetag(unsigned short sid, unsigned short tagid, const char *tag)
{
	char buf[MAX_COMMENT];

	scnprintf(buf, sizeof(buf),
		  "%04X %s %s", sid, tagid == UDIF_TAGID_BEGIN ? "B" : "E", tag);

	BOOT_TIME_ADD1(buf);
}
#else
static inline void udif_timetag(unsigned short sid, unsigned short tagid, const char *tag){}
#endif

#endif /* __UDIF_BOOT_TIME_H__ */
