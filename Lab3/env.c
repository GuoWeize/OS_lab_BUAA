#include "mmu.h"
#include "error.h"
#include "env.h"
#include "kerelf.h"
#include "sched.h"
#include "pmap.h"
#include "printf.h"

struct Env *envs = NULL;		// ���н���
struct Env *curenv = NULL;		// ��ǰ���еĽ���

static struct Env_list env_free_list;   // ���н�������
struct Env_list env_sched_list[2];      // ���Ƚ�������

extern Pde *boot_pgdir;		// ��ʼҳĿ¼��
extern char *KERNEL_SP;		// �ں�ջָ��


/*  Ϊÿ����������һ�����е�ID
��ǰ����: ���� e ����
����״̬: �ɹ��򷵻� e �� env_id*/
u_int
mkenvid(struct Env *e) {
	static u_long next_env_id = 0;
	
	/* envid �ĵ�λ���� e �� envs �����λ�� */
	u_int idx = e - envs;

	/* envid �ĸ�λ���������ֵ */
	return (++next_env_id << (1 + LOG2NENV)) | idx;
}


/*  ͨ��һ�� id ��ȡ��Ӧ�Ľ���
��ǰ����: 
	penv ����, checkperm ��0��1
����״̬: 
	�ɹ�����0, ��� id �� 0, ���� *penv = curenv; �������� *penv = envs[ENVX(envid)];
	ʧ�ܷ���-E_BAD_ENV, ���� *penv Ϊ NULL */
int
envid2env(u_int envid, struct Env **penv, int checkperm) {
	struct Env *e;
	
	/*Step 1: ʹ�� envid ȷ������ e */
	if (envid == 0) {
		*penv = curenv;
		return 0;
	}
	e = &envs[ENVX(envid)];

	//���̴��ڿ���״̬���߽��̲�����
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*penv = NULL;
		return -E_BAD_ENV;
	}
	
	/*Step 2: �� checkperm ���м��
	��� checkperm Ϊ��, ����Ľ��̱����ǵ�ǰ���̻����ǵ�ǰ���̵�ֱ���ӽ���, ���򱨴� */
	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
		*penv = NULL;
		return -E_BAD_ENV;
	}
	
	*penv = e;
	return 0;
}


/*  �� envs �е����н��̱��Ϊ����, �����뵽 env_free_list ��
	�������, ʹ�õ�һ�ε��� env_alloc ʱ, ������ȷ�� envs[] Ԫ�� */
void
env_init(void) {
	int i;
	/*Step 1: ��ʼ�� env_free_list. */
	LIST_INIT(&env_free_list);

	/*Step 2: ���� envs �е�Ԫ��, ��ʼ������Ԫ��(��Ҫ��ʼ�����ǵ�״̬, �����Ϊ����)
	Ȼ�����ǰ�����ȷ��˳����뵽 env_free_list �� */
	for (i = NENV-1; i >= 0; i--) {
		envs[i].env_status = ENV_FREE;
		LIST_INSERT_HEAD(&env_free_list, &envs[i], env_link);
	}
}


/*  ��ʼ���½��� e ���ں˵�ַ����
	����һ��ҳĿ¼, ���������� e->env_pgdir �� e->env_cr3, ���ҳ�ʼ���½��̵�ַ�ռ���ں˲���
	��Ҫ��������ַ�ռ���û�̬ӳ���κζ��� */
static int
env_setup_vm(struct Env *e) {

	int i, r;
	struct Page *p = NULL;
	Pde *pgdir;

	/*Step 1: ʹ���� lab2 ����ɵĺ���ΪҳĿ¼����һ��ҳ��, ���������������ü���
			  pgdir �� e ��ҳĿ¼, Ϊ�丳ֵ */
	r = page_alloc(&p);
	if (r != 0) {
		panic("env_setup_vm - page alloc error\n");
		return r;
	}
	p->pp_ref++;
	pgdir = (Pde *)page2kva(p);

	/*Step 2: ��� pgdir �� UTOP ֮�µĿռ� */
	for (i = 0; i < PDX(UTOP); i++) {
		pgdir[i] = 0;
	}

	/*Step 3: ���ں˵� boot_pgdir ��ֵ�� pgdir */
	/* Hint: ���н��̵������ַ�ռ䶼���� UTOP (���� VPT �� UVPT), ���ֲο�mmu.h
			 �����ں˵� boot_pgdir ��Ϊһ����ģ�� */
	for (i = PDX(UTOP); i < PDX(0xffffffff); i++) {
		pgdir[i] = boot_pgdir[i];
	}

	/*Step 4: ���������� e->env_pgdir �� e->env_cr3 */
	e->env_pgdir = pgdir;
	e->env_cr3 = PADDR(pgdir);

	/* VPT �� UVPT ӳ�䵽�����Լ���ҳ��, ���Ų�ͬ��Ȩ��λ */
	e->env_pgdir[PDX(VPT)] = e->env_cr3;
	e->env_pgdir[PDX(UVPT)] = e->env_cr3 | PTE_V | PTE_R;
	return 0;
}


