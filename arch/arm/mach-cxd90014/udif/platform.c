/*
 * arch/arm/mach-cxd90014/udif/platform.c
 *
 *
 * Copyright 2012 Sony Corporation
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/udif/device.h>
#include <linux/udif/string.h>
#include <linux/udif/macros.h>
#include <mach/platform.h>
#include <mach/udif/platform.h>
#include <mach/regs-clk.h>

#define __UDIF_IOMEM_INIT(n,pa,sz)	UDIF_IOMEM_INIT(n,pa,sz,UDIF_IO_FLAGS_NONCACHED)

#define UDIF_INTRPT_INIT(n,i,f) \
{ \
	.name	= n, \
	.irq	= i, \
	.flags	= f, \
}


/* HW Timer stuff */
#define HWTIMER_NAME		"hwtimer"
#define HWTIMER_IO_BASE(ch)	CXD90014_TIMER_BASE(ch)
#define HWTIMER_IO_SIZE		0x100
#define HWTIMER_IRQ(ch)		IRQ_TIMER(ch)
#define HWTIMER_IRQ_FLAGS	IRQF_TIMER

static UDIF_CHANNELS hwtimer[UDIF_NR_HWTIMER] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "0", HWTIMER_IO_BASE(5), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "0", HWTIMER_IRQ(5), HWTIMER_IRQ_FLAGS),
	},
	[1] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "1", HWTIMER_IO_BASE(6), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "1", HWTIMER_IRQ(6), HWTIMER_IRQ_FLAGS),
	},
	[2] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "2", HWTIMER_IO_BASE(7), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "2", HWTIMER_IRQ(7), HWTIMER_IRQ_FLAGS),
	},
	[3] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "3", HWTIMER_IO_BASE(8), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "3", HWTIMER_IRQ(8), HWTIMER_IRQ_FLAGS),
	},
};

/* gpio stuff */
#define GPIO_NAME		"gpio"
#define GPIO_IO_BASE		CXD90014_GPIO_BASE
#define GPIO_IO_SIZE		0x00000050U
#define GPIO_IO_OFFSET		0x00000100U
/* gpio portn stuff */
#define GPIO_PORT_NAME(x)	( "gpio_p" #x )
#define GPIO_PORT_IO_BASE(x)	( GPIO_IO_BASE + (GPIO_IO_OFFSET * (x)) )

static UDIF_CHANNELS gpio[UDIF_NR_GPIO] = {
	[0]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(0),  GPIO_PORT_IO_BASE(0),  GPIO_IO_SIZE),  },
	[1]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(1),  GPIO_PORT_IO_BASE(1),  GPIO_IO_SIZE),  },
	[2]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(2),  GPIO_PORT_IO_BASE(2),  GPIO_IO_SIZE),  },
	[3]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(3),  GPIO_PORT_IO_BASE(3),  GPIO_IO_SIZE),  },
	[4]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(4),  GPIO_PORT_IO_BASE(4),  GPIO_IO_SIZE),  },
	[5]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(5),  GPIO_PORT_IO_BASE(5),  GPIO_IO_SIZE),  },
	[6]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(6),  GPIO_PORT_IO_BASE(6),  GPIO_IO_SIZE),  },
	[7]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(7),  GPIO_PORT_IO_BASE(7),  GPIO_IO_SIZE),  },
	[8]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(8),  GPIO_PORT_IO_BASE(8),  GPIO_IO_SIZE),  },
	[9]  = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(9),  GPIO_PORT_IO_BASE(9),  GPIO_IO_SIZE),  },
	[10] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(10), GPIO_PORT_IO_BASE(10), GPIO_IO_SIZE),  },
	[11] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(11), GPIO_PORT_IO_BASE(11), GPIO_IO_SIZE),  },
	[12] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(12), GPIO_PORT_IO_BASE(12), GPIO_IO_SIZE),  },
	[13] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(13), GPIO_PORT_IO_BASE(13), GPIO_IO_SIZE),  },
	[14] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(14), GPIO_PORT_IO_BASE(14), GPIO_IO_SIZE),  },
	[15] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(15), GPIO_PORT_IO_BASE(15), GPIO_IO_SIZE),  },
	[16] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(16), GPIO_PORT_IO_BASE(16), GPIO_IO_SIZE),  },
	[17] = { .iomem = __UDIF_IOMEM_INIT(GPIO_PORT_NAME(17), GPIO_PORT_IO_BASE(17), GPIO_IO_SIZE),  },
};

