#include "mmu.h"
#include "pmap.h"
#include "printf.h"
#include "env.h"
#include "error.h"

/* 这些变量由 mips_detect_memory() 设置 */
u_long maxpa;            /* 最大物理地址 */
u_long npage;            /* 页面总数 */
u_long basemem;          /* base memory 大小(多少字节) */
u_long extmem;           /* extended memory 大小(多少字节) */

Pde *boot_pgdir;			//初始页目录项

struct Page *pages;			//Page数组
static u_long freemem;		//空闲内存顶端

static struct Page_list page_free_list; /* 空闲页面链表 */


/* 初始化 basemem 和 npage
设置 basemem 为64MB, 并计算相应的 npage 值 */
void 
mips_detect_memory() {
	/* Step 1: 初始化 basemem. (当使用真实计算机时, CMOS 告诉我们有多少KB). */
	maxpa = 67108864;	//64MB
	basemem = 67108864;	//64MB
	extmem = 0;

	// Step 2: 计算相应的 npage
	npage = basemem / BY2PG;

	printf("Physical memory: %dK available, ", (int)(maxpa / 1024));
	printf("base = %dK, extended = %dK\n", (int)(basemem / 1024), (int)(extmem / 1024));
}

/* 只用于操作系统刚刚启动时
按照参数 align 进行取整后, 分配 n 字节大小的物理内存. 如果参数 clear 为真, 将新分配的内存全部清零
如果内存耗尽, 需要 panic; 否则返回新分配的内存的首地址*/
static void *
alloc(u_int n, u_int align, int clear) {
	extern char end[];		//0x80400000
	u_long alloced_mem;		//当前分配的空间

	/* 如果是初次, 初始化 freemem 为连接器不分配任何内核代码或全局变量的第一个虚拟地址 */
	if (freemem == 0) {
		freemem = (u_long)end;
	}

	/* Step 1: 对 freemem 取整 */
	freemem = ROUND(freemem, align);

	/* Step 2: 保存 freemem 当前的值作为分配的内存的基地址 */
	alloced_mem = freemem;

	/* Step 3: 增加 freemem 来记录内存分配, freemem 表示空闲内存顶端 */
	freemem = freemem + n;

	/* Step 4: 如果参数 clear 为真, 清空分配的内存 */
	if (clear) {
		bzero((void *)alloced_mem, n);
	}

	// 内存耗尽了, PANIC !!
	if (PADDR(freemem) >= maxpa) {
		panic("out of memorty\n");
		return (void *)-E_NO_MEM;
	}

	/* Step 5: 返回分配的内存 */
	return (void *)alloced_mem;
}

/* 在给定页目录 pgdir 中获取虚地址 va 的页表项
如果页表不存在并且参数 create 为真, 则创建一个页表 */
static Pte *
boot_pgdir_walk(Pde *pgdir, u_long va, int create) {
	
	Pde *pgdir_entry;		//指向页目录项
	Pte *pgtable;			//指向页表基址 虚
	Pte *pgtable_entry;		//指向页表项

	/* Step 1: 获取相应的页目录项和页表 */
	/* Hint: 使用 KADDR 和 PTE_ADDR 从页目录项的值中获取页表 */
	pgdir_entry = &pgdir[PDX(va)];
	if (*pgdir_entry & PTE_V) {
		pgtable = (Pte *)KADDR(PTE_ADDR(*pgdir_entry));
	}

	/* Step 2: 如果相应的页表不存在并且参数 create 为真, 则创建一个页表 */
	else if (create) {
		pgtable = (Pte *)alloc(BY2PG, BY2PG, 1);	// 使用 alloc 申请空间存放页表
		*pgdir_entry = PADDR(pgtable) | PTE_V;		// 将 pgtable 转为物理地址, 设置权限位, 赋值给页目录项的内容
	}
	else {
		return (Pte *)NULL;	//不创建页表, 直接返回0
	}
	
	/* Step 3: 获取虚地址 va 的页表项, 并返回指向其的指针 */
	pgtable_entry = &pgtable[PTX(va)];
	return pgtable_entry;
}

/* 将虚地址空间 [va, va+size) 映射到位于页目录 pgdir 中的页表的物理地址空间 [pa, pa+size)
使用允许标志位 perm|PTE_V
初始状态: size 是 BY2PG 的倍数 */
void 
boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) {
	int i;				//循环变量
	u_long va_temp;		//临时虚拟地址
	u_long pa_temp;		//临时物理地址
	Pte *pgtable_entry;	//页表项基址

	/* Step 1: 检查 size 是不是 BY2PG 的倍数 */
	if (size % BY2PG != 0) return;

	/* Step 2: 将虚地址空间映射到物理地址空间 */
	for (i = 0; i < size / BY2PG; i++) {
		va_temp = (u_long)(va + i * BY2PG);		//计算当前虚地址
		pa_temp = (u_long)(pa + i * BY2PG);		//计算当前物理地址
		pgtable_entry = boot_pgdir_walk(pgdir, va_temp, 1);	//使用 boot_pgdir_walk 获取当前虚地址的页表项
		*pgtable_entry = pa_temp | perm | PTE_V;	//将当前物理地址赋值给页表项内容, 设置权限位
	}
	pgdir[PDX(va)] = pgdir[PDX(va)] | perm | PTE_V;		//设置页目录项权限位
}

