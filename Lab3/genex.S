#include "regdef.h"
#include "cp0regdef.h"
#include "asm.h"
#include "stackframe.h"

.macro  __build_clear_sti
        STI
.endm


.macro  __build_clear_cli
        CLI
.endm


.macro  BUILD_HANDLER exception handler clear
		.align  5
        NESTED(handle_\exception, TF_SIZE, sp)
				.set    noat
				nop
				SAVE_ALL
				__build_clear_\clear
				.set    at
				move    a0, sp
				jal     \handler
				nop
				j       ret_from_exception
				nop
		END(handle_\exception)
.endm


FEXPORT(ret_from_exception)
		.set	noat
		.set	noreorder
		RESTORE_SOME
		.set	at
        lw      k0,TF_EPC(sp)
        lw      sp,TF_REG29(sp)		/* Deallocate stack */
        jr      k0
        rfe


/* 时钟中断处理函数 */
.set	noreorder
.align  5
NESTED(handle_int, TF_SIZE, sp)
		.set    noat
		
		SAVE_ALL
		CLI
		.set    at
		mfc0    t0, CP0_CAUSE
		mfc0    t2, CP0_STATUS
		and     t0, t2

		andi    t1, t0, STATUSF_IP4		/* 判断是不是对应的 4 号中断位引发的中断 */
		bnez    t1, timer_irq			/* 如果是, 执行 timer_irq */
		nop
END(handle_int)


/* 中断服务函数 */
.extern delay
timer_irq:
1:      j       sched_yield			/* 跳转到调度函数 sched_yield */
        nop
        j       ret_from_exception
        nop


LEAF(do_reserved)
END(do_reserved)


.extern tlbra
.set    noreorder
NESTED(do_refill, 0, sp)
		.extern mCONTEXT
1:      nop
        lw		k1,mCONTEXT
        and		k1,0xfffff000
        mfc0	k0,CP0_BADVADDR
        srl		k0,20
        and		k0,0xfffffffc
        addu	k0,k1

        lw		k1,0(k0)
        nop
        move	t0,k1
        and		t0,0x0200
        beqz	t0,NOPAGE
        nop
        and		k1,0xfffff000
		mfc0	k0,CP0_BADVADDR
		srl		k0,10
		and		k0,0xfffffffc
		and		k0,0x00000fff
        addu	k0,k1

        or		k0,0x80000000
        lw		k1,0(k0)
        nop
		move	t0,k1
		and		t0,0x0200
		beqz	t0,NOPAGE
        nop
        move	k0,k1
        and		k0,0x1
        beqz	k0,NoCOW
        nop
        and		k1,0xfffffbff
NoCOW:
        mtc0	k1,CP0_ENTRYLO0
        nop
        tlbwr

        j		2f
        nop
NOPAGE:
		nop
        mfc0	a0,CP0_BADVADDR
        lw		a1,mCONTEXT
        nop

        sw		ra,tlbra
        jal		pageout
        nop
        lw		ra,tlbra
        nop

        j		1b
2:		nop

        jr		ra
        nop
END(do_refill)



BUILD_HANDLER reserved	do_reserved		cli
BUILD_HANDLER tlb       do_refill       cli
BUILD_HANDLER mod       page_fault_handler cli