/* nand stuff */
#define NAND_NAME		"nand"
/* nand Ch0 stuff */
#define NAND_CH0_NAME		"nandreg"
#define NAND_CH0_IO_BASE	0x00020000U
#define NAND_CH0_IO_SIZE	0x00010000U
#define NAND_CH0_IRQ		IRQ_NANDC
/* nand Ch1 stuff */
#define NAND_CH1_NAME		"nandc"
#define NAND_CH1_IO_BASE	0x10000000U
#define NAND_CH1_IO_SIZE	0x00010000U
/* nand Ch2 stuff */
#define NAND_CH2_NAME		"arm7io"
#define NAND_CH2_IO_BASE	0x00011000U
#define NAND_CH2_IO_SIZE	0x00001000U
#define NAND_CH2_IRQ		IRQ_BOSS
/* nand Ch3 stuff */
#define NAND_CH3_NAME		"arm7sram"
#define NAND_CH3_IO_BASE	0x00000000U
#define NAND_CH3_IO_SIZE	0x00004000U
/* nand Ch4 stuff */
#define NAND_CH4_NAME		"clkrst3"
#define NAND_CH4_IO_BASE	CXD90014_CLKRST3(0)
#define NAND_CH4_IO_SIZE	0x00001000U

static UDIF_CHANNELS nand[UDIF_NR_NAND] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(NAND_CH0_NAME, NAND_CH0_IO_BASE, NAND_CH0_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(NAND_CH0_NAME, NAND_CH0_IRQ, 0),
	},
	[1] = {
		.iomem = __UDIF_IOMEM_INIT(NAND_CH1_NAME, NAND_CH1_IO_BASE, NAND_CH1_IO_SIZE),
	},
	[2] = {
		.iomem = __UDIF_IOMEM_INIT(NAND_CH2_NAME, NAND_CH2_IO_BASE, NAND_CH2_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(NAND_CH2_NAME, NAND_CH2_IRQ, 0),
	},
	[3] = {
		.iomem = __UDIF_IOMEM_INIT(NAND_CH3_NAME, NAND_CH3_IO_BASE, NAND_CH3_IO_SIZE),
	},
	[4] = {
		.iomem = __UDIF_IOMEM_INIT(NAND_CH4_NAME, NAND_CH4_IO_BASE, NAND_CH4_IO_SIZE),
	},
};

static UDIF_ERR nand_init(UDIF_CHANNELS *chs)
{
	if (unlikely(!udif_ioremap(&chs->iomem))) {
		UDIF_PERR("nand ioremap failed\n");
		return UDIF_ERR_NOMEM;
	}

	return UDIF_ERR_OK;
}

static void nand_exit(UDIF_CHANNELS *chs)
{
	udif_iounmap(&chs->iomem);
}

/* DMAC device */
#define DMAC_NAME		"dmac"
#define DMAC_IO_SIZE		0x00010000U
#define DMAC_IO_BASE		CXD90014_AHB_BASE
#define DMAC1_IO_BASE		(DMAC_IO_BASE + DMAC_IO_SIZE)
#define DMAC_IRQ(x)		IRQ_XDMAC0(x)
#define DMAC1_IRQ(x)		IRQ_XDMAC1(x)
#define DMAC_CH0_NAME		"dmac_ch0"
#define DMAC_CH1_NAME		"dmac_ch1"
#define DMAC_CH2_NAME		"dmac_ch2"
#define DMAC_CH3_NAME		"dmac_ch3"
#define DMAC_CH4_NAME		"dmac_ch4"
#define DMAC_CH5_NAME		"dmac_ch5"
#define DMAC_CH6_NAME		"dmac_ch6"
#define DMAC_CH7_NAME		"dmac_ch7"

