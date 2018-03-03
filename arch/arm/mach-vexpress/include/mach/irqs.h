/* 2011-10-25: File changed by Sony Corporation */
#define IRQ_LOCALTIMER		29
#define IRQ_LOCALWDOG		30

#ifdef CONFIG_ARCH_VEXPRESS_CA5X2
#define NR_IRQS	256
#else
#define NR_IRQS	128
#endif
