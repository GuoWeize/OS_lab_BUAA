#ifndef _PMAP_H_
#define _PMAP_H_

#include "types.h"
#include "queue.h"
#include "mmu.h"
#include "printf.h"

typedef LIST_HEAD(Page_list, Page);
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
	/* �������� */
	Page_LIST_entry_t pp_link;
	/* ���ü�����ָ�����ҳ���ָ��ĸ���
	ֻ���� page_alloc �����ҳ�����, �� boot �׶�ʹ�� pmap.c ��� alloc �����ҳ�� û�� ��Ч�����ü���*/
	u_short pp_ref;
};

extern struct Page *pages;


/* ��ȡҳ������ */
static inline u_long
page2ppn(struct Page *pp) {
	return pp - pages;
}


/* ��ȡҳ�� pp �������ַ */
static inline u_long
page2pa(struct Page *pp) {
	return page2ppn(pp) << PGSHIFT;
}


/* ��ȡ�����ַΪ pa ��ҳ�� */
static inline struct Page *
pa2page(u_long pa) {
	if (PPN(pa) >= npage) {
		panic("pa2page called with invalid pa: %x", pa);
	}

	return &pages[PPN(pa)];
}


/* ��ȡҳ�� pp ���ں����ַ */
static inline u_long
page2kva(struct Page *pp) {
	return KADDR(page2pa(pp));
}


/* �����ַ va ת��Ϊ�����ַ */
static inline u_long
va2pa(Pde *pgdir, u_long va) {
	Pte *p;	//ҳ���ַ ��

	//��ҳĿ¼��ַ����ҳĿ¼������, �õ���Ӧ��ҳĿ¼��
	pgdir = &pgdir[PDX(va)];	//pgdir = pgdir + PDX(va)

	if (!(*pgdir & PTE_V)) {
		return ~0;
	}

	//ȡ��ҳĿ¼��, ��Ȩ��λ����, תΪ�ں����ַ, �õ�ҳ�����ַ ��
	p = (Pte *)KADDR(PTE_ADDR(*pgdir));

	if (!(p[PTX(va)] & PTE_V)) {
		return ~0;
	}

	//�Ƚ�ҳ���ַ����ҳ��������, �õ�����ҳ���ַ, Ȼ��ȡ������, ���Ȩ��λ����
	return PTE_ADDR(p[PTX(va)]);
}

/********** �ڴ������(�μ�pmap.c�е�ʵ��) ***********/

void mips_detect_memory();
void mips_vm_init();

void mips_init();
void page_init(void);
int page_alloc(struct Page **pp);
void page_free(struct Page *pp);
void page_decref(struct Page *pp);
int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte);
int page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm);
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte);
void page_remove(Pde *pgdir, u_long va);
void tlb_invalidate(Pde *pgdir, u_long va);

void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm);

extern struct Page *pages;


#endif /* _PMAP_H_ */