static UDIF_CHANNELS dmac[UDIF_NR_DMAC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH0_NAME, DMAC_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH0_NAME, DMAC_IRQ(0), 0),
	},
	[1] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH1_NAME, DMAC_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH1_NAME, DMAC_IRQ(1), 0),
	},
	[2] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH2_NAME, DMAC_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH2_NAME, DMAC_IRQ(2), 0),
	},
	[3] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH3_NAME, DMAC_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH3_NAME, DMAC_IRQ(3), 0),
	},
	[4] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH4_NAME, DMAC1_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH4_NAME, DMAC1_IRQ(0), 0),
	},
	[5] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH5_NAME, DMAC1_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH5_NAME, DMAC1_IRQ(1), 0),
	},
	[6] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH6_NAME, DMAC1_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH6_NAME, DMAC1_IRQ(2), 0),
	},
	[7] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_CH7_NAME, DMAC1_IO_BASE, DMAC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(DMAC_CH7_NAME, DMAC1_IRQ(3), 0),
	},
};

/* ldec stuff */
#define LDEC_NAME		"ldec"
#define LDEC_IO_BASE		0xF2903000U
#define LDEC_IO_SIZE		0x00001000U
#define LDEC_IRQ		IRQ_LDEC

static UDIF_CHANNELS ldec[UDIF_NR_LDEC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(LDEC_NAME, LDEC_IO_BASE, LDEC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(LDEC_NAME, LDEC_IRQ, 0),
	},
};

/* Sircs */
#define SIRCS_NAME	"sircs"
#define SIRCS_IO_BASE	(CXD90014_APB0_BASE + 0x400000)
#define SIRCS_IO_SIZE	0x1000
#define SIRCS_IRQ	IRQ_SIRCS_RX

#define SIRCS_HCLK	CXD90014_CLKRST4(IPCLKEN2)
#define SIRCS_HCLK_SET	(SIRCS_HCLK + CLKRST_SET)
#define SIRCS_HCLK_CLR	(SIRCS_HCLK + CLKRST_CLR)
#define SIRCS_HCLK_SHIFT	2
#define SIRCS_12MHZ	CXD90014_CLKRST4(IPCLKEN4)
#define SIRCS_12MHZ_SET	(SIRCS_12MHZ + CLKRST_SET)
#define SIRCS_12MHZ_CLR	(SIRCS_12MHZ + CLKRST_CLR)
#define SIRCS_12MHZ_SHIFT	16

/* overrun */
#define SIRCS_OVERRUN_NAME      "sircs_overun"
#define SIRCS_OVERRUN_IRQ       IRQ_SIRCS_OVR

static UDIF_CHANNELS sircs[UDIF_NR_SIRCS] = {
	[UDIF_SIRCS_RX] = {
		.iomem = __UDIF_IOMEM_INIT(SIRCS_NAME, SIRCS_IO_BASE, SIRCS_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(SIRCS_NAME, SIRCS_IRQ, 0),
		.clock = UDIF_CLOCK_INIT(SIRCS_HCLK_SET, SIRCS_HCLK_CLR, SIRCS_HCLK_SHIFT,
					 SIRCS_12MHZ_SET, SIRCS_12MHZ_CLR, SIRCS_12MHZ_SHIFT,
					 0, 0, 0),
	},
	[UDIF_SIRCS_OVERRUN] = {
		.intr = UDIF_INTRPT_INIT(SIRCS_OVERRUN_NAME, SIRCS_OVERRUN_IRQ, 0),
	},
};

/* I2C stuff */
#define I2C_NAME	"i2c"
#define I2C_IO_BASE	(CXD90014_APB0_BASE + 0x401000)
#define I2C_IO_SIZE	0x1000
#define I2C_IRQ		IRQ_I2C

#define I2C_HCLK	CXD90014_CLKRST4(IPCLKEN2)
#define I2C_HCLK_SET    (I2C_HCLK + CLKRST_SET)
#define I2C_HCLK_CLR    (I2C_HCLK + CLKRST_CLR)
#define I2C_HCLK_SHIFT  1
#define I2C_12MHZ       CXD90014_CLKRST4(IPCLKEN4)
#define I2C_12MHZ_SET   (I2C_12MHZ + CLKRST_SET)
#define I2C_12MHZ_CLR   (I2C_12MHZ + CLKRST_CLR)
#define I2C_12MHZ_SHIFT 17

static UDIF_CHANNELS i2c[UDIF_NR_I2C] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(I2C_NAME, I2C_IO_BASE, I2C_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(I2C_NAME, I2C_IRQ, 0),
		.clock = UDIF_CLOCK_INIT(I2C_HCLK_SET, I2C_HCLK_CLR, I2C_HCLK_SHIFT,
					 I2C_12MHZ_SET, I2C_12MHZ_CLR, I2C_12MHZ_SHIFT,
					 0, 0, 0),
	},
};

