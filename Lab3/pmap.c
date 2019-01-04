#include "mmu.h"
#include "pmap.h"
#include "printf.h"
#include "env.h"
#include "error.h"

/* ��Щ������ mips_detect_memory() ���� */
u_long maxpa;            /* ��������ַ */
u_long npage;            /* ҳ������ */
u_long basemem;          /* base memory ��С(�����ֽ�) */
u_long extmem;           /* extended memory ��С(�����ֽ�) */

Pde *boot_pgdir;			//��ʼҳĿ¼��

struct Page *pages;			//Page����
static u_long freemem;		//�����ڴ涥��

static struct Page_list page_free_list; /* ����ҳ������ */


/* ��ʼ�� basemem �� npage
���� basemem Ϊ64MB, ��������Ӧ�� npage ֵ */
void 
mips_detect_memory() {
	/* Step 1: ��ʼ�� basemem. (��ʹ����ʵ�����ʱ, CMOS ���������ж���KB). */
	maxpa = 67108864;	//64MB
	basemem = 67108864;	//64MB
	extmem = 0;

	// Step 2: ������Ӧ�� npage
	npage = basemem / BY2PG;

	printf("Physical memory: %dK available, ", (int)(maxpa / 1024));
	printf("base = %dK, extended = %dK\n", (int)(basemem / 1024), (int)(extmem / 1024));
}

/* ֻ���ڲ���ϵͳ�ո�����ʱ
���ղ��� align ����ȡ����, ���� n �ֽڴ�С�������ڴ�. ������� clear Ϊ��, ���·�����ڴ�ȫ������
����ڴ�ľ�, ��Ҫ panic; ���򷵻��·�����ڴ���׵�ַ*/
static void *
alloc(u_int n, u_int align, int clear) {
	extern char end[];		//0x80400000
	u_long alloced_mem;		//��ǰ����Ŀռ�

	/* ����ǳ���, ��ʼ�� freemem Ϊ�������������κ��ں˴����ȫ�ֱ����ĵ�һ�������ַ */
	if (freemem == 0) {
		freemem = (u_long)end;
	}

	/* Step 1: �� freemem ȡ�� */
	freemem = ROUND(freemem, align);

	/* Step 2: ���� freemem ��ǰ��ֵ��Ϊ������ڴ�Ļ���ַ */
	alloced_mem = freemem;

	/* Step 3: ���� freemem ����¼�ڴ����, freemem ��ʾ�����ڴ涥�� */
	freemem = freemem + n;

	/* Step 4: ������� clear Ϊ��, ��շ�����ڴ� */
	if (clear) {
		bzero((void *)alloced_mem, n);
	}

	// �ڴ�ľ���, PANIC !!
	if (PADDR(freemem) >= maxpa) {
		panic("out of memorty\n");
		return (void *)-E_NO_MEM;
	}

	/* Step 5: ���ط�����ڴ� */
	return (void *)alloced_mem;
}

/* �ڸ���ҳĿ¼ pgdir �л�ȡ���ַ va ��ҳ����
���ҳ�����ڲ��Ҳ��� create Ϊ��, �򴴽�һ��ҳ�� */
static Pte *
boot_pgdir_walk(Pde *pgdir, u_long va, int create) {
	
	Pde *pgdir_entry;		//ָ��ҳĿ¼��
	Pte *pgtable;			//ָ��ҳ���ַ ��
	Pte *pgtable_entry;		//ָ��ҳ����

	/* Step 1: ��ȡ��Ӧ��ҳĿ¼���ҳ�� */
	/* Hint: ʹ�� KADDR �� PTE_ADDR ��ҳĿ¼���ֵ�л�ȡҳ�� */
	pgdir_entry = &pgdir[PDX(va)];
	if (*pgdir_entry & PTE_V) {
		pgtable = (Pte *)KADDR(PTE_ADDR(*pgdir_entry));
	}

	/* Step 2: �����Ӧ��ҳ�����ڲ��Ҳ��� create Ϊ��, �򴴽�һ��ҳ�� */
	else if (create) {
		pgtable = (Pte *)alloc(BY2PG, BY2PG, 1);	// ʹ�� alloc ����ռ���ҳ��
		*pgdir_entry = PADDR(pgtable) | PTE_V;		// �� pgtable תΪ�����ַ, ����Ȩ��λ, ��ֵ��ҳĿ¼�������
	}
	else {
		return (Pte *)NULL;	//������ҳ��, ֱ�ӷ���0
	}
	
	/* Step 3: ��ȡ���ַ va ��ҳ����, ������ָ�����ָ�� */
	pgtable_entry = &pgtable[PTX(va)];
	return pgtable_entry;
}

