/*
 * arch/arm/mach-cxd4132/udif/platform.c
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/udif/device.h>
#include <linux/udif/string.h>
#include <linux/udif/macros.h>
#include <mach/platform.h>
#include <mach/udif/platform.h>

#ifdef CONFIG_CXD41XX_DMAC
#define PL080_DMAC
#endif

#define PER_CLKRST_BASE	(CXD4132_PERMISC_BASE + 0x1000)

#define SZ_32K 0x00008000

#define __UDIF_IOMEM_INIT(n,pa,sz)	UDIF_IOMEM_INIT(n,pa,sz,UDIF_IO_FLAGS_NONCACHED)

#define UDIF_INTRPT_INIT(n,i,f) \
{ \
	.name	= n, \
	.irq	= i, \
	.flags	= f, \
}

#ifdef PL080_DMAC
#define UDIF_DMAC_INIT(n,c,r,f) \
{ \
	.ch	= c, \
	.rqid	= r, \
	.intr	= UDIF_INTRPT_INIT(n "_dmac", IRQ_DMAC0(c), f), \
}
#else
#define UDIF_DMAC_INIT(n,c,r,f) \
{ \
	.ch	= c, \
	.rqid	= r, \
	.intr	= UDIF_INTRPT_INIT(n "_fdmac", IRQ_FSP_DMAC(c), f), \
}
#endif

#define __UDIF_DMAC_INIT(n,v) \
{ \
	.num	= n, \
	.dmac	= v, \
}

/*
 * Each Device
 */

/* GPIO stuff */
#define GPIO_NAME 		"gpio"
#define GPIO_IO_SIZE		0x100
#define GPIO_IO_BASE(x)		PA_GPIO(x)

static UDIF_CHANNELS gpio[UDIF_NR_GPIO] = {
	[0] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "0", GPIO_IO_BASE(0), GPIO_IO_SIZE), },
	[1] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "1", GPIO_IO_BASE(1), GPIO_IO_SIZE), },
	[2] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "2", GPIO_IO_BASE(2), GPIO_IO_SIZE), },
	[3] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "3", GPIO_IO_BASE(3), GPIO_IO_SIZE), },
	[4] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "4", GPIO_IO_BASE(4), GPIO_IO_SIZE), },
	[5] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "5", GPIO_IO_BASE(5), GPIO_IO_SIZE), },
	[6] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "6", GPIO_IO_BASE(6), GPIO_IO_SIZE), },
	[7] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "7", GPIO_IO_BASE(7), GPIO_IO_SIZE), },
	[8] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "8", GPIO_IO_BASE(8), GPIO_IO_SIZE), },
	[9] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "9", GPIO_IO_BASE(9), GPIO_IO_SIZE), },
	[10] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "10", GPIO_IO_BASE(10), GPIO_IO_SIZE), },
	[11] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "11", GPIO_IO_BASE(11), GPIO_IO_SIZE), },
	[12] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "12", GPIO_IO_BASE(12), GPIO_IO_SIZE), },
	[13] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "13", GPIO_IO_BASE(13), GPIO_IO_SIZE), },
	[14] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "14", GPIO_IO_BASE(14), GPIO_IO_SIZE), },
	[15] = { .iomem = __UDIF_IOMEM_INIT(GPIO_NAME "15", GPIO_IO_BASE(15), GPIO_IO_SIZE), },
};

/* GPE stuff */
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/mm.h>

#define GPE_NAME 	"gpe"
#define GPE_REV 	"1.0"
#define GPE_IO_BASE	0xf5060000
#define GPE_IO_SIZE	SZ_64K
#define GPE_IRQ		IRQ_GPE
#define GPE_IRQ_FLAGS	IRQF_DISABLED

static UDIF_CHANNELS gpe[UDIF_NR_GPE] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(GPE_NAME, GPE_IO_BASE, GPE_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(GPE_NAME, GPE_IRQ, 0),
	},
};

static irqreturn_t gpe_uio_handler(int irq, struct uio_info *info)
{
	return IRQ_HANDLED;
}

struct resource gpe_res = {
	.start	= GPE_IO_BASE,
	.end	= GPE_IO_BASE + GPE_IO_SIZE - 1,
	.flags	= IORESOURCE_MEM,
};