/* MS stuff */
#define MS_NAME		"ms"
#define MS_IO_BASE	(CXD90014_AHB_BASE + 0x303000)
#define MS_IO_SIZE	0x1000
#define MS_IRQ		IRQ_MS0

#define MS_HCLK		CXD90014_CLKRST4(IPCLKEN0)
#define MS_HCLK_SET	(MS_HCLK + CLKRST_SET)
#define MS_HCLK_CLR	(MS_HCLK + CLKRST_CLR)
#define MS_HCLK_SHIFT	0
#define MS_DCLK		CXD90014_CLKRST4(IPCLKEN5)
#define MS_DCLK_SET	(MS_DCLK + CLKRST_SET)
#define MS_DCLK_CLR	(MS_DCLK + CLKRST_CLR)
#define MS_DCLK_SHIFT	24

static UDIF_CHANNELS ms[UDIF_NR_MS] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(MS_NAME, MS_IO_BASE, MS_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MS_NAME, MS_IRQ, 0),
		.clock = UDIF_CLOCK_INIT(MS_HCLK_SET, MS_HCLK_CLR, MS_HCLK_SHIFT,
					 MS_DCLK_SET, MS_DCLK_CLR, MS_DCLK_SHIFT,
					 0, 0, 0),
	},
};

/* MMC stuff */
#define MMC_NAME 	"mmc"
#define MMC_SDIF0_IO_BASE	0xF0300000U
#define MMC_SDIF0_IO_SIZE	0x00001000U
#define MMC_SDIF1_IO_BASE	0xF0301000U
#define MMC_SDIF1_IO_SIZE	0x00001000U
#define MMC_MMC_IO_BASE		0xF0302000U
#define MMC_MMC_IO_SIZE		0x00001000U
#define TE_IRQ(x)	IRQ_SDIF(x)

/* MMC is mapped  */
static UDIF_CHANNELS mmc[UDIF_NR_MMC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(MMC_NAME "0", MMC_SDIF0_IO_BASE, MMC_SDIF0_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MMC_NAME "0", TE_IRQ(0), 0),
	},
	[1] = {
		.iomem = __UDIF_IOMEM_INIT(MMC_NAME "1", MMC_SDIF1_IO_BASE, MMC_SDIF1_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MMC_NAME "1", TE_IRQ(1), 0),
	},
	[2] = {
		.iomem = __UDIF_IOMEM_INIT(MMC_NAME "2", MMC_MMC_IO_BASE, MMC_MMC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MMC_NAME "2", TE_IRQ(2), 0),
	},
};

/* HDMI stuff */
#define HDMI_NAME	"hdmi"
#define HDMI_IO_BASE	(CXD90014_APB0_BASE + 0xa24000)
#define HDMI_IO_SIZE	0x4000

static UDIF_CHANNELS hdmi[UDIF_NR_HDMI] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(HDMI_NAME, HDMI_IO_BASE, HDMI_IO_SIZE),
	},
};

/* CEC stuff */
#define CEC_NAME	"cec"
#define CEC_IO_BASE	(CXD90014_APB5_BASE + 0x10000)
#define CEC_IO_SIZE	0x190