/* �����ַ�ռ� [va, va+size) ӳ�䵽λ��ҳĿ¼ pgdir �е�ҳ��������ַ�ռ� [pa, pa+size)
ʹ�������־λ perm|PTE_V
��ʼ״̬: size �� BY2PG �ı��� */
void 
boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) {
	int i;				//ѭ������
	u_long va_temp;		//��ʱ�����ַ
	u_long pa_temp;		//��ʱ�����ַ
	Pte *pgtable_entry;	//ҳ�����ַ

	/* Step 1: ��� size �ǲ��� BY2PG �ı��� */
	if (size % BY2PG != 0) return;

	/* Step 2: �����ַ�ռ�ӳ�䵽�����ַ�ռ� */
	for (i = 0; i < size / BY2PG; i++) {
		va_temp = (u_long)(va + i * BY2PG);		//���㵱ǰ���ַ
		pa_temp = (u_long)(pa + i * BY2PG);		//���㵱ǰ�����ַ
		pgtable_entry = boot_pgdir_walk(pgdir, va_temp, 1);	//ʹ�� boot_pgdir_walk ��ȡ��ǰ���ַ��ҳ����
		*pgtable_entry = pa_temp | perm | PTE_V;	//����ǰ�����ַ��ֵ��ҳ��������, ����Ȩ��λ
	}
	pgdir[PDX(va)] = pgdir[PDX(va)] | perm | PTE_V;		//����ҳĿ¼��Ȩ��λ
}

/* ��������ҳ��
Hint: 
�μ� mmu.h �е� UPAGES �� UENVS */
void 
mips_vm_init() {
	extern char end[];
	extern int mCONTEXT;
	extern struct Env *envs;

	Pde *pgdir;
	u_int n;

	/* Step 1: ΪҳĿ¼ boot_pgdir ����һ��ҳ�� */
	pgdir = (Pde *)alloc(BY2PG, BY2PG, 1);
	printf("to memory %x for struct page directory.\n", freemem);
	mCONTEXT = (int)pgdir;
	boot_pgdir = pgdir;

	/* Step 2: Ϊȫ������ pages ������ʴ�С�������ڴ�, ���������ڴ����
	֮��, �����ַ UPAGES ӳ�䵽�ոշ���������ַ pages 
	Ϊ�˶��뿼��, ��ӳ��֮ǰӦ���ȶ��ڴ��Сȡ�� */
	pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
	printf("to memory %x for struct Pages.\n", freemem);
	n = ROUND(npage * sizeof(struct Page), BY2PG);
	boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_R);

	/* Step 3: Ϊȫ������ envs ������ʴ�С�������ڴ�, ���ڽ��̹���
	֮�������ַӳ�䵽 UENVS */
	envs = (struct Env *)alloc(NENV * sizeof(struct Env), BY2PG, 1);
	n = ROUND(NENV * sizeof(struct Env), BY2PG);
	boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_R);

	printf("pmap.c:\t mips vm init success\n");
}