static struct uio_info gpe_uio_info = {
	.name		= GPE_NAME,
	.version	= GPE_REV,
	.irq		= GPE_IRQ,
	.irq_flags	= GPE_IRQ_FLAGS,
	.handler	= gpe_uio_handler,
};

static struct platform_device gpe_device = {
	.name			= "uio_pdrv",
	.num_resources		= 1,
	.resource		= &gpe_res,
	.dev.platform_data	= &gpe_uio_info,
};

static int gpe_init(UDIF_CHANNELS *chs)
{
	UDIF_ERR ret = platform_device_register(&gpe_device);

	if (unlikely(ret)) {
		UDIF_PERR("gpe: failed platform_device_register(), ret = %d\n", ret);
	}

	return ret;
}

static void gpe_exit(UDIF_CHANNELS *chs)
{
	platform_device_unregister(&gpe_device);
}

/* SIO */
#define SIO_NAME	"sio"
#define SIO_IO_BASE(ch)	(CXD4132_APB0_BASE + 0x10000 + SIO_IO_SIZE * (ch))
#define SIO_IO_SIZE	0x200
#define SIO_IRQ(ch)	IRQ_SIO(ch)

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

/* Sircs */
#define SIRCS_NAME	"sircs"
#define SIRCS_IO_BASE	(CXD4132_APB0_BASE + 0x28000)
#define SIRCS_IO_SIZE	0x100
#define SIRCS_IRQ	IRQ_SIRCS_RX

#define SIRCS_HCLK		(PER_CLKRST_BASE + 0x4B0)
#define SIRCS_HCLK_SET		(SIRCS_HCLK + 0x4)
#define SIRCS_HCLK_CLR		(SIRCS_HCLK + 0x8)
#define SIRCS_HCLK_SHIFT	0
#define SIRCS_12MHZ		(PER_CLKRST_BASE + 0x4c0)
#define SIRCS_12MHZ_SET		(SIRCS_12MHZ + 0x4)
#define SIRCS_12MHZ_CLR		(SIRCS_12MHZ + 0x8)
#define SIRCS_12MHZ_SHIFT	0

/* overrun */
#define SIRCS_OVERRUN_NAME	"sircs_overun"
#define SIRCS_OVERRUN_IRQ	IRQ_SIRCS_OVR

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
#define I2C_IO_BASE	(CXD4132_APB0_BASE + 0x18000)
#define I2C_IO_SIZE	0x100
#define I2C_IRQ		IRQ_I2C

#define I2C_HCLK	(PER_CLKRST_BASE + 0x490)
#define I2C_HCLK_SET	(I2C_HCLK + 0x4)
#define I2C_HCLK_CLR	(I2C_HCLK + 0x8)
#define I2C_HCLK_SHIFT	0
#define I2C_12MHZ	(PER_CLKRST_BASE + 0x4a0)
#define I2C_12MHZ_SET	(I2C_12MHZ + 0x4)
#define I2C_12MHZ_CLR	(I2C_12MHZ + 0x8)
#define I2C_12MHZ_SHIFT	0

static UDIF_CHANNELS i2c[UDIF_NR_I2C] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(I2C_NAME, I2C_IO_BASE, I2C_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(I2C_NAME, I2C_IRQ, 0),
		.clock = UDIF_CLOCK_INIT(I2C_HCLK_SET, I2C_HCLK_CLR, I2C_HCLK_SHIFT,
					 I2C_12MHZ_SET, I2C_12MHZ_CLR, I2C_12MHZ_SHIFT,
					 0, 0, 0),
	},
};

/* DMAC stuff */
#ifdef PL080_DMAC
#define DMAC_NAME 	"dmac"
#define DMAC_IO_BASE	CXD4132_DMAC0_BASE
#define DMAC_IO_SIZE	CXD4132_DMAC0_SIZE
#else
#define DMAC_NAME 	"fdmac"
#define DMAC_IO_BASE	(CXD4132_FSP_BASE+0x1000)
#define DMAC_IO_SIZE	(0x1000)
#endif