static UDIF_CHANNELS cec[UDIF_NR_CEC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(CEC_NAME, CEC_IO_BASE, CEC_IO_SIZE),
	},
};

/* FUSB */
/* fusb stuff */
#define FUSB_NAME           "fusb"
/* fusb Ch0 stuff */
#define FUSB_CH0_NAME       "fusb_usb_vbus"
#define FUSB_CH0_IRQ        232U
/* fusb Ch1 stuff */
#define FUSB_CH1_NAME       "fusb_usb_id"
#define FUSB_CH1_IRQ        233U
/* fusb Ch2 stuff */
#define FUSB_CH2_NAME       "otg"
#define FUSB_CH2_IO_BASE    0xF2920000U
#define FUSB_CH2_IO_SIZE    0x00001000U
/* fusb Ch3 stuff */
#define FUSB_CH3_NAME       "ho"
#define FUSB_CH3_IO_BASE    0xF0200000U
#define FUSB_CH3_IO_SIZE    0x00003000U
/* fusb Ch4 stuff */
#define FUSB_CH4_NAME       "hdc"
#define FUSB_CH4_IO_BASE    0xF0210000U
#define FUSB_CH4_IO_SIZE    0x00020000U
/* fusb Ch5 stuff */
#define FUSB_CH5_NAME       "hdmac"
#define FUSB_CH5_IO_BASE    0xF0204000U
#define FUSB_CH5_IO_SIZE    0x00001000U
/* fusb Ch6 stuff */
#define FUSB_CH6_NAME       "h2xbb"
#define FUSB_CH6_IO_BASE    0xF0203000U
#define FUSB_CH6_IO_SIZE    0x00001000U
/* fusb Ch7 stuff */
#define FUSB_CH7_NAME       "clkrst4"
#define FUSB_CH7_IO_BASE    0xF2A30000U
#define FUSB_CH7_IO_SIZE    0x00001000U

static UDIF_CHANNELS fusb[UDIF_NR_FUSB] = {
    [0] = {
        .iomem = __UDIF_IOMEM_INIT(FUSB_CH2_NAME, FUSB_CH2_IO_BASE, FUSB_CH2_IO_SIZE),
    },
    [1] = {
        .iomem = __UDIF_IOMEM_INIT(FUSB_CH3_NAME, FUSB_CH3_IO_BASE, FUSB_CH3_IO_SIZE),
    },
    [2] = {
        .iomem = __UDIF_IOMEM_INIT(FUSB_CH4_NAME, FUSB_CH4_IO_BASE, FUSB_CH4_IO_SIZE),
    },
    [3] = {
        .iomem = __UDIF_IOMEM_INIT(FUSB_CH5_NAME, FUSB_CH5_IO_BASE, FUSB_CH5_IO_SIZE),
    },
    [4] = {
        .iomem = __UDIF_IOMEM_INIT(FUSB_CH6_NAME, FUSB_CH6_IO_BASE, FUSB_CH6_IO_SIZE),
    },
    [5] = {
        .iomem = __UDIF_IOMEM_INIT(FUSB_CH7_NAME, FUSB_CH7_IO_BASE, FUSB_CH7_IO_SIZE),
    },
    [6] = {
        .intr = UDIF_INTRPT_INIT(FUSB_CH0_NAME, FUSB_CH0_IRQ, IRQF_TRIGGER_RISING),
    },
    [7] = {
        .intr = UDIF_INTRPT_INIT(FUSB_CH1_NAME, FUSB_CH1_IRQ, IRQF_TRIGGER_RISING),
    },
};

/* SIO */
#define SIO_NAME        "sio"
#define SIO_IO_BASE(ch) (CXD90014_APB0_BASE + 0x405000 + SIO_IO_SIZE * (ch))
#define SIO_IO_SIZE     0x200
#define SIO_IRQ(ch)     IRQ_SIO(ch)