/*  ��������ʼ��һ���µĽ���, ����ɹ�, ���̱����� *new
��ǰ����:
	����µĽ���û�и�����, �� parent_id ӦΪ 0
	env_init �ڴ˺���֮ǰ����
����״̬:
	�ɹ����� 0, ��Ϊ�µĽ������ú��ʵ�ֵ
	���û�п��н�����, ���� -E_NO_FREE_ENV */
int
env_alloc(struct Env **new, u_int parent_id) {
	int r;
	struct Env *e;

	/*Step 1: �� env_free_list ��ȡһ������ */
	e = LIST_FIRST(&env_free_list);
	if (e == NULL) {
		*new = NULL;
		return -E_NO_FREE_ENV;
	}

	/*Step 2: ���� env_setup_vm ��Ϊ�½��̳�ʼ���ں˴洢���� */
	r = env_setup_vm(e);
	if (r != 0)
		return r;

	/*Step 3: �ú��ʵ�ֵ��ʼ���½��̵� id, status, parent_id (������ PC) */
	e->env_id = mkenvid(e);
	e->env_status = ENV_RUNNABLE;
	e->env_parent_id = parent_id;

	/*Step 4: ��ע�½��� env_tf �ĳ�ʼ��, ������ CPU ״̬��ջ�Ĵ��� $29 */
	e->env_tf.cp0_status = 0x10001004;
	e->env_tf.regs[29] = USTACKTOP;

	/*Step 5: ���½��̴� Env free list ���Ƴ� */
	LIST_REMOVE(e, env_link);
	*new = e;
	return 0;
}


/*  ELF ��������ȡÿ����, ֮��������������������ÿ����ӳ�䵽��ȷ�����ַ
	bin_size �� bin ���ļ��еĴ�С, sgsize �Ƕ����ڴ��еĴ�С
��ǰ����:
	bin ��Ϊ��, va ���ܲ��� 4KB �����
����״̬:
	�ɹ����� 0, ���򷵻ظ�ֵ */
static int
load_icode_mapper(u_long va, u_int32_t sgsize,
				  u_char *bin, u_int32_t bin_size, void *user_data) {
	
	struct Env *env = (struct Env *)user_data;	// userdata Ϊ env �� PCB
	struct Page *p = NULL;
	u_long offset = va - ROUNDDOWN(va, BY2PG);	// va ����һ��ҳ���С�Ĳ���
	int r;
	u_long i;
	void* src;
	void* dst;
	size_t len;

	/*Step 1: �� bin �����е����ݼ��ص��ڴ� */
	for (i = 0; i < bin_size; i += BY2PG) {
		// ����һ��ҳ�沢�������ü���
		r = page_alloc(&p);
		if (r != 0)
			return r;
		p->pp_ref++;

		// ���ǲ�ͬ�ĸ������
		if (i == 0) {	// [bin, BY2PG-offset] --> page_0[offset, BY2PG]
			src = bin;
			dst = (u_char *)(page2kva(p) + offset);
			len = (size_t)MIN(BY2PG - offset, bin_size);
		}
		else {			// [bin-offset, page_end] --> page_i[0, BY2PG]
			src = bin + i - offset;
			dst = (u_char *)page2kva(p);
			len = (size_t)MIN(BY2PG, bin_size - i);
		}
		bcopy(src, dst, len);

		// ����ҳӳ��
		r = page_insert(env->env_pgdir, p, va + i, PTE_V | PTE_R);
		if (r != 0)
			return r;
	}

	/*Step 2: ��� bin_size < sgsize, ����ҳ��ﵽ sgsize, i ������ bin_size ��ֵ */
	while (i < sgsize) {
		r = page_alloc(&p);
		if (r != 0)
			return r;
		p->pp_ref++;

		r = page_insert(env->env_pgdir, p, va + i, PTE_V | PTE_R);
		if (r != 0)
			return r;

		i += BY2PG;
	}

	return 0;
}


