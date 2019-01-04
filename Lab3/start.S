#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

.section
.text
.exc_vec3
NESTED(except_vec3, 0, sp)
		.set noat 
		.set noreorder
1:
		mfc0	k1, CP0_CAUSE		/* 取得异常码 */

		la		k0, exception_handlers	/* 找到对应的中断处理函数 */
		andi	k1, 0x7c
		addu	k0, k1
		lw		k0, (k0)

		NOP
		jr		k0		/* 跳转到对应的中断处理函数中 */
		nop
END(except_vec3)
.set at


.data
.globl mCONTEXT
mCONTEXT:	.word 0

.globl delay
delay:		.word 0

.globl tlbra
tlbra:		.word 0

.section .data .stk
KERNEL_STACK:	.space 0x8000

.text
LEAF(_start)
        .set    mips2
        .set    reorder

        /* Disable interrupts */
        mtc0    zero, CP0_STATUS

        /* Disable watch exception. */
        mtc0    zero, CP0_WATCHLO
        mtc0    zero, CP0_WATCHHI

        /* disable kernel mode cache */
        mfc0    t0, CP0_CONFIG
        and     t0, ~0x7
        ori     t0, 0x2
        mtc0    t0, CP0_CONFIG

        /* set up stack */
        li      sp, 0x80400000
        li		t0, 0x80400000
        sw		t0, mCONTEXT

        /* jump to main */
        jal     main

loop:
        j       loop
        nop

END(_start)