static UDIF_CHANNELS sio[UDIF_NR_SIO] = {
        [0] = {
                .iomem = __UDIF_IOMEM_INIT(SIO_NAME "0", SIO_IO_BASE(0), SIO_IO_SIZE),
                .intr = UDIF_INTRPT_INIT(SIO_NAME "0", SIO_IRQ(0), 0),
        },
        [1] = {
                .iomem = __UDIF_IOMEM_INIT(SIO_NAME "1", SIO_IO_BASE(1), SIO_IO_SIZE),
                .intr = UDIF_INTRPT_INIT(SIO_NAME "1", SIO_IRQ(1), 0),
        },
        [2] = {
                .iomem = __UDIF_IOMEM_INIT(SIO_NAME "2", SIO_IO_BASE(2), SIO_IO_SIZE),
                .intr = UDIF_INTRPT_INIT(SIO_NAME "2", SIO_IRQ(2), 0),
        },
        [3] = {
                .iomem = __UDIF_IOMEM_INIT(SIO_NAME "3", SIO_IO_BASE(3), SIO_IO_SIZE),
                .intr = UDIF_INTRPT_INIT(SIO_NAME "3", SIO_IRQ(3), 0),
        },
        [4] = {
                .iomem = __UDIF_IOMEM_INIT(SIO_NAME "4", SIO_IO_BASE(4), SIO_IO_SIZE),
                .intr = UDIF_INTRPT_INIT(SIO_NAME "4", SIO_IRQ(4), 0),
        },
};

/* SATA */
#define SATA_NAME        "sata"

/*
 * All Devices
 */
UDIF_DEVICE udif_devices[UDIF_ID_NUM] = {
	[UDIF_ID_HWTIMER] = {
		.name	= HWTIMER_NAME,
		.nr_ch	= NARRAY(hwtimer),
		.chs	= hwtimer,
	},
	[UDIF_ID_GPIO] = {
		.name	= GPIO_NAME,
		.nr_ch	= NARRAY(gpio),
		.chs	= gpio,
	},
	[UDIF_ID_NAND] = {
		.name   = NAND_NAME,
		.nr_ch  = NARRAY(nand),
		.chs    = nand,
		.init	= nand_init,
		.exit	= nand_exit,
	},
	[UDIF_ID_DMAC] = {
		.name   = DMAC_NAME,
		.nr_ch  = NARRAY(dmac),
		.chs    = dmac,
	},
	[UDIF_ID_LDEC] = {
		.name   = LDEC_NAME,
		.nr_ch  = NARRAY(ldec),
		.chs    = ldec,
	},
	[UDIF_ID_SIRCS] = {
		.name   = SIRCS_NAME,
		.nr_ch  = NARRAY(sircs),
		.chs    = sircs,
	},
	[UDIF_ID_I2C] = {
		.name   = I2C_NAME,
		.nr_ch  = NARRAY(i2c),
		.chs    = i2c,
	},
	[UDIF_ID_MS] = {
		.name   = MS_NAME,
		.nr_ch  = NARRAY(ms),
		.chs    = ms,
	},
	[UDIF_ID_MMC] = {
		.name	= MMC_NAME,
		.nr_ch 	= NARRAY(mmc),
		.chs	= mmc,
	},
	[UDIF_ID_HDMI] = {
		.name   = HDMI_NAME,
		.nr_ch  = NARRAY(hdmi),
		.chs    = hdmi,
	},
	[UDIF_ID_CEC] = {
		.name   = CEC_NAME,
		.nr_ch  = NARRAY(cec),
		.chs    = cec,
	},
	[UDIF_ID_FUSB_OTG] = {
		.name   = FUSB_NAME,
		.nr_ch  = NARRAY(fusb),
		.chs    = fusb,
	},
        [UDIF_ID_SIO] = {
                .name   = SIO_NAME,
                .nr_ch = NARRAY(sio),
                .chs    = sio,
        },
        [UDIF_ID_SATA] = {
                .name   = SATA_NAME,
                .nr_ch = 0,
        },
};

EXPORT_SYMBOL(udif_devices);

static int udif_platform_register(void)
{
	int i, ret = 0;

	for (i = 0; i < NARRAY(udif_devices); i++)
		ret = udif_device_register(&udif_devices[i]);

	return ret;
}

arch_initcall(udif_platform_register);
