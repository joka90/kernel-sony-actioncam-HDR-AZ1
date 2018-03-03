/*
 * arch/arm/mach-cxd4132/pm_macro.S
 *
 * sleep macro
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
#include <mach/hardware.h>
#include <mach/bootram.h>
#include <mach/regs-dmc.h>
#include <mach/regs-octrl.h>
#include <mach/regs-htp.h>

#define DDR2_MR17	17

#undef ERROR_INJECTION

#undef CHKSUM_TYPE1
#undef CHKSUM_TYPE2

#if defined(CHKSUM_TYPE1) && defined(CHKSUM_TYPE2)
# define CHECKSUM_EVAL
#endif

/* CHKSUM_TYPE1 parameters */
#if 0
#define CHKSUM_AREA	64
#define CHKSUM_SIZE	SZ_16K		/* 32byte align */
#define CHKSUM_LEAP	SZ_2M		/* 32byte align */
#else
#define CHKSUM_AREA	5243
#define CHKSUM_SIZE	SZ_1K		/* 32byte align */
#define CHKSUM_LEAP	(24*1024)	/* 32byte align */
#endif

/* CHKSUM_TYPE2 parameters */
#if CONFIG_CXD4132_TEXT_OFFSET < SZ_128M
#define CHKSUM2_AREA	(DDR_BASE)	/* 32byte align */
#define CHKSUM2_SIZE	SZ_128M		/* 32byte align */
#else
#define CHKSUM2_AREA	(DDR_BASE+SZ_128M)	/* 32byte align */
#define CHKSUM2_SIZE	SZ_64M			/* 32byte align */
#endif

#if 0
#include <mach/pm_macro-sio.S>
#include <mach/pm_macro-sio-type1.S>
#include <mach/pm_macro-nvram.S>
#endif

#undef CHKSUM_SPEED
#undef CHKSUM_SPEED2

	.macro	pm_workarea_va,rx
	ldr	\rx, =VA_BOOTRAM+CXD4132_SUSPEND_WORKAREA
	.endm
	.macro	pm_workarea_phys,rx
	ldr	\rx, =CXD4132_SRAM_BASE+CXD4132_SUSPEND_WORKAREA
	.endm
	.macro	pm_stack_phys,rx
	ldr	\rx, =CXD4132_SRAM_BASE+CXD4132_SUSPEND_WORKAREA
	.endm

	.macro	pm_outer_cache
#ifdef CONFIG_CACHE_PL310S
	bl	cxd4132_pl310_shutdown		@ r0-r2 corrupted
#endif /* CONFIG_CACHE_PL310S */
	.endm

	.macro	pm_hook,rd
	ldr	r0, pm_chksum
	cmp	r0, #0
	beq	9f
	@ memory check sum
#ifdef CHKSUM_TYPE1
#ifdef CHKSUM_SPEED
	mov	r0, #7
	mcr	p15,0,r0,c15,c12,0	@ PMC start
#endif
#ifdef CHKSUM_SPEED2
	mach_freerun r0			@ timer
	mach_read_cycles r12,r0		@ now
#endif
	mov	r2, #0			@ parity
	ldr	r11, =CHKSUM_AREA
	ldr	r0, =DDR_BASE		@ 32byte align
2:
	@ Outer loop (CHKSUM_AREA times)
	add	r1, r0, #CHKSUM_SIZE	@ 32byte align
	b	1f
	@ if loop is 32byte aligned, it is 25% slow down.
	.align	5
	nop				@ for mis-align loop
1:
	@ Inner loop (CHKSUM_SIZE/32 times)
	ldmia	r0!, {r3-r10}
	cmp	r0, r1
	eor	r2, r2, r3
	eor	r2, r2, r4
	eor	r2, r2, r5
	eor	r2, r2, r6
	eor	r2, r2, r7
	eor	r2, r2, r8
	eor	r2, r2, r9
	eor	r2, r2, r10
	bne	1b
	add	r0, r0, #CHKSUM_LEAP	@ 32byte align
	subs	r11, r11, #1
	bne	2b
	eor	r11, r2, r2, lsl #16
	lsr	r11, r11, #16		@ r11 = CHKSUM
#ifdef CHKSUM_SPEED
	@ cycles
	mrc	p15,0,r5,c15,c12,1	@ r5 = cycles
#endif
#ifdef CHKSUM_SPEED2
	mach_freerun r0			@ timer
	mach_read_cycles r5,r0		@ now
	sub	r5, r5, r12
	mach_cycles_to_uses r5
#endif
	@ display CHKSUM
	mov	r0, r11
	bl	__printhex4
	mov	r0, #'\n'
	bl	__printch