static UDIF_CHANNELS dmac[UDIF_NR_DMAC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(DMAC_NAME, DMAC_IO_BASE, DMAC_IO_SIZE),
	},
};

static UDIF_DMAC dmac_dmac[] = {
	UDIF_DMAC_INIT(DMAC_NAME "-0", 0, 0, 0),
	UDIF_DMAC_INIT(DMAC_NAME "-1", 1, 1, 0),
	UDIF_DMAC_INIT(DMAC_NAME "-2", 2, 2, 0),
	UDIF_DMAC_INIT(DMAC_NAME "-3", 3, 3, 0),
#ifdef PL080_DMAC
	UDIF_DMAC_INIT(DMAC_NAME "-4", 4, 4, 0),
	UDIF_DMAC_INIT(DMAC_NAME "-5", 5, 5, 0),
	UDIF_DMAC_INIT(DMAC_NAME "-6", 6, 6, 0),
	UDIF_DMAC_INIT(DMAC_NAME "-7", 7, 7, 0),
#endif
};

/* NAND */
#define NAND_NAME	"nand"
#define NAND_IO_BASE	FSP_CS0_BASE
#define NAND_IO_SIZE	SZ_128K
#define NAND_IRQ	IRQ_FSP_XINT
#if 0
# define NAND_IRQ_FLAGS	IRQF_NODELAY
#else
# define NAND_IRQ_FLAGS	0
#endif

#define NAND_DMAC_CH	0
#define NAND_DMAC_IRQ	DMAC_CH_TO_IRQ(DMAC_CH_NAND)

static UDIF_CHANNELS nand[UDIF_NR_NAND] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(NAND_NAME, NAND_IO_BASE, NAND_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(NAND_NAME, NAND_IRQ, NAND_IRQ_FLAGS),
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

static UDIF_DMAC nand_dmac[] = {
	UDIF_DMAC_INIT(NAND_NAME, NAND_DMAC_CH, UDIF_DMAC_NORQID, NAND_IRQ_FLAGS),
};

/* MMC stuff */
#define MMC_NAME 	"mmc"
#define TE_IO_SIZE	0x10000
#define TE_IO_BASE(x)	(CXD4132_AHB_BASE + 0x70000 + TE_IO_SIZE * (x))
#define TE_IRQ(x)	IRQ_SDIF(x)

/* software reset */
#define TE_HCLK(x)	(PER_CLKRST_BASE + 0x100 + 0x30 * (x))
#define TE_HCLK_SET(x)	(TE_HCLK(x) + 0x4)
#define TE_HCLK_CLR(x)	(TE_HCLK(x) + 0x8)
#define TE_HCLK_SHIFT	0

#define TE_DCLK		CXD4132_CLKRST(0x1f)
#define TE_DCLK_SET	(TE_DCLK + 0x4)
#define TE_DCLK_CLR	(TE_DCLK + 0x8)
#define TE_DCLK_SHIFT(x) (8 + (x))

#define TE_DSEL		CXD4132_CLKRST(0x21)
#define TE_DSEL_SET	TE_DSEL
#define TE_DSEL_CLR	0
#define TE_DSEL_SHIFT(x) (20 + (x) * 4)

/* MMC is mapped TE1 */
static UDIF_CHANNELS mmc[UDIF_NR_MMC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(MMC_NAME "0", TE_IO_BASE(0), TE_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MMC_NAME "0", TE_IRQ(0), 0),
		.clock = UDIF_CLOCK_INIT(TE_HCLK_SET(0),  TE_HCLK_CLR(0),  TE_HCLK_SHIFT,
					 TE_DCLK_SET, TE_DCLK_CLR, TE_DCLK_SHIFT(0),
					 TE_DSEL_SET, TE_DSEL_CLR, TE_DSEL_SHIFT(0)),
	},
	[1] = {
		.iomem = __UDIF_IOMEM_INIT(MMC_NAME "1", TE_IO_BASE(1), TE_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MMC_NAME "1", TE_IRQ(1), 0),
		.clock = UDIF_CLOCK_INIT(TE_HCLK_SET(1),  TE_HCLK_CLR(1),  TE_HCLK_SHIFT,
					 TE_DCLK_SET, TE_DCLK_CLR, TE_DCLK_SHIFT(1),
					 TE_DSEL_SET, TE_DSEL_CLR, TE_DSEL_SHIFT(1)),
	},
	[2] = {
		.iomem = __UDIF_IOMEM_INIT(MMC_NAME "2", TE_IO_BASE(2), TE_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MMC_NAME "2", TE_IRQ(2), 0),
		.clock = UDIF_CLOCK_INIT(TE_HCLK_SET(2),  TE_HCLK_CLR(2),  TE_HCLK_SHIFT,
					 TE_DCLK_SET, TE_DCLK_CLR, TE_DCLK_SHIFT(2),
					 TE_DSEL_SET, TE_DSEL_CLR, TE_DSEL_SHIFT(2)),
	},
};