/* 建立二级页表
Hint: 
参见 mmu.h 中的 UPAGES 和 UENVS */
void 
mips_vm_init() {
	extern char end[];
	extern int mCONTEXT;
	extern struct Env *envs;

	Pde *pgdir;
	u_int n;

	/* Step 1: 为页目录 boot_pgdir 分配一个页面 */
	pgdir = (Pde *)alloc(BY2PG, BY2PG, 1);
	printf("to memory %x for struct page directory.\n", freemem);
	mCONTEXT = (int)pgdir;
	boot_pgdir = pgdir;

	/* Step 2: 为全局数组 pages 分配合适大小的物理内存, 用于物理内存管理
	之后, 将虚地址 UPAGES 映射到刚刚分配的物理地址 pages 
	为了对齐考虑, 在映射之前应该先对内存大小取整 */
	pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
	printf("to memory %x for struct Pages.\n", freemem);
	n = ROUND(npage * sizeof(struct Page), BY2PG);
	boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_R);

	/* Step 3: 为全局数组 envs 分配合适大小的物理内存, 用于进程管理
	之后将物理地址映射到 UENVS */
	envs = (struct Env *)alloc(NENV * sizeof(struct Env), BY2PG, 1);
	n = ROUND(NENV * sizeof(struct Env), BY2PG);
	boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_R);

	printf("pmap.c:\t mips vm init success\n");
}

/* 初始化 struct Page 和 page_free_list
数组 pages 的每个物理页面都有一个 struct Page, 页面使用引用计数, 空闲页面都在空闲链表里 */
void
page_init(void) {
	/* Step 1: 初始化 page_free_list. */
	LIST_INIT(&page_free_list);

	/* Step 2: 取整 freemem 为 BY2PG 的倍数 */
	freemem = ROUND(freemem, BY2PG);

	/* Step 3: 将低于 freemem 的内存标记为被使用 (设置 pp_ref 为 1) */
	int index;
	int max_index = PADDR(freemem) / BY2PG;		//使用到的页面的最大标号
	for (index = 0; index < max_index; index++) {
		pages[index].pp_ref = 1;
	}

	/* Step 4: 将其他内存标记为空闲, 加入空闲链表 */
	for (index = max_index; index < npage; index++) {
		pages[index].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, &pages[index], pp_link);
	}
}

/* 从空闲内存分配一个物理页面, 并清空该页面
如果分配失败(内存耗尽), 返回 -E_NO_MEM; 否则, 设置分配的页面地址给 *pp 并返回 0 */
int
page_alloc(struct Page **pp) {
	struct Page *ppage_temp;	//分配的页面

	/* Step 1: 从空闲内存获取一个空闲页面, 如果失败, 返回 -E_NO_MEM */
	if (LIST_EMPTY(&page_free_list)) {
		return -E_NO_MEM;
	}
	ppage_temp = LIST_FIRST(&page_free_list);
	LIST_REMOVE(ppage_temp, pp_link);

	/* Step 2: 初始化这个页面 */
	bzero((void *)page2kva(ppage_temp), BY2PG);
	*pp = ppage_temp;
	return 0;
}

/* 释放一个页面, 如果它的 pp_ref 到 0, 将其标记为空闲, 即加入空闲列表 */
void
page_free(struct Page *pp) {

	/* Step 1: 如果仍有引用这个页面的虚地址, 什么都不做 */
	if (pp->pp_ref > 0) return;

	/* Step 2: 如果 pp_ref 为 0, 将这个页面标记为空闲, 即加入空闲列表 */
	if (pp->pp_ref == 0) {
		LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
		return;
	}
	
	/* 如果 pp_ref<0, 之前一定出了问题, 所以报错!!! */
	panic("cgh:pp->pp_ref is less than zero\n");
	return;
}