/* ��ʼ�� struct Page �� page_free_list
���� pages ��ÿ������ҳ�涼��һ�� struct Page, ҳ��ʹ�����ü���, ����ҳ�涼�ڿ��������� */
void
page_init(void) {
	/* Step 1: ��ʼ�� page_free_list. */
	LIST_INIT(&page_free_list);

	/* Step 2: ȡ�� freemem Ϊ BY2PG �ı��� */
	freemem = ROUND(freemem, BY2PG);

	/* Step 3: ������ freemem ���ڴ���Ϊ��ʹ�� (���� pp_ref Ϊ 1) */
	int index;
	int max_index = PADDR(freemem) / BY2PG;		//ʹ�õ���ҳ��������
	for (index = 0; index < max_index; index++) {
		pages[index].pp_ref = 1;
	}

	/* Step 4: �������ڴ���Ϊ����, ����������� */
	for (index = max_index; index < npage; index++) {
		pages[index].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, &pages[index], pp_link);
	}
}

/* �ӿ����ڴ����һ������ҳ��, ����ո�ҳ��
�������ʧ��(�ڴ�ľ�), ���� -E_NO_MEM; ����, ���÷����ҳ���ַ�� *pp ������ 0 */
int
page_alloc(struct Page **pp) {
	struct Page *ppage_temp;	//�����ҳ��

	/* Step 1: �ӿ����ڴ��ȡһ������ҳ��, ���ʧ��, ���� -E_NO_MEM */
	if (LIST_EMPTY(&page_free_list)) {
		return -E_NO_MEM;
	}
	ppage_temp = LIST_FIRST(&page_free_list);
	LIST_REMOVE(ppage_temp, pp_link);

	/* Step 2: ��ʼ�����ҳ�� */
	bzero((void *)page2kva(ppage_temp), BY2PG);
	*pp = ppage_temp;
	return 0;
}

/* �ͷ�һ��ҳ��, ������� pp_ref �� 0, ������Ϊ����, ����������б� */
void
page_free(struct Page *pp) {

	/* Step 1: ��������������ҳ������ַ, ʲô������ */
	if (pp->pp_ref > 0) return;

	/* Step 2: ��� pp_ref Ϊ 0, �����ҳ����Ϊ����, ����������б� */
	if (pp->pp_ref == 0) {
		LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
		return;
	}
	
	/* ��� pp_ref<0, ֮ǰһ����������, ���Ա���!!! */
	panic("cgh:pp->pp_ref is less than zero\n");
	return;
}

/* ����һ��ָ��ҳĿ¼��ָ�� pgdir, ����һ��ָ�����ַ va ��Ӧҳ�����ָ�� (�� PTE_R|PTE_V ����ʱ)
��ʼ״̬: pgdir Ӧ���Ƕ���ҳ��ṹ
����״̬: ����ڴ�ľ�, ���� -E_NO_MEM; ����, �ɹ�ȡ��ҳ����, �����ĵ�ַ������ *ppte, ������0, ��־�ɹ� */
int
pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
	Pde *pgdir_entry;		//ָ��ҳĿ¼��
	Pte *pgtable;			//ָ��ҳ���ַ ��
	struct Page *ppage;		//ָ��ҳ��

	/* Step 1: ��ȡ��Ӧ��ҳĿ¼���ҳ�� */
	pgdir_entry = pgdir + PDX(va);
	if (*pgdir_entry & PTE_V) {
		pgtable = (Pte *)KADDR(PTE_ADDR(*pgdir_entry));
	}

	/* Step 2: �����Ӧ��ҳ�����ڻ�����Ч, ���Ҳ��� create Ϊ��, �򴴽�һ��ҳ�� */
	else if (create) {
		// ����һҳ�����ڴ���������ҳ��, ����ڴ�ľ�, ���� -E_NO_MEM
		if (page_alloc(&ppage) == -E_NO_MEM) {
			*ppte = (Pte *)NULL;
			return -E_NO_MEM;
		}
		//�ڴ�����ɹ�
		ppage->pp_ref++;
		pgtable = (Pte *)page2kva(ppage);		//�� pgtable ����Ϊ ppage ��Ӧ���ں����ַ
		*pgdir_entry = PADDR(pgtable) | PTE_V | PTE_R;	//��ҳ��������ַ��ֵ��ҳĿ¼�������, ����Ȩ��λ
	}
	else {
		*ppte = (Pte *)NULL;
		return -E_INVAL;
	}

	/* Step 3: ��ҳ���ֵ�� *ppte ����Ϊ����ֵ���� */
	*ppte = &pgtable[PTX(va)];
	return 0;
}