/* MS stuff */
#define MS_NAME		"ms"
#define MS_IO_SIZE	0x10000
#define MS_IO_BASE(x)	(CXD4132_AHB_BASE + 0xa0000 + MS_IO_SIZE * (x))
#define MS_IRQ(x)	(IRQ_MS0 + (x))

/* software reset */
#define MS_HCLK(x)	(PER_CLKRST_BASE + 0x190)
#define MS_HCLK_SET(x)	(MS_HCLK(x) + 0x4)
#define MS_HCLK_CLR(x)	(MS_HCLK(x) + 0x8)
#define MS_HCLK_SHIFT	0

#define MS_DCLK		CXD4132_CLKRST(0x1f)
#define MS_DCLK_SET	(MS_DCLK + 0x4)
#define MS_DCLK_CLR	(MS_DCLK + 0x8)
#define MS_DCLK_SHIFT(x) (4 + (x))

#define MS_DSEL		CXD4132_CLKRST(0x21)
#define MS_DSEL_SET	MS_DSEL
#define MS_DSEL_CLR	0
#define MS_DSEL_SHIFT(x) (16 + (x) * 4)

static UDIF_CHANNELS ms[UDIF_NR_MS] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(MS_NAME "0", MS_IO_BASE(0), MS_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MS_NAME "0", MS_IRQ(0), 0),
		.clock = UDIF_CLOCK_INIT(MS_HCLK_SET(0),  MS_HCLK_CLR(0),  MS_HCLK_SHIFT,
					 MS_DCLK_SET, MS_DCLK_CLR, MS_DCLK_SHIFT(0),
					 MS_DSEL_SET, MS_DSEL_CLR, MS_DSEL_SHIFT(0)),
	},
};

/* hdmi stuff */
#define HDMI_NAME	"hdmi"
#define HDMI_IO_BASE	(CXD4132_APB3_BASE + 0xd000)
#define HDMI_IO_SIZE	0x10000

static UDIF_CHANNELS hdmi[UDIF_NR_HDMI] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(HDMI_NAME, HDMI_IO_BASE, HDMI_IO_SIZE),
	},
};

/* cec stuff */
#define CEC_NAME	"cec"
#define CEC_IO_BASE	(CXD4132_APB3_BASE + 0xc000)
#define CEC_IO_SIZE	0x190

static UDIF_CHANNELS cec[UDIF_NR_CEC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(CEC_NAME, CEC_IO_BASE, CEC_IO_SIZE),
	},
};

/* fsp stuff */
#define FSP_NAME	"fsp"
#define FSP_IO_BASE	(CXD4132_FSP_BASE)
#define FSP_IO_SIZE	CXD4132_FSP_SIZE

static UDIF_CHANNELS fsp[UDIF_NR_FSP] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(FSP_NAME, FSP_IO_BASE, FSP_IO_SIZE),
	},
};

/* meno stuff */
#define MENO_NAME	"meno"
#define MENO_IO_BASE	(CXD4132_FSP_BASE+0x2000)
#define MENO_IO_SIZE	0x3000
#define MENO_IRQ	IRQ_FSP_MENO

static UDIF_CHANNELS meno[UDIF_NR_MENO] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(MENO_NAME, MENO_IO_BASE, MENO_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MENO_NAME, MENO_IRQ, 0),
	},
};

