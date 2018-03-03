/*
 * cxd90014_cpuid.h
 *
 * CPUID definitions
 *
 * Copyright 2012,2013 Sony Corporation
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

#ifndef __PLAT_CXD90014_INCLUDE_MACH_CXD90014_CPUID_H
#define __PLAT_CXD90014_INCLUDE_MACH_CXD90014_CPUID_H

#define CXD90014_CPUID_UNDEF	0xffffffffUL

#ifndef __ASSEMBLY__
extern unsigned long cxd90014_cpuid;

#define IS_CXD90014_CPUID_UNDEF() (CXD90014_CPUID_UNDEF == cxd90014_cpuid)

extern int cxd90014_pcie_exists(void);
extern void cxd90014_update_cpuid(void);
extern void cxd90014_read_cpuid(void);
#endif /* !__ASSEMBLY__ */

#endif /* __PLAT_CXD90014_INCLUDE_MACH_CXD90014_CPUID_H */