#if defined(CHKSUM_SPEED) || defined(CHKSUM_SPEED2)
	@ display cycles
	mov	r0, r5
	bl	__printhex8
	mov	r0, #'\n'
	bl	__printch
#endif /* CHKSUM_SPEED || CHKSUM_SPEED2 */
#endif /* CHKSUM_TYPE1 */
#ifdef CHKSUM_TYPE2
	@ preserve r11
	ldr	r0, pm_csumaddr
	ldr	r1, pm_csumsize
	add	r1, r0
	cmp	r0, #0
	bne	1f
	ldr	r0, =CHKSUM2_AREA	@ 32byte align
	add	r1, r0, #CHKSUM2_SIZE	@ 32byte align
1:
	mov	r2, #0
	b	1f
	@ if loop is 32byte aligned, it is 25% slow down.
	.align	5
	nop				@ for mis-align loop
1:
	ldmia	r0!, {r3-r10}
	cmp	r0, r1
	eor	r2, r2, r3
	eor	r2, r2, r4
	eor	r2, r2, r5
	eor	r2, r2, r6
	eor	r2, r2, r7
	eor	r2, r2, r8
	eor	r2, r2, r9
	eor	r2, r2, r10
	bne	1b
	eor	r4, r2, r2, lsl #16
	lsr	r4, r4, #16		@ r4 = CHKSUM
	@ display CHKSUM
	mov	r0, r4
	bl	__printhex4
	mov	r0, #'\n'
	bl	__printch
#endif /* CHKSUM_TYPE2 */
#ifdef CHECKSUM_EVAL
	@ merge two CHKSUM
	orr	r11, r4, r11, lsl #16
#endif /* CHECKSUM_EVAL */
#if !defined(CHKSUM_TYPE1) && defined(CHKSUM_TYPE2)
	mov	r11, r4
#endif

#if 0
	@ save CHKSUM to NVRAM
	bl	nvram_open
	cmp	r0, #0
	bne	9f
	mov	r0, r11			@ CHKSUM
	bl	nvram_write
	cmp	r0, #0
	beq	1f
	mov	r0, #'W'
	bl	__printch
1:
	bl	nvram_close
#endif
9:
	.endm

	.macro	pm_hook2,rd
