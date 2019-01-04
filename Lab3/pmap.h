#ifndef _PMAP_H_
#define _PMAP_H_

#include "types.h"
#include "queue.h"
#include "mmu.h"
#include "printf.h"

typedef LIST_HEAD(Page_list, Page);
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
	/* 空闲链表 */
	Page_LIST_entry_t pp_link;
	/* 引用计数是指向这个页面的指针的个数
	只有用 page_alloc 分配的页面才有, 在 boot 阶段使用 pmap.c 里的 alloc 分配的页面 没有 有效的引用计数*/
	u_short pp_ref;
};

extern struct Page *pages;


/* 获取页面的序号 */
static inline u_long
page2ppn(struct Page *pp) {
	return pp - pages;
}


/* 获取页面 pp 的物理地址 */
static inline u_long
page2pa(struct Page *pp) {
	return page2ppn(pp) << PGSHIFT;
}


/* 获取物理地址为 pa 的页面 */
static inline struct Page *
pa2page(u_long pa) {
	if (PPN(pa) >= npage) {
		panic("pa2page called with invalid pa: %x", pa);
	}

	return &pages[PPN(pa)];
}


/* 获取页面 pp 的内核虚地址 */
static inline u_long
page2kva(struct Page *pp) {
	return KADDR(page2pa(pp));
}


/* 将虚地址 va 转换为物理地址 */
static inline u_long
va2pa(Pde *pgdir, u_long va) {
	Pte *p;	//页表基址 虚

	//将页目录基址加上页目录项索引, 得到对应的页目录项
	pgdir = &pgdir[PDX(va)];	//pgdir = pgdir + PDX(va)

	if (!(*pgdir & PTE_V)) {
		return ~0;
	}

	//取出页目录项, 将权限位清零, 转为内核虚地址, 得到页表基地址 虚
	p = (Pte *)KADDR(PTE_ADDR(*pgdir));

	if (!(p[PTX(va)] & PTE_V)) {
		return ~0;
	}

	//先将页表基址加上页表项索引, 得到物理页面基址, 然后取出内容, 最后将权限位清零
	return PTE_ADDR(p[PTX(va)]);
}

/********** 内存管理函数(参见pmap.c中的实现) ***********/

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
