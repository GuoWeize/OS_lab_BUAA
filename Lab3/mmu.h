#ifndef _MMU_H_
#define _MMU_H_

/* 本文包括:
*      Part 1.  MIPS 定义.
*      Part 2.  内存标准.
*      Part 3.  辅助函数.
*/

/********************************* MIPS 定义 *************************************/

#define BY2PG           4096            // 一页映射的4KB
#define PDMAP           (4*1024*1024)   // 一个页目录项映射的4MB
#define PGSHIFT         12				// 页内偏移位数 log2(BY2PG)
#define PDSHIFT         22              // 页目录项偏移位数 log2(PDMAP)
#define PTE2PT          1024			// 页表中有1024个页表项
#define PDX(va)         ((((u_long)(va))>>22) & 0x03FF)	// 获取虚地址 va 对应的页目录项索引
#define PTX(va)         ((((u_long)(va))>>12) & 0x03FF)	// 获取虚地址 va 对应的页表项索引
#define PTE_ADDR(pte)   ((u_long)(pte) & ~0xFFF)		// 将权限位清零

// 页面号的地址域
#define PPN(va)         (((u_long)(va))>>12)	// 获取虚地址 va 对应的页号 (前20位)
#define VPN(va)         PPN(va)					// 获取虚地址 va 对应的页号 (前20位)
#define VA2PFN(va)      (((u_long)(va)) & 0xFFFFF000 )	// 获取虚地址 va 对应的页面基地址 (页内偏移为0)
#define VA2PDE(va)      (((u_long)(va)) & 0xFFC00000 )	// 获取虚地址 va 对应的页目录项基地址 (页表项索引和页内偏移为0)

/* 页表项/页目录项 flag */
#define PTE_G           0x0100  // 全局位
#define PTE_V           0x0200  // 有效位
#define PTE_R           0x0400  // 脏位, 为 0 时未被改过
#define PTE_D           0x0002  // 文件系统缓存被修改
#define PTE_COW         0x0001  // Copy On Write
#define PTE_UC          0x0800  // 未被缓存
#define PTE_LIBRARY     0x0004  // 共享内存




/********************************* 内存标准 *************************************/

/*
o     4G ----------->  +----------------------------+-----------0x10000 0000
o                      |       ...                  |  kseg3
o                      +----------------------------+------------0xe000 0000
o                      |       ...                  |  kseg2
o                      +----------------------------+------------0xc000 0000
o                      |   Interrupts & Exception   |  kseg1
o                      +----------------------------+------------0xa000 0000
o                      |      Invalid memory        |   /|\
o                      +----------------------------+----|-------Physics Memory Max
o                      |       ...                  |  kseg0
o  VPT,KSTACKTOP-----> +----------------------------+----|-------0x8040 0000-------end
o                      |       Kernel Stack         |    | KSTKSIZE            /|\
o                      +----------------------------+----|------                |
o                      |       Kernel Text          |    |                    PDMAP
o      KERNBASE -----> +----------------------------+----|-------0x8001 0000    |
o                      |   Interrupts & Exception   |   \|/                    \|/
o      ULIM     -----> +----------------------------+------------0x8000 0000-------
o                      |         User VPT           |     PDMAP                /|\
o      UVPT     -----> +----------------------------+------------0x7fc0 0000    |
o                      |         PAGES              |     PDMAP                 |
o      UPAGES   -----> +----------------------------+------------0x7f80 0000    |
o                      |         ENVS               |     PDMAP                 |
o  UTOP,UENVS   -----> +----------------------------+------------0x7f40 0000    |
o  UXSTACKTOP -/       |     user exception stack   |     BY2PG                 |
o                      +----------------------------+------------0x7f3f f000    |
o                      |       Invalid memory       |     BY2PG                 |
o      USTACKTOP ----> +----------------------------+------------0x7f3f e000  kuseg
o                      |     normal user stack      |     BY2PG                 |
o                      +----------------------------+------------0x7f3f d000    |
o                      |                            |                           |
o                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                           |
o                      .                            .                           |
o                      .                            .                           |
o                      .                            .                           |
o                      |~~~~~~~~~~~~~~~~~~~~~~~~~~~~|                           |
o                      |                            |                           |
o       UTEXT   -----> +----------------------------+------------0x0040 0000    |
o                      |                            |    2 * PDMAP             \|/
o     0 ------------>  +----------------------------+ -----------0x0000 0000-------
o
*/

