/*
 * arch/arm/mach-cxd90014/include/mach/clock.h
 *
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
 */
#ifndef __UDIF_CLOCK_H__
#define __UDIF_CLOCK_H__

typedef struct UDIF_CLK_CTL {
	UDIF_VA		set;
	UDIF_VA		clr;
	UDIF_U32	shift;
} UDIF_CLK_CTL;

typedef struct UDIF_CLOCK {
	UDIF_CLK_CTL	hclk;
	UDIF_CLK_CTL	dclk;
	UDIF_CLK_CTL	dclksel;
} UDIF_CLOCK;

#define UDIF_CLK_CTL_INIT(s,c,sh) \
{ \
	.set	= UDIF_IO_ADDRESS(s), \
	.clr	= UDIF_IO_ADDRESS(c), \
	.shift	= sh, \
}

#define UDIF_CLOCK_INIT(hs, hc, hsh, ds, dc, dsh, dss, dsc, dssh) \
{ \
	.hclk	 = UDIF_CLK_CTL_INIT(hs, hc, hsh), \
	.dclk	 = UDIF_CLK_CTL_INIT(ds, dc, dsh), \
	.dclksel = UDIF_CLK_CTL_INIT(dss, dsc, dssh), \
}

UDIF_ERR __udif_devio_hclk(UDIF_CLOCK *, UDIF_U8);
UDIF_ERR __udif_devio_devclk(UDIF_CLOCK *, UDIF_U8, UDIF_U8);

typedef enum {
	UDIF_CLK_ID_CRG11_00 = 0,
	UDIF_CLK_ID_CRG11_01,
	UDIF_CLK_ID_CRG11_10,
	UDIF_CLK_ID_CRG11_11,
	UDIF_CLK_ID_CRG11_20,
	UDIF_CLK_ID_CRG11_21,
	UDIF_CLK_ID_CRG11_3,
	UDIF_CLK_ID_CRG11_40,
	UDIF_CLK_ID_CRG11_41,
	UDIF_CLK_ID_NUM,
} UDIF_CLK_ID;

typedef	UDIF_ERR (*UDIF_CLK_CB)(UDIF_CLK_ID id, UDIF_VA vbase, UDIF_VP data);

UDIF_ERR udif_change_clk(UDIF_CLK_ID id, UDIF_CLK_CB cb, UDIF_VP data);

#endif /* __UDIF_CLOCK_H__ */