/*  Ϊ�û�����������ʼ��ջ�Ͷ����Ƴ���
	�������ʹ�� ELF ���������������Ķ�����ӳ��, �����̵��û��ڴ�
	������ӳ�������� ELF �������ṩ
	���������һ��ҳ��ӳ�䵽���̵ĳ�ʼ��ջ, ��λ�����ַ�� USTACKTOP - BY2PG
	Hints: ���е�ӳ�䶼�� read/write , ��������ε� */
static void
load_icode(struct Env *e, u_char *binary, u_int size) {
	/* Hint: ����֪��Ϊ�� ������Ĳ�ͬӳ�� ������Ҫ��ЩȨ��
			 ��ס binary ӳ���� .out ��ʽ, �����˴�������� */
	struct Page *p = NULL;
	u_long entry_point;
	u_long r;
	u_long perm;

	/* Step 1: ����һ��ҳ�� */
	r = page_alloc(&p);
	if (r != 0)
		return;

	/* Step 2: ʹ���ʵ���Ȩ��Ϊ�½������ó�ʼ��ջ */
	/* Hint: �û�ջӦ����д��? */
	perm = PTE_V | PTE_R;
	page_insert(e->env_pgdir, p, USTACKTOP - BY2PG, perm);

	/* Step 3: ���� load_elf ��ELF �ļ��������ص��ڴ��� */
	// �� load_icode_mapper ������Ϊ�������� load_elf ��
	r = load_elf(binary, size, &entry_point, e, load_icode_mapper);
	if (r != 0)
		return;
	
	/* Step 4: ��PC�Ĵ�������Ϊ�ʵ���ֵ */
	e->env_tf.pc = entry_point;
}


/*  ʹ�� env_alloc ����һ���½���, ʹ�� load_icode ���� ELF �������ļ�, ���������ȼ�
	�������ֻ���ں˳�ʼ��ʱ����, �����е�һ���û�ģʽ�Ľ���֮ǰ
	Hints: ���������װ�� env_alloc �� load_icode */
void
env_create_priority(u_char *binary, int size, int priority) {
	struct Env *e;
	
	/*Step 1: ʹ�� env_alloc ����һ���½��� */
	int r;
	r = env_alloc(&e, 0);
	if (r != 0)
		return;

	/*Step 2: �����½��̵����ȼ� */
	e->env_pri = priority;

	/*Step 3: ʹ�� load_icode ���� ELF �������ļ� */
	load_icode(e, binary, size);

	// �����������
	LIST_INSERT_HEAD(&env_sched_list[0], e, env_sched_link);
}


/*  ����һ��Ĭ�����ȼ�(1)���½��� */
void
env_create(u_char *binary, int size) {
	/* Step 1: ʹ�� env_create_priority ����һ�����ȼ�Ϊ 1 �Ľ��� */
	env_create_priority(binary, size, 1);
}


/*  �ͷŽ��� e ����ʹ�õ������ڴ� */
void
env_free(struct Env *e) {
	Pte *pt;
	u_int pdeno, pteno, pa;

	/* ��¼���̵����� */
	printf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

	/* �����ַ�ռ����û����������ҳӳ�� */
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		/* ���鿴ӳ��ҳ�� */
		if (!(e->env_pgdir[pdeno] & PTE_V)) {
			continue;
		}
		/* �ҵ�ҳ��� pa �� va */
		pa = PTE_ADDR(e->env_pgdir[pdeno]);
		pt = (Pte *)KADDR(pa);
		/* ���ҳ��������ҳ���� */
		for (pteno = 0; pteno <= PTX(~0); pteno++)
			if (pt[pteno] & PTE_V) {
				page_remove(e->env_pgdir, (pdeno << PDSHIFT) | (pteno << PGSHIFT));
			}
		/* �ͷ�ҳ���� */
		e->env_pgdir[pdeno] = 0;
		page_decref(pa2page(pa));
	}

	/* �ͷ�ҳĿ¼ */
	pa = e->env_cr3;
	e->env_pgdir = 0;
	e->env_cr3 = 0;
	page_decref(pa2page(pa));

	/* �ѽ����ͻؿ��������ҴӾ���������ȥ�� */
	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD(&env_free_list, e, env_link);
	LIST_REMOVE(e, env_sched_link);
}