/* 给定一个指向页目录的指针 pgdir, 返回一个指向虚地址 va 对应页表项的指针 (在 PTE_R|PTE_V 允许时)
初始状态: pgdir 应该是二级页表结构
结束状态: 如果内存耗尽, 返回 -E_NO_MEM; 否则, 成功取得页表项, 将它的地址保存在 *ppte, 并返回0, 标志成功 */
int
pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
	Pde *pgdir_entry;		//指向页目录项
	Pte *pgtable;			//指向页表基址 虚
	struct Page *ppage;		//指向页面

	/* Step 1: 获取相应的页目录项和页表 */
	pgdir_entry = pgdir + PDX(va);
	if (*pgdir_entry & PTE_V) {
		pgtable = (Pte *)KADDR(PTE_ADDR(*pgdir_entry));
	}

	/* Step 2: 如果相应的页表不存在或者无效, 并且参数 create 为真, 则创建一个页表 */
	else if (create) {
		// 申请一页物理内存来存放这个页表, 如果内存耗尽, 返回 -E_NO_MEM
		if (page_alloc(&ppage) == -E_NO_MEM) {
			*ppte = (Pte *)NULL;
			return -E_NO_MEM;
		}
		//内存申请成功
		ppage->pp_ref++;
		pgtable = (Pte *)page2kva(ppage);		//将 pgtable 设置为 ppage 对应的内核虚地址
		*pgdir_entry = PADDR(pgtable) | PTE_V | PTE_R;	//将页表的物理地址赋值给页目录项的内容, 设置权限位
	}
	else {
		*ppte = (Pte *)NULL;
		return -E_INVAL;
	}

	/* Step 3: 将页表项赋值给 *ppte 并作为返回值返回 */
	*ppte = &pgtable[PTX(va)];
	return 0;
}

/* 将物理地址 pp 映射到虚地址 va
成功返回0; 如果页表不能被分配返回 -E_NO_MEM
先判断虚地址 va 是否有对应的页表项: 如果页表项有效（或者叫va是否已经有了映射的物理地址）的话, 则判断这个物理地址是不是要插入的物理地址 pp
如果不是, 则调用 page_remove 把该物理地址移除掉, 并更新 tlb, 插入新的物理页 pp, 并增加引用
如果是的话, 则修改权限, 将页表项的允许标记位(低12位)应该被设置为 perm|PTE_V, 并放到 tlb 中 */
int
page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm) {
	u_int PERM;
	Pte *pgtable_entry;
	PERM = perm | PTE_V;

	/* Step 1: 获取对应的页表项 */
	pgdir_walk(pgdir, va, 0, &pgtable_entry);

	if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) {
		if (pa2page(*pgtable_entry) != pp) {
			page_remove(pgdir, va);
		}
		else {
			tlb_invalidate(pgdir, va);
			*pgtable_entry = (page2pa(pp) | PERM);
			return 0;
		}
	}

	/* Step 2: 更新 TLB. */
	tlb_invalidate(pgdir, va);

	/* Step 3: 重新获取页表项, 这次允许创建, 来实现这个插入 */
	if (pgdir_walk(pgdir, va, 1, &pgtable_entry) != 0) {
		return -E_NO_MEM;
	}

	*pgtable_entry = (page2pa(pp) | PERM);
	pp->pp_ref++;
	return 0;
}

/* 查找虚地址 va 映射到的页面
返回一个指向页面的指针, 并将其页表项保存到 *ppte
如果 va 没有映射到任何页面, 返回 NULL */
struct Page *
page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *ppage;
	Pte *pte;

	/* Step 1: 获取页表项 */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: 检查页表项是否存在, 是否有效 */
	if (pte == (Pte *)NULL) {
		return NULL;
	}
	if ((*pte & PTE_V) == 0) {
		return NULL;    //页面不在内存中
	}

	/* Step 2: 获取相应的 struct Page */
	ppage = pa2page(*pte);	//页表项中存储的是页面的物理地址
	if (ppte) {
		*ppte = pte;
	}

	return ppage;
}

/* 减少页面 *pp 的 pp_ref, 如果 pp_ref 到 0, 释放这个页面 */
void 
page_decref(struct Page *pp) {
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

/* 移除映射到虚地址 va 的映射 */
void
page_remove(Pde *pgdir, u_long va) {
	Pte *pgtable_entry;
	struct Page *ppage;

	/* Step 1: 获取页表项, 检查页表项是否有效, 如果无效, 直接返回 */
	ppage = page_lookup(pgdir, va, &pgtable_entry);
	if (ppage == NULL) {
		return;
	}

	/* Step 2: 减少 pp_ref, 当没有虚地址映射到这个页面时, 释放它 */
	page_decref(ppage);

	/* Step 3: 更新 TLB. */
	*pgtable_entry = (Pte)NULL;
	tlb_invalidate(pgdir, va);
	return;
}

/* 更新 TLB. */
void
tlb_invalidate(Pde *pgdir, u_long va) {
	if (curenv) {
		tlb_out(PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id));
	}
	else {
		tlb_out(PTE_ADDR(va));
	}
}
