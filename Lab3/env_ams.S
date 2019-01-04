#include "regdef.h"
#include "cp0regdef.h"
#include "asm.h"
#include "trap.h"

.data
.global KERNEL_SP;
KERNEL_SP:	.word	0

.text
.set noreorder
LEAF(env_pop_tf)
		.set    mips1
        nop
        move    k0,a0				/* k0 为当前进程 Trapframe 的地址 */
        mtc0    a1,CP0_ENTRYHI		/* 将 curenv 的 TLB 索引赋值给 CP0_ENTRYHI */

        mfc0    t0,CP0_STATUS		/* 将 CP0_STATUS 后两位清零 */
        ori     t0,0x3
        xori    t0,0x3
        mtc0    t0,CP0_STATUS

		lw      v1,TF_LO(k0)		/* 恢复 HI LO EPC */
        mtlo    v1
        lw      v0,TF_HI(k0)
        lw      v1,TF_EPC(k0)
        mthi    v0
        mtc0    v1,CP0_EPC

        lw      $31,TF_REG31(k0)	/* 恢复所有寄存器 */
        lw      $30,TF_REG30(k0)
        lw      $29,TF_REG29(k0)
        lw      $28,TF_REG28(k0)
        lw      $25,TF_REG25(k0)
        lw      $24,TF_REG24(k0)
        lw      $23,TF_REG23(k0)
        lw      $22,TF_REG22(k0)
        lw      $21,TF_REG21(k0)
        lw      $20,TF_REG20(k0)
        lw      $19,TF_REG19(k0)
        lw      $18,TF_REG18(k0)
        lw      $17,TF_REG17(k0)
        lw      $16,TF_REG16(k0)
        lw      $15,TF_REG15(k0)
        lw      $14,TF_REG14(k0)
        lw      $13,TF_REG13(k0)
        lw      $12,TF_REG12(k0)
        lw      $11,TF_REG11(k0)
        lw      $10,TF_REG10(k0)
        lw      $9,TF_REG9(k0)
        lw      $8,TF_REG8(k0)
        lw      $7,TF_REG7(k0)
        lw      $6,TF_REG6(k0)
        lw      $5,TF_REG5(k0)
        lw      $4,TF_REG4(k0)
        lw      $3,TF_REG3(k0)
        lw      $2,TF_REG2(k0)
        lw      $1,TF_REG1(k0)

        lw      k1,TF_PC(k0)		/* 提取 Trapframe 中的 PC */

        lw      k0,TF_STATUS(k0)	/* 恢复 CP0_STATUS */
        nop
        mtc0    k0,CP0_STATUS

        j       k1					/* 跳转到 Trapframe 中的 PC */
        rfe
        nop
END(env_pop_tf)


LEAF(lcontext)
        .extern mCONTEXT			/* mCONTEXT 为 boot_pgdir */
        sw		a0, mCONTEXT		/* 将当前进程的页目录物理地址 写入 mCONTEXT */
        jr      ra
        nop
END(lcontext)

