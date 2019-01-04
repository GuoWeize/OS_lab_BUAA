#ifndef _TRAP_H_
#define _TRAP_H_

/* these are processor defined */
#define T_DIVIDE     0    /* divide error */
#define T_DEBUG      1    /* debug exception */
#define T_NMI        2    /* non-maskable interrupt */
#define T_BRKPT      3    /* breakpoint */
#define T_OFLOW      4    /* overflow */
#define T_BOUND      5    /* bounds check */
#define T_ILLOP      6    /* illegal opcode */
#define T_DEVICE     7    /* device not available */ 
#define T_DBLFLT     8    /* double fault */
/* 9 is reserved */
#define T_TSS       10    /* invalid task switch segment */
#define T_SEGNP     11    /* segment not present */
#define T_STACK     12    /* stack exception */
#define T_GPFLT     13    /* genernal protection fault */
#define T_PGFLT     14    /* page fault */
/* 15 is reserved */
#define T_FPERR     16    /* floating point error */
#define T_ALIGN     17    /* aligment check */
#define T_MCHK      18    /* machine check */

/* These are arbitrarily chosen, but with care not to overlap
* processor defined exceptions or interrupt vectors.
*/
#define T_SYSCALL   0x30 /* system call */
#define T_DEFAULT   500  /* catchall */

#ifndef __ASSEMBLER__

#include "types.h"

struct Trapframe {
	// 保存主要的寄存器
	unsigned long regs[32];
	// 保存特殊的寄存器
	unsigned long cp0_status;
	unsigned long hi;
	unsigned long lo;
	unsigned long cp0_badvaddr;
	unsigned long cp0_cause;
	unsigned long cp0_epc;
	unsigned long pc;
};
void *set_except_vector(int n, void * addr);
void trap_init();

#endif /* !__ASSEMBLER__ */


/* 全部异常的栈空间布局:
   ptrace 需要将所有寄存器保存到栈里, 如果这里的顺序改变了, 需要更新 include/asm-mips/ptrace.h
   第一个 5*PTRSIZE 字节保存 C 子程序的参数 */
#define TF_REG0         0
#define TF_REG1         ((TF_REG0) + 4)
#define TF_REG2         ((TF_REG1) + 4)
#define TF_REG3         ((TF_REG2) + 4)
#define TF_REG4         ((TF_REG3) + 4)
#define TF_REG5         ((TF_REG4) + 4)
#define TF_REG6         ((TF_REG5) + 4)
#define TF_REG7         ((TF_REG6) + 4)
#define TF_REG8         ((TF_REG7) + 4)
#define TF_REG9         ((TF_REG8) + 4)
#define TF_REG10        ((TF_REG9) + 4)
#define TF_REG11        ((TF_REG10) + 4)
#define TF_REG12        ((TF_REG11) + 4)
#define TF_REG13        ((TF_REG12) + 4)
#define TF_REG14        ((TF_REG13) + 4)
#define TF_REG15        ((TF_REG14) + 4)
#define TF_REG16        ((TF_REG15) + 4)
#define TF_REG17        ((TF_REG16) + 4)
#define TF_REG18        ((TF_REG17) + 4)
#define TF_REG19        ((TF_REG18) + 4)
#define TF_REG20        ((TF_REG19) + 4)
#define TF_REG21        ((TF_REG20) + 4)
#define TF_REG22        ((TF_REG21) + 4)
#define TF_REG23        ((TF_REG22) + 4)
#define TF_REG24        ((TF_REG23) + 4)
#define TF_REG25        ((TF_REG24) + 4)
// $26 (k0) 和 $27 (k1) 不保存
#define TF_REG26        ((TF_REG25) + 4)
#define TF_REG27        ((TF_REG26) + 4)
#define TF_REG28        ((TF_REG27) + 4)
#define TF_REG29        ((TF_REG28) + 4)
#define TF_REG30        ((TF_REG29) + 4)
#define TF_REG31        ((TF_REG30) + 4)

#define TF_STATUS       ((TF_REG31) + 4)

#define TF_HI           ((TF_STATUS) + 4)
#define TF_LO           ((TF_HI) + 4)

#define TF_BADVADDR     ((TF_LO)+4)
#define TF_CAUSE        ((TF_BADVADDR) + 4)
#define TF_EPC          ((TF_CAUSE) + 4)
#define TF_PC           ((TF_EPC) + 4)

// Trapframe 的大小, word/double word 对齐取整
#define TF_SIZE         ((TF_PC)+4)


#endif /* _TRAP_H_ */