/* �������ַ pp ӳ�䵽���ַ va
�ɹ�����0; ���ҳ���ܱ����䷵�� -E_NO_MEM
���ж����ַ va �Ƿ��ж�Ӧ��ҳ����: ���ҳ������Ч�����߽�va�Ƿ��Ѿ�����ӳ��������ַ���Ļ�, ���ж���������ַ�ǲ���Ҫ����������ַ pp
�������, ����� page_remove �Ѹ������ַ�Ƴ���, ������ tlb, �����µ�����ҳ pp, ����������
����ǵĻ�, ���޸�Ȩ��, ��ҳ�����������λ(��12λ)Ӧ�ñ�����Ϊ perm|PTE_V, ���ŵ� tlb �� */
int
page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm) {
	u_int PERM;
	Pte *pgtable_entry;
	PERM = perm | PTE_V;

	/* Step 1: ��ȡ��Ӧ��ҳ���� */
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

	/* Step 2: ���� TLB. */
	tlb_invalidate(pgdir, va);

	/* Step 3: ���»�ȡҳ����, ���������, ��ʵ��������� */
	if (pgdir_walk(pgdir, va, 1, &pgtable_entry) != 0) {
		return -E_NO_MEM;
	}

	*pgtable_entry = (page2pa(pp) | PERM);
	pp->pp_ref++;
	return 0;
}

/* �������ַ va ӳ�䵽��ҳ��
����һ��ָ��ҳ���ָ��, ������ҳ����浽 *ppte
��� va û��ӳ�䵽�κ�ҳ��, ���� NULL */
struct Page *
page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *ppage;
	Pte *pte;

	/* Step 1: ��ȡҳ���� */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: ���ҳ�����Ƿ����, �Ƿ���Ч */
	if (pte == (Pte *)NULL) {
		return NULL;
	}
	if ((*pte & PTE_V) == 0) {
		return NULL;    //ҳ�治���ڴ���
	}

	/* Step 2: ��ȡ��Ӧ�� struct Page */
	ppage = pa2page(*pte);	//ҳ�����д洢����ҳ��������ַ
	if (ppte) {
		*ppte = pte;
	}

	return ppage;
}

/* ����ҳ�� *pp �� pp_ref, ��� pp_ref �� 0, �ͷ����ҳ�� */
void 
page_decref(struct Page *pp) {
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

/* �Ƴ�ӳ�䵽���ַ va ��ӳ�� */
void
page_remove(Pde *pgdir, u_long va) {
	Pte *pgtable_entry;
	struct Page *ppage;

	/* Step 1: ��ȡҳ����, ���ҳ�����Ƿ���Ч, �����Ч, ֱ�ӷ��� */
	ppage = page_lookup(pgdir, va, &pgtable_entry);
	if (ppage == NULL) {
		return;
	}

	/* Step 2: ���� pp_ref, ��û�����ַӳ�䵽���ҳ��ʱ, �ͷ��� */
	page_decref(ppage);

	/* Step 3: ���� TLB. */
	*pgtable_entry = (Pte)NULL;
	tlb_invalidate(pgdir, va);
	return;
}

/* ���� TLB. */
void
tlb_invalidate(Pde *pgdir, u_long va) {
	if (curenv) {
		tlb_out(PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id));
	}
	else {
		tlb_out(PTE_ADDR(va));
	}
}