#if 0  /* wait 10sec and XRESET_REQ */
	ldr	r0, =10000000
	bl	__udelay
	@ XRESET_REQ
	ldr	r0, =CXD4132_PERMISC_BASE
	ldr	r1, =PER_RSTREQ_EN|PER_RSTREQ_ASSERT
	str	r1, [r0, #PER_RSTREQ_SET]
	@ wfi
	mov	r0, #0
1:
	mcr	p15, 0, r0, c7, c0, 4		@ wfi
	b	1b
#endif
#if 0  /* SUSPEND RETURN TEST */
# if 0  /* Enter test mode only if cfg.poff=254,x */
	ldr	r0, cxd41xx_xpower_off_addr
	ldr	r1, =PA_GPIO(254)
	cmp	r0, r1				@ test mode ?
	bne	pm_hook2_end
# endif
	ldr	r0, [\rd, #SDC_REF_SET]
	and	r0, r0, #~(SDC_REF_R1|SDC_REF_R0)
	str	r0, [\rd, #SDC_REF_SET]		@ stop auto-refresh
	/* wait about 10sec */
	ldr	r0, =10000000
	bl	__udelay
	/* resume */
	ldr	r1, [\rd, #SDC_REF_SET]
	orr	r0, r0, #(SDC_REF_R1|SDC_REF_R0)
	str	r1, [\rd, #SDC_REF_SET]		@ start auto-refresh
	ldr	r1, =SDC_CMD_SREX_R1|SDC_CMD_SREX_R0
	str	r1, [\rd, #SDC_COMMAND3] 	@ EXIT SRef
	ldr	r0, =cxd4115_cpu_resume
	bic	r0, r0, #0x40000000		@ virt_to_phys
	mov	pc, r0
pm_hook2_end:
#endif /* SUSPEND RETURN TEST */
	.endm

	.macro	pm_reset,rd,rx
	.endm

	.macro	pm_memory,rx
	ldr	\rx, =CXD4132_SDC_BASE
	@ RANK0 PASR
	ldrb	r1, cxd41xx_ddr_pasr0		@ PC relative
	cmp	r1, #0
	beq	1f
	orr	r1, r1, #DDR2_MR17 << 8		@ MR17
	orr	r1, r1, #SDC_DDR2_MRW_R0	@ WRITE
	bl	__sdc_issue2
1:
	@ RANK1 PASR
	ldrb	r1, cxd41xx_ddr_pasr1		@ PC relative
	cmp	r1, #0
	beq	1f
	orr	r1, r1, #DDR2_MR17 << 8		@ MR17
	orr	r1, r1, #SDC_DDR2_MRW_R1	@ WRITE
	bl	__sdc_issue2
1:
	@ DDR ctrl
	ldr	r0, cxd41xx_ddr_ctrl		@ PC relative
	cmp	r0, #DMC_DDRCTRL_NO
	beq	ddr_ctrl_end			@ do not control
	@ set DDRs to SREF or DPD
	ldr	\rx, =CXD4132_SDC_BASE
	mov	r1, #0				@ SREF command word
	mov	r2, #0				@ DPD  command word
	@ RANK0
	ldrb	r0, cxd41xx_ddr_ctrl		@ PC relative
	cmp	r0, #DMC_DDRCTRL_NO
	beq	1f
	cmp	r0, #DMC_DDRCTRL_SREF
	orreq	r1, r1, #SDC_CMD_SREF_R0	@ set RANK0 to SREF
	orrne	r2, r2, #SDC_CMD_DPD_R0		@ set RANK0 to DPD
1:
	ldr	r0, [\rx, #SDC_MEMORY]
	ands	r0, r0, #SDC_MODE2RANK		@ 2RANKs populated ?
	beq	1f
	@ RANK1
	ldrb	r0, cxd41xx_ddr_ctrl+1		@ PC relative
	cmp	r0, #DMC_DDRCTRL_NO
	beq	1f
	cmp	r0, #DMC_DDRCTRL_SREF
	orreq	r1, r1, #SDC_CMD_SREF_R1	@ set RANK1 to SREF
	orrne	r2, r2, #SDC_CMD_DPD_R1		@ set RANK1 to DPD
1:
	bl	__sdc_issue
ddr_ctrl_end:
	@ PRC RESET
	ldr	r1, =CXD4132_HTP_PRC
	mov	r0, #0
	str	r0, [r1, #HTP_PRC_RESET]
	mov	r0, #5
	bl	__udelay			@ wait 5us for PRC_RESET
	.endm

	.macro	pm_functions
	/*
	 * issue SDC_COMMAND
	 *    input:	r4=SDC base address, r1=SREF param, r2=DPD param
	 *    output:	none.
	 *    destruct: r0, r2, r3, r5
	 */
__sdc_issue:
	mov	r5, lr				@ save lr
	bl	__sdc_ready			@ wait for ready
	cmp	r1, #0
	beq	1f
	str	r1, [r4, #SDC_COMMAND3]		@ SREF issue
	bl	__sdc_ready			@ wait for done
1:
	cmp	r2, #0
	beq	1f
	ldr	r0, [r4, #SDC_REF_SET]
	and	r0, r0, #~(SDC_REF_R1|SDC_REF_R0)
	str	r0, [r4, #SDC_REF_SET]		@ stop auto-refresh
	str	r2, [r4, #SDC_COMMAND3]		@ DPD issue
	bl	__sdc_ready			@ wait for done
	mov	r0, #500
	bl	__udelay			@ wait 500us
1:
	mov	pc, r5

	/*
	 * Wait for SDC_COMMAND3 register ready
	 *    input:	r4=SDC base address
	 *    output:	none.
	 *    destruct:	r0, r3
	 */
__sdc_ready:
	ldr	r3, =SDC_CMD_BUSY
1:
	ldr	r0, [r4, #SDC_COMMAND3]
	ands	r0, r0, r3
	bne	1b
	mov	pc, lr

	/*
	 * issue SDC_COMMAND2
	 *    input:	r4=SDC base address, r1=COMMAND
	 *    output:	r0=SDC_COMMAND2
	 *    destruct: r0, r5
	 */
__sdc_issue2:
	mov	r5, lr				@ save lr
	bl	__sdc_ready2			@ wait for ready
	str	r1, [r4, #SDC_COMMAND2]		@ issue
	bl	__sdc_ready2			@ wait for done
	ldr	r0, [r4, #SDC_COMMAND2]		@ result
	mov	pc, r5

	/*
	 * Wait for SDC_COMMAND2 register ready
	 *    input:	r4=SDC base address
	 *    output:	none.
	 *    destruct:	r0, r3
	 */
__sdc_ready2:
	ldr	r3, =SDC_DDR2_BUSY
1:
	ldr	r0, [r4, #SDC_COMMAND2]
	ands	r0, r0, r3
	bne	1b
	mov	pc, lr

	.globl	pm_chksum
pm_chksum:	.long 	0
	.globl	pm_csumaddr
pm_csumaddr:	.long 	0
	.globl	pm_csumsize
pm_csumsize:	.long 	0

#if 0
	pm_sio
	pm_sio_type1
	pm_nvram
#endif
	.endm
