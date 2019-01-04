#ifndef _MMU_H_
#define _MMU_H_

/* ���İ���:
*      Part 1.  MIPS ����.
*      Part 2.  �ڴ��׼.
*      Part 3.  ��������.
*/

/********************************* MIPS ���� *************************************/

#define BY2PG           4096            // һҳӳ���4KB
#define PDMAP           (4*1024*1024)   // һ��ҳĿ¼��ӳ���4MB
#define PGSHIFT         12				// ҳ��ƫ��λ�� log2(BY2PG)
#define PDSHIFT         22              // ҳĿ¼��ƫ��λ�� log2(PDMAP)
#define PTE2PT          1024			// ҳ������1024��ҳ����
#define PDX(va)         ((((u_long)(va))>>22) & 0x03FF)	// ��ȡ���ַ va ��Ӧ��ҳĿ¼������
#define PTX(va)         ((((u_long)(va))>>12) & 0x03FF)	// ��ȡ���ַ va ��Ӧ��ҳ��������
#define PTE_ADDR(pte)   ((u_long)(pte) & ~0xFFF)		// ��Ȩ��λ����

// ҳ��ŵĵ�ַ��
#define PPN(va)         (((u_long)(va))>>12)	// ��ȡ���ַ va ��Ӧ��ҳ�� (ǰ20λ)
#define VPN(va)         PPN(va)					// ��ȡ���ַ va ��Ӧ��ҳ�� (ǰ20λ)
#define VA2PFN(va)      (((u_long)(va)) & 0xFFFFF000 )	// ��ȡ���ַ va ��Ӧ��ҳ�����ַ (ҳ��ƫ��Ϊ0)
#define VA2PDE(va)      (((u_long)(va)) & 0xFFC00000 )	// ��ȡ���ַ va ��Ӧ��ҳĿ¼�����ַ (ҳ����������ҳ��ƫ��Ϊ0)

/* ҳ����/ҳĿ¼�� flag */
#define PTE_G           0x0100  // ȫ��λ
#define PTE_V           0x0200  // ��Чλ
#define PTE_R           0x0400  // ��λ, Ϊ 0 ʱδ���Ĺ�
#define PTE_D           0x0002  // �ļ�ϵͳ���汻�޸�
#define PTE_COW         0x0001  // Copy On Write
#define PTE_UC          0x0800  // δ������
#define PTE_LIBRARY     0x0004  // �����ڴ�




/********************************* �ڴ��׼ *************************************/

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

#define TIMESTACK 0x82000000		//ʱ��ջջ��

#define VPT (ULIM + PDMAP )			//�ں�ҳ�����ַ
#define KSTACKTOP (VPT-0x100)		//�ں�ջ��
#define KSTKSIZE (8*BY2PG)			//�ں�ջ��С(8��ҳ��)
#define KERNBASE 0x80010000			//�ں˻���ַ

#define ULIM 0x80000000				//�û�������
#define UVPT (ULIM - PDMAP)			//�û�ҳ�����ַ
#define UVPDIR 0x7fdff000			//�û�ҳĿ¼�Ļ���ַ
#define UPAGES (UVPT - PDMAP)		//�û�ҳ����
#define UENVS (UPAGES - PDMAP)		//�û�����������ַ

#define UTOP UENVS					//�û����򶥶�
#define UXSTACKTOP UTOP				//�û��쳣ջ��
#define USTACKTOP (UTOP - 2*BY2PG)	//�û�ջ��
#define UTEXT 0x00400000			//�û������


// �ں˴�����
#define E_UNSPECIFIED   1       // δָ����δ֪����
#define E_BAD_ENV       2       // ���̲�����
// ������������ʹ��
#define E_INVAL         3       // �Ƿ�����
#define E_NO_MEM        4       // �ڴ治��, ����ʧ��
#define E_NO_FREE_ENV   5       // ���н��̲���, ����ʧ��
// ��������ֵ
#define E_IPC_NOT_RECV  6       // ���͸������յĽ���

// �ļ�ϵͳ���� -- ֻ���û���
#define E_NO_DISK       7       // ������û��ʣ��ռ�
#define E_MAX_OPEN      8       // ���˹�����ļ�
#define E_NOT_FOUND     9       // �Ҳ����ļ�
#define E_BAD_PATH      10      // ����·��
#define E_FILE_EXISTS   11      // �ļ��Ѵ���
#define E_NOT_EXEC      12      // �ļ�����ִ��

#define MAXERROR 12




/********************************* �������� *************************************/
#ifndef __ASSEMBLER__
#include "types.h"

void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);

extern char bootstacktop[], bootstack[];

extern u_long npage;	// �ڴ��С(����ҳ)

typedef u_long Pde;		// ҳĿ¼��
typedef u_long Pte;		// ҳ����

extern volatile Pte* vpt[];		//ҳ��
extern volatile Pde* vpd[];		//ҳĿ¼

// �� kseg0 ���ַ kva ת��Ϊ�����ַ
#define PADDR(kva)                                              \
        ({                                                              \
                u_long a = (u_long) (kva);                              \
                if (a < ULIM)                                   \
                        panic("PADDR called with invalid kva %08lx", a);\
                a - ULIM;                                               \
        })

// �������ַ pa ת��Ϊ kseg0 ���ַ
#define KADDR(pa)                                               \
        ({                                                              \
                u_long ppn = PPN(pa);                                   \
                if (ppn >= npage)                                       \
                        panic("KADDR called with invalid pa %08lx", (u_long)pa);\
                (pa) + ULIM;                                    \
        })

// ���Ժ�, ʧ�����: assertion failed
#define assert(x)       \
        do {    if (!(x)) panic("assertion failed: %s", #x); } while (0)

// ��� _p �����û�������, ��Ϊ�û�������
#define TRUP(_p)                                                \
        ({                                                              \
                register typeof((_p)) __m_p = (_p);                     \
                (u_int) __m_p > ULIM ? (typeof(_p)) ULIM : __m_p;       \
        })


extern void tlb_out(u_int entryhi);

#endif //!__ASSEMBLER__
#endif // !_MMU_H_