/* ldec stuff */
#define LDEC_NAME	"ldec"
#define LDEC_IO_BASE	(CXD4132_FSP_BASE+0x100)
#define LDEC_IO_SIZE	0x300
#define LDEC_IRQ	IRQ_FSP_LDEC
#define LDEC_DMAC_CH	3

static UDIF_CHANNELS ldec[UDIF_NR_LDEC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(LDEC_NAME, LDEC_IO_BASE, LDEC_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(LDEC_NAME, LDEC_IRQ, 0),
	},
};

static UDIF_DMAC ldec_dmac[] = {
	UDIF_DMAC_INIT(LDEC_NAME, LDEC_DMAC_CH, UDIF_DMAC_NORQID, 0),
};

/* sata stuff */
#define SATA_NAME	"sata"
#define SATA_IO_BASE	(CXD4132_AHB_BASE + 0x10000)
#define SATA_IO_SIZE	0x8000
#define SATA_IRQ	IRQ_SATA

/* other irqs */
#define SATA_PORT0_NAME	"sata_port0"
#define SATA_PORT0_IRQ	IRQ_SATA_PORT0
#define SATA_CCC_NAME	"sata_ccc"
#define SATA_CCC_IRQ	IRQ_SATA_CCC

#define SATA_HCLK		(PER_CLKRST_BASE + 0x1600)
#define SATA_HCLK_SET		(SATA_HCLK + 0x4)
#define SATA_HCLK_CLR		(SATA_HCLK + 0x8)
#define SATA_HCLK_SHIFT		0

static UDIF_CHANNELS sata[UDIF_NR_SATA] = {
	[UDIF_SATA] = {
		.iomem = __UDIF_IOMEM_INIT(SATA_NAME, SATA_IO_BASE, SATA_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(SATA_NAME, SATA_IRQ, 0),
		.clock = UDIF_CLOCK_INIT(SATA_HCLK_SET, SATA_HCLK_CLR, SATA_HCLK_SHIFT,
					 0, 0, 0,
					 0, 0, 0),
	},
	[UDIF_SATA_PORT0] = {
		.intr = UDIF_INTRPT_INIT(SATA_PORT0_NAME, SATA_PORT0_IRQ, 0),
	},
	[UDIF_SATA_CCC] = {
		.intr = UDIF_INTRPT_INIT(SATA_CCC_NAME, SATA_CCC_IRQ, 0),
	},
};

/* SDC stuff */
#define SDC_NAME	"sdc"
#define SDC_IO_BASE	CXD4132_SDC_BASE
#define SDC_IO_SIZE	CXD4132_SDC_SIZE

static UDIF_CHANNELS sdc[UDIF_NR_SDC] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(SDC_NAME, SDC_IO_BASE, SDC_IO_SIZE),
	},
};

/* MIA stuff */
#define MIA_NAME	"mia"
#define MIA_IO_BASE	(FSP_CS2_BASE + 0x200000)
#define MIA_IO_SIZE	0x10000
#define MIA_IRQ		IRQ_SHM
#if 0
# define MIA_IRQ_FLAGS	IRQF_NODELAY
#else
# define MIA_IRQ_FLAGS	0
#endif
#define MIA_DMAC_CH	2

static UDIF_CHANNELS mia[UDIF_NR_MIA] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(MIA_NAME, MIA_IO_BASE, MIA_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(MIA_NAME, MIA_IRQ, MIA_IRQ_FLAGS),
	},
};

static UDIF_DMAC mia_dmac[] = {
	UDIF_DMAC_INIT(MIA_NAME, MIA_DMAC_CH, UDIF_DMAC_NORQID, 0),
};

/* HW Timer stuff */
#define HWTIMER_NAME		"hwtimer"
#define HWTIMER_IO_BASE(ch)	CXD4115_TIMER_BASE(ch)
#define HWTIMER_IO_SIZE		0x20
#define HWTIMER_IRQ(ch)		IRQ_TIMER(ch)
#define HWTIMER_IRQ_FLAGS	IRQF_TIMER