/* �ͷŽ��� e , ��� e �ǵ�ǰ����, ��ѡ��һ���µĽ���ִ�� */
void
env_destroy(struct Env *e) {
	env_free(e);	//�ͷ� e

	/* ѡ��һ���½���ִ�� */
	if (curenv == e) {
		curenv = NULL;
		bcopy(	KERNEL_SP - sizeof(struct Trapframe),
				TIMESTACK - sizeof(struct Trapframe),
				sizeof(struct Trapframe));
		printf("i am killed ... \n");
		sched_yield();
	}
}


extern void env_pop_tf(struct Trapframe *tf, int id);	// ������ env_asm.S
extern void lcontext(u_int contxt);

/*  �����л��� e
	�� env_pop_tf ���Ĵ������浽 Trapframe, ���Ĵ���״̬�� curenv �л��� e */
void
env_run(struct Env *e) {
	/*Step 1: ���浱ǰ���� curenv �ļĴ���״̬ */
	/* Hint: �����������ִ��, ��Ҫ�����������л�, ���Է��� env_destroy �ķ���
			 (�� old ����Ķ�����������ǰ���̵� env_tf ��, �Դﵽ������������ĵ�Ч��) */
	struct Trapframe* old = (struct Trapframe*)(TIMESTACK - sizeof(struct Trapframe));
	if (curenv) {
		bcopy(old, &(curenv->env_tf), sizeof(struct Trapframe));
		curenv->env_tf.pc = old->cp0_epc;
	}
	
	/*Step 2: ���½�������Ϊ��ǰ���� curenv */
	curenv = e;

	/*Step 3: ʹ�� lcontext �л���ַ�ռ� */
	lcontext(KADDR(curenv->env_cr3));

	/*Step 4: ʹ�� env_pop_tf �ָ����̵�������(�Ĵ���) ��������̵��û�ģʽ */
	/* Hint: ������Ҫʹ�� GET_ENV_ASID, ����Ϊʲô? */
	env_pop_tf(&(curenv->env_tf), GET_ENV_ASID(curenv->env_id));
}


void
env_check() {
	struct Env *temp, *pe, *pe0, *pe1, *pe2;
	struct Env_list fl;
	int re = 0;
	// should be able to allocate three envs
	pe0 = 0;
	pe1 = 0;
	pe2 = 0;
	assert(env_alloc(&pe0, 0) == 0);
	assert(env_alloc(&pe1, 0) == 0);
	assert(env_alloc(&pe2, 0) == 0);

	assert(pe0);
	assert(pe1 && pe1 != pe0);
	assert(pe2 && pe2 != pe1 && pe2 != pe0);

	// temporarily steal the rest of the free envs
	fl = env_free_list;
	// now this env_free list must be empty!!!!
	LIST_INIT(&env_free_list);

	// should be no free memory
	assert(env_alloc(&pe, 0) == -E_NO_FREE_ENV);

	// recover env_free_list
	env_free_list = fl;

	printf("pe0->env_id %d\n", pe0->env_id);
	printf("pe1->env_id %d\n", pe1->env_id);
	printf("pe2->env_id %d\n", pe2->env_id);

	assert(pe0->env_id == 2048);
	assert(pe1->env_id == 4097);
	assert(pe2->env_id == 6146);
	printf("env_init() work well!\n");

	/* check envid2env work well */
	pe2->env_status = ENV_FREE;
	re = envid2env(pe2->env_id, &pe, 0);

	assert(pe == 0 && re == -E_BAD_ENV);

	pe2->env_status = ENV_RUNNABLE;
	re = envid2env(pe2->env_id, &pe, 0);

	assert(pe->env_id == pe2->env_id &&re == 0);

	temp = curenv;
	curenv = pe0;
	re = envid2env(pe2->env_id, &pe, 1);
	assert(pe == 0 && re == -E_BAD_ENV);
	curenv = temp;
	printf("envid2env() work well!\n");

	/* check env_setup_vm() work well */
	printf("pe1->env_pgdir %x\n", pe1->env_pgdir);
	printf("pe1->env_cr3 %x\n", pe1->env_cr3);

	assert(pe2->env_pgdir[PDX(UTOP)] == boot_pgdir[PDX(UTOP)]);
	assert(pe2->env_pgdir[PDX(UTOP) - 1] == 0);
	printf("env_setup_vm passed!\n");

	assert(pe2->env_tf.cp0_status == 0x10001004);
	printf("pe2`s sp register %x\n", pe2->env_tf.regs[29]);
	printf("env_check() succeeded!\n");
}
