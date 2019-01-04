#ifndef __ASM_MIPS_REGDEF_H
#define __ASM_MIPS_REGDEF_H

/* Symbolic register names for 32 bit ABI */
#define zero    $0      /* 永远为0 */
#define AT      $1      /* 编译器临时变量 */
#define v0      $2      /* 返回值 */
#define v1      $3
#define a0      $4      /* 参数寄存器 */
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8      
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16    
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24    
#define t9      $25
#define jp      $25     /* PIC 跳转寄存器 */
#define k0      $26     /* 内核寄存器 0 */
#define k1      $27		/* 内核寄存器 1 */
#define gp      $28     /* 全局指针 */
#define sp      $29     /* 栈指针 */
#define fp      $30     /* 帧指针 */
#define s8      $30     /* 与 fp 类似 */
#define ra      $31     /* 返回地址 */

#endif /* __ASM_MIPS_REGDEF_H */
