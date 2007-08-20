#ifndef _ARMV_H_
#define _ARMV_H_

#define USR26_MODE	(0x00)
#define FIQ26_MODE	(0x01)
#define IRQ26_MODE	(0x02)
#define SVC26_MODE	(0x03)
#define USR_MODE	(0x10)
#define FIQ_MODE	(0x11)
#define IRQ_MODE	(0x12)
#define SVC_MODE	(0x13)
#define ABT_MODE	(0x17)
#define UND_MODE	(0x1b)
#define SYSTEM_MODE	(0x1f)
#define MODE_MASK	(0x1f)
#define F_BIT		(0x40)
#define I_BIT		(0x80)
#define CC_V_BIT	(1 << 28)
#define CC_C_BIT	(1 << 29)
#define CC_Z_BIT	(1 << 30)
#define CC_N_BIT	(1 << 31)

/* for MMU */
#define CP15_1_MMU	(1 << 0)	/* MMU */
#define CP15_1_ALIGN	(1 << 1)	/* alignment fault */
#define CP15_1_DCACHE	(1 << 2)	/* Data Cache */
#define CP15_1_NOP	0x78		/* Read/Write as 0b1111 */
#define CP15_1_SYSP	(1 << 8)	/* System protection */
#define CP15_1_ROMP	(1 << 9)	/* ROM protection */
#define CP15_1_BTB	(1 << 11)	/* Branch Target Buffer */
#define CP15_1_ICACHE	(1 << 12)	/* Instruction Cache */
#define CP15_1_VECTREL	(1 << 13)	/* Exception Vector Relocation */

#endif /* _ARMV_H_ */
