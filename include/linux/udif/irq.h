/*
 * include/linux/udif/irq.h
 *
 *
 * Copyright 2012 Sony Corporation
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
#ifndef __UDIF_IRQ_H__
#define __UDIF_IRQ_H__

typedef struct UDIF_INTR {
	UDIF_IRQ	irq;
	UDIF_VP		data;
} UDIF_INTR;

typedef	UDIF_ERR (*UDIF_INT_CB)(UDIF_INTR *);

typedef struct UDIF_INTRPT {
	UDIF_U8		name[16];
	UDIF_IRQ	irq;
	UDIF_U32	flags;
	UDIF_INT_CB	hndl;
	UDIF_VP		data;
} UDIF_INTRPT;

UDIF_ERR __udif_request_irq(UDIF_INTRPT *, UDIF_INT_CB, UDIF_VP);
UDIF_ERR __udif_free_irq(UDIF_INTRPT *);
UDIF_ERR __udif_enable_irq(UDIF_IRQ);
UDIF_ERR __udif_disable_irq(UDIF_IRQ);
UDIF_ERR __udif_disable_irq_nosync(UDIF_IRQ);

#define udif_intr_irq(x)	((x)?(x)->irq:0)
#define udif_intr_flags(x)	((x)?(x)->flags:(-1UL))
#define udif_intr_data(x)	((x)?(x)->data:0)

#define __udif_clearirq(x)	gic_clearirq((x)->irq)
#define __udif_maskirq(x,e)	gic_maskirq((x)->irq,e)
#define __udif_pollirq(x)	gic_pollirq((x)->irq)

#endif /* __UDIF_IRQ_H__ */