#define TIMESTACK 0x82000000		//时间栈栈顶

#define VPT (ULIM + PDMAP )			//内核页表基地址
#define KSTACKTOP (VPT-0x100)		//内核栈顶
#define KSTKSIZE (8*BY2PG)			//内核栈大小(8个页面)
#define KERNBASE 0x80010000			//内核基地址

#define ULIM 0x80000000				//用户区上限
#define UVPT (ULIM - PDMAP)			//用户页表基地址
#define UVPDIR 0x7fdff000			//用户页目录的基地址
#define UPAGES (UVPT - PDMAP)		//用户页面区
#define UENVS (UPAGES - PDMAP)		//用户进程区基地址

#define UTOP UENVS					//用户区域顶端
#define UXSTACKTOP UTOP				//用户异常栈顶
#define USTACKTOP (UTOP - 2*BY2PG)	//用户栈顶
#define UTEXT 0x00400000			//用户代码段


// 内核错误码
#define E_UNSPECIFIED   1       // 未指明或未知错误
#define E_BAD_ENV       2       // 进程不存在
// 不能在请求活动中使用
#define E_INVAL         3       // 非法参数
#define E_NO_MEM        4       // 内存不足, 请求失败
#define E_NO_FREE_ENV   5       // 空闲进程不足, 创建失败
// 允许的最大值
#define E_IPC_NOT_RECV  6       // 发送给不接收的进程

// 文件系统错误 -- 只在用户级
#define E_NO_DISK       7       // 磁盘上没有剩余空间
#define E_MAX_OPEN      8       // 打开了过多的文件
#define E_NOT_FOUND     9       // 找不到文件
#define E_BAD_PATH      10      // 错误路径
#define E_FILE_EXISTS   11      // 文件已存在
#define E_NOT_EXEC      12      // 文件不可执行

#define MAXERROR 12




/********************************* 辅助函数 *************************************/
#ifndef __ASSEMBLER__
#include "types.h"

void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);

extern char bootstacktop[], bootstack[];

extern u_long npage;	// 内存大小(多少页)

typedef u_long Pde;		// 页目录项
typedef u_long Pte;		// 页表项

extern volatile Pte* vpt[];		//页表
extern volatile Pde* vpd[];		//页目录

// 将 kseg0 虚地址 kva 转换为物理地址
#define PADDR(kva)                                              \
        ({                                                              \
                u_long a = (u_long) (kva);                              \
                if (a < ULIM)                                   \
                        panic("PADDR called with invalid kva %08lx", a);\
                a - ULIM;                                               \
        })

// 将物理地址 pa 转换为 kseg0 虚地址
#define KADDR(pa)                                               \
        ({                                                              \
                u_long ppn = PPN(pa);                                   \
                if (ppn >= npage)                                       \
                        panic("KADDR called with invalid pa %08lx", (u_long)pa);\
                (pa) + ULIM;                                    \
        })

// 断言宏, 失败输出: assertion failed
#define assert(x)       \
        do {    if (!(x)) panic("assertion failed: %s", #x); } while (0)

// 如果 _p 大于用户区上限, 则为用户区上限
#define TRUP(_p)                                                \
        ({                                                              \
                register typeof((_p)) __m_p = (_p);                     \
                (u_int) __m_p > ULIM ? (typeof(_p)) ULIM : __m_p;       \
        })


extern void tlb_out(u_int entryhi);

#endif //!__ASSEMBLER__
#endif // !_MMU_H_