static UDIF_CHANNELS hwtimer[UDIF_NR_HWTIMER] = {
	[0] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "0", HWTIMER_IO_BASE(4), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "0", HWTIMER_IRQ(4), HWTIMER_IRQ_FLAGS),
	},
	[1] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "1", HWTIMER_IO_BASE(2), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "1", HWTIMER_IRQ(2), HWTIMER_IRQ_FLAGS),
	},
	[2] = {
		.iomem = __UDIF_IOMEM_INIT(HWTIMER_NAME "2", HWTIMER_IO_BASE(3), HWTIMER_IO_SIZE),
		.intr = UDIF_INTRPT_INIT(HWTIMER_NAME "2", HWTIMER_IRQ(3), HWTIMER_IRQ_FLAGS),
	},
};

/*
 * All Devices
 */
UDIF_DEVICE udif_devices[UDIF_ID_NUM] = {
	[UDIF_ID_GPIO] = {
		.name	= GPIO_NAME,
		.nr_ch 	= NARRAY(gpio),
		.chs	= gpio,
	},
	[UDIF_ID_GPE] = {
		.name	= GPE_NAME,
		.nr_ch 	= NARRAY(gpe),
		.chs	= gpe,
		.init	= gpe_init,
		.exit	= gpe_exit,
	},
	[UDIF_ID_SIO] = {
		.name	= SIO_NAME,
		.nr_ch = NARRAY(sio),
		.chs	= sio,
	},
	[UDIF_ID_SIRCS] = {
		.name	= SIRCS_NAME,
		.nr_ch = NARRAY(sircs),
		.chs	= sircs,
	},
	[UDIF_ID_I2C] = {
		.name	= I2C_NAME,
		.nr_ch 	= NARRAY(i2c),
		.chs	= i2c,
	},
	[UDIF_ID_DMAC] = {
		.name	= DMAC_NAME,
		.dmac	= __UDIF_DMAC_INIT(NARRAY(dmac_dmac), dmac_dmac),
		.nr_ch = NARRAY(dmac),
		.chs	= dmac,
	},
	[UDIF_ID_NAND] = {
		.name	= NAND_NAME,
		.dmac	= __UDIF_DMAC_INIT(NARRAY(nand_dmac), nand_dmac),
		.nr_ch = NARRAY(nand),
		.chs	= nand,
		.init	= nand_init,
		.exit	= nand_exit,
	},
	[UDIF_ID_MMC] = {
		.name	= MMC_NAME,
		.nr_ch 	= NARRAY(mmc),
		.chs	= mmc,
	},
	[UDIF_ID_MS] = {
		.name	= MS_NAME,
		.nr_ch 	= NARRAY(ms),
		.chs	= ms,
	},
	[UDIF_ID_HDMI] = {
		.name	= HDMI_NAME,
		.nr_ch 	= NARRAY(hdmi),
		.chs	= hdmi,
	},
	[UDIF_ID_CEC] = {
		.name	= CEC_NAME,
		.nr_ch 	= NARRAY(cec),
		.chs	= cec,
	},
	[UDIF_ID_FSP] = {
		.name	= FSP_NAME,
		.nr_ch 	= NARRAY(fsp),
		.chs	= fsp,
	},
	[UDIF_ID_MENO] = {
		.name	= MENO_NAME,
		.nr_ch 	= NARRAY(meno),
		.chs	= meno,
	},
	[UDIF_ID_LDEC] = {
		.name	= LDEC_NAME,
		.dmac	= __UDIF_DMAC_INIT(NARRAY(ldec_dmac), ldec_dmac),
		.nr_ch 	= NARRAY(ldec),
		.chs	= ldec,
	},
	[UDIF_ID_SATA] = {
		.name	= SATA_NAME,
		.nr_ch 	= NARRAY(sata),
		.chs	= sata,
	},
	[UDIF_ID_SDC] = {
		.name	= SDC_NAME,
		.nr_ch 	= NARRAY(sdc),
		.chs	= sdc,
	},
	[UDIF_ID_MIA] = {
		.name	= MIA_NAME,
		.dmac	= __UDIF_DMAC_INIT(NARRAY(mia_dmac), mia_dmac),
		.nr_ch 	= NARRAY(mia),
		.chs	= mia,
	},
	[UDIF_ID_HWTIMER] = {
		.name	= HWTIMER_NAME,
		.nr_ch	= NARRAY(hwtimer),
		.chs	= hwtimer,
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
