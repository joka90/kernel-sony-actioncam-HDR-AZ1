/* 2010-11-29: File added by Sony Corporation */
/*
 * PARTNER-Jet Linux support patch by KMC
 *
 * Ver2.0	08.04.14
 *
 */



    asm("	.text");
    asm("	.align	2");
    asm("	.global	__kmc_code_data");
    asm("__kmc_code_data:");
    asm("	movm	[other],(sp)");
    asm("	mov	__kmc_code_data_table,a1");
    asm("	mov	4,d1");
    asm("	mov	d0,a0");
    asm("	and	0xff,a0");
    asm("	asl2	a0");
    asm("	add	a1,a0");
    asm("	jmp	(a0)");
    asm("	nop");
    asm("	.align	2");
    asm("__kmc_code_data_table:");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("	jmp	jmp_end");
    asm("	.align	2");
    asm("jmp_end:");
    asm("	add	-1,d1");
    asm("	beq	jmp_end1");
    asm("	lsr     8,d0");
    asm("	mov	d0,a0");
    asm("	and	0xff,a0");
    asm("	asl2	a0");
    asm("	add	a1,a0");
    asm("	jmp	(a0)");
    asm("jmp_end1:");
    asm("	movm    (sp),[other]");
    asm("	ret    [],0");


