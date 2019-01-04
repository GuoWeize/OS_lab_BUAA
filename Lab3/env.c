#include "mmu.h"
#include "error.h"
#include "env.h"
#include "kerelf.h"
#include "sched.h"
#include "pmap.h"
#include "printf.h"

struct Env *envs = NULL;		// 所有进程
struct Env *curenv = NULL;		// 当前运行的进程

static struct Env_list env_free_list;   // 空闲进程链表
struct Env_list env_sched_list[2];      // 调度进程链表

extern Pde *boot_pgdir;		// 初始页目录项
extern char *KERNEL_SP;		// 内核栈指针


/*  为每个进程生成一个独有的ID
先前条件: 进程 e 存在
结束状态: 成功则返回 e 的 env_id*/
u_int
mkenvid(struct Env *e) {
	static u_long next_env_id = 0;
	
	/* envid 的低位保存 e 在 envs 数组的位置 */
	u_int idx = e - envs;

	/* envid 的高位保存计数数值 */
	return (++next_env_id << (1 + LOG2NENV)) | idx;
}


/*  通过一个 id 获取对应的进程
先前条件: 
	penv 存在, checkperm 是0或1
结束状态: 
	成功返回0, 如果 id 是 0, 设置 *penv = curenv; 否则设置 *penv = envs[ENVX(envid)];
	失败返回-E_BAD_ENV, 设置 *penv 为 NULL */
int
envid2env(u_int envid, struct Env **penv, int checkperm) {
	struct Env *e;
	
	/*Step 1: 使用 envid 确定进程 e */
	if (envid == 0) {
		*penv = curenv;
		return 0;
	}
	e = &envs[ENVX(envid)];

	//进程处于空闲状态或者进程不存在
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*penv = NULL;
		return -E_BAD_ENV;
	}
	
	/*Step 2: 对 checkperm 进行检查
	如果 checkperm 为真, 特殊的进程必须是当前进程或者是当前进程的直接子进程, 否则报错 */
	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
		*penv = NULL;
		return -E_BAD_ENV;
	}
	
	*penv = e;
	return 0;
}


/*  将 envs 中的所有进程标记为空闲, 并加入到 env_free_list 中
	逆序插入, 使得第一次调用 env_alloc 时, 返回正确的 envs[] 元素 */
void
env_init(void) {
	int i;
	/*Step 1: 初始化 env_free_list. */
	LIST_INIT(&env_free_list);

	/*Step 2: 遍历 envs 中的元素, 初始化所有元素(主要初始化它们的状态, 并标记为空闲)
	然后将它们按照正确的顺序插入到 env_free_list 中 */
	for (i = NENV-1; i >= 0; i--) {
		envs[i].env_status = ENV_FREE;
		LIST_INSERT_HEAD(&env_free_list, &envs[i], env_link);
	}
}


/*  初始化新进程 e 的内核地址布局
	分配一个页目录, 根据其设置 e->env_pgdir 和 e->env_cr3, 并且初始化新进程地址空间的内核部分
	不要向进程虚地址空间的用户态映射任何东西 */
static int
env_setup_vm(struct Env *e) {

	int i, r;
	struct Page *p = NULL;
	Pde *pgdir;

	/*Step 1: 使用在 lab2 中完成的函数为页目录分配一个页面, 并且增加它的引用计数
			  pgdir 是 e 的页目录, 为其赋值 */
	r = page_alloc(&p);
	if (r != 0) {
		panic("env_setup_vm - page alloc error\n");
		return r;
	}
	p->pp_ref++;
	pgdir = (Pde *)page2kva(p);

	/*Step 2: 清空 pgdir 在 UTOP 之下的空间 */
	for (i = 0; i < PDX(UTOP); i++) {
		pgdir[i] = 0;
	}

	/*Step 3: 将内核的 boot_pgdir 赋值给 pgdir */
	/* Hint: 所有进程的虚拟地址空间都高于 UTOP (除了 VPT 和 UVPT), 布局参考mmu.h
			 复制内核的 boot_pgdir 作为一部分模板 */
	for (i = PDX(UTOP); i < PDX(0xffffffff); i++) {
		pgdir[i] = boot_pgdir[i];
	}

	/*Step 4: 根据其设置 e->env_pgdir 和 e->env_cr3 */
	e->env_pgdir = pgdir;
	e->env_cr3 = PADDR(pgdir);

	/* VPT 和 UVPT 映射到进程自己的页表, 有着不同的权限位 */
	e->env_pgdir[PDX(VPT)] = e->env_cr3;
	e->env_pgdir[PDX(UVPT)] = e->env_cr3 | PTE_V | PTE_R;
	return 0;
}


/*  创建并初始化一个新的进程, 如果成功, 进程保存在 *new
先前条件:
	如果新的进程没有父进程, 则 parent_id 应为 0
	env_init 在此函数之前调用
结束状态:
	成功返回 0, 并为新的进程设置合适的值
	如果没有空闲进程了, 返回 -E_NO_FREE_ENV */
int
env_alloc(struct Env **new, u_int parent_id) {
	int r;
	struct Env *e;

	/*Step 1: 从 env_free_list 获取一个进程 */
	e = LIST_FIRST(&env_free_list);
	if (e == NULL) {
		*new = NULL;
		return -E_NO_FREE_ENV;
	}

	/*Step 2: 调用 env_setup_vm 来为新进程初始化内核存储布局 */
	r = env_setup_vm(e);
	if (r != 0)
		return r;

	/*Step 3: 用合适的值初始化新进程的 id, status, parent_id (不包括 PC) */
	e->env_id = mkenvid(e);
	e->env_status = ENV_RUNNABLE;
	e->env_parent_id = parent_id;

	/*Step 4: 关注新进程 env_tf 的初始化, 尤其是 CPU 状态和栈寄存器 $29 */
	e->env_tf.cp0_status = 0x10001004;
	e->env_tf.regs[29] = USTACKTOP;

	/*Step 5: 将新进程从 Env free list 中移除 */
	LIST_REMOVE(e, env_link);
	*new = e;
	return 0;
}


/*  ELF 加载器提取每个段, 之后加载器调用这个函数将每个段映射到正确的虚地址
	bin_size 是 bin 在文件中的大小, sgsize 是段在内存中的大小
先前条件:
	bin 不为空, va 可能不是 4KB 对齐的
结束状态:
	成功返回 0, 否则返回负值 */
static int
load_icode_mapper(u_long va, u_int32_t sgsize,
				  u_char *bin, u_int32_t bin_size, void *user_data) {
	
	struct Env *env = (struct Env *)user_data;	// userdata 为 env 的 PCB
	struct Page *p = NULL;
	u_long offset = va - ROUNDDOWN(va, BY2PG);	// va 不足一个页面大小的部分
	int r;
	u_long i;
	void* src;
	void* dst;
	size_t len;

	/*Step 1: 将 bin 中所有的内容加载到内存 */
	for (i = 0; i < bin_size; i += BY2PG) {
		// 分配一个页面并增加引用计数
		r = page_alloc(&p);
		if (r != 0)
			return r;
		p->pp_ref++;

		// 考虑不同的复制情况
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

		// 更新页映射
		r = page_insert(env->env_pgdir, p, va + i, PTE_V | PTE_R);
		if (r != 0)
			return r;
	}

	/*Step 2: 如果 bin_size < sgsize, 分配页面达到 sgsize, i 是现在 bin_size 的值 */
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


/*  为用户进程设立初始化栈和二进制程序
	这个函数使用 ELF 加载器加载完整的二进制映像, 到进程的用户内存
	二进制映像的入口由 ELF 加载器提供
	这个函数将一个页面映射到进程的初始化栈, 其位于虚地址的 USTACKTOP - BY2PG
	Hints: 所有的映射都是 read/write , 包括代码段的 */
static void
load_icode(struct Env *e, u_char *binary, u_int size) {
	/* Hint: 必须知道为了 创造出的不同映射 各自需要哪些权限
			 记住 binary 映像是 .out 格式, 包含了代码和数据 */
	struct Page *p = NULL;
	u_long entry_point;
	u_long r;
	u_long perm;

	/* Step 1: 分配一个页面 */
	r = page_alloc(&p);
	if (r != 0)
		return;

	/* Step 2: 使用适当的权限为新进程设置初始化栈 */
	/* Hint: 用户栈应当可写吗? */
	perm = PTE_V | PTE_R;
	page_insert(e->env_pgdir, p, USTACKTOP - BY2PG, perm);

	/* Step 3: 调用 load_elf 将ELF 文件真正加载到内存中 */
	// 将 load_icode_mapper 函数作为参数传入 load_elf 中
	r = load_elf(binary, size, &entry_point, e, load_icode_mapper);
	if (r != 0)
		return;
	
	/* Step 4: 将PC寄存器设置为适当的值 */
	e->env_tf.pc = entry_point;
}


/*  使用 env_alloc 创建一个新进程, 使用 load_icode 加载 ELF 二进制文件, 并设置优先级
	这个函数只在内核初始化时调用, 在运行第一个用户模式的进程之前
	Hints: 这个函数包装了 env_alloc 和 load_icode */
void
env_create_priority(u_char *binary, int size, int priority) {
	struct Env *e;
	
	/*Step 1: 使用 env_alloc 创建一个新进程 */
	int r;
	r = env_alloc(&e, 0);
	if (r != 0)
		return;

	/*Step 2: 设置新进程的优先级 */
	e->env_pri = priority;

	/*Step 3: 使用 load_icode 加载 ELF 二进制文件 */
	load_icode(e, binary, size);

	// 加入就绪链表
	LIST_INSERT_HEAD(&env_sched_list[0], e, env_sched_link);
}


/*  创建一个默认优先级(1)的新进程 */
void
env_create(u_char *binary, int size) {
	/* Step 1: 使用 env_create_priority 创建一个优先级为 1 的进程 */
	env_create_priority(binary, size, 1);
}


/*  释放进程 e 和其使用的所有内存 */
void
env_free(struct Env *e) {
	Pte *pt;
	u_int pdeno, pteno, pa;

	/* 记录进程的死亡 */
	printf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

	/* 清除地址空间中用户区域的所有页映射 */
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		/* 仅查看映射页表 */
		if (!(e->env_pgdir[pdeno] & PTE_V)) {
			continue;
		}
		/* 找到页表的 pa 和 va */
		pa = PTE_ADDR(e->env_pgdir[pdeno]);
		pt = (Pte *)KADDR(pa);
		/* 清除页表中所有页表项 */
		for (pteno = 0; pteno <= PTX(~0); pteno++)
			if (pt[pteno] & PTE_V) {
				page_remove(e->env_pgdir, (pdeno << PDSHIFT) | (pteno << PGSHIFT));
			}
		/* 释放页表本身 */
		e->env_pgdir[pdeno] = 0;
		page_decref(pa2page(pa));
	}

	/* 释放页目录 */
	pa = e->env_cr3;
	e->env_pgdir = 0;
	e->env_cr3 = 0;
	page_decref(pa2page(pa));

	/* 把进程送回空闲链表并且从就绪链表中去除 */
	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD(&env_free_list, e, env_link);
	LIST_REMOVE(e, env_sched_link);
}


/* 释放进程 e , 如果 e 是当前进程, 则选择一个新的进程执行 */
void
env_destroy(struct Env *e) {
	env_free(e);	//释放 e

	/* 选择一个新进程执行 */
	if (curenv == e) {
		curenv = NULL;
		bcopy(	KERNEL_SP - sizeof(struct Trapframe),
				TIMESTACK - sizeof(struct Trapframe),
				sizeof(struct Trapframe));
		printf("i am killed ... \n");
		sched_yield();
	}
}


extern void env_pop_tf(struct Trapframe *tf, int id);	// 定义在 env_asm.S
extern void lcontext(u_int contxt);

/*  进程切换到 e
	用 env_pop_tf 将寄存器保存到 Trapframe, 将寄存器状态从 curenv 切换到 e */
void
env_run(struct Env *e) {
	/*Step 1: 保存当前进程 curenv 的寄存器状态 */
	/* Hint: 如果进程正在执行, 需要进行上下文切换, 可以仿照 env_destroy 的方法
			 (把 old 区域的东西拷贝到当前进程的 env_tf 中, 以达到保存进程上下文的效果) */
	struct Trapframe* old = (struct Trapframe*)(TIMESTACK - sizeof(struct Trapframe));
	if (curenv) {
		bcopy(old, &(curenv->env_tf), sizeof(struct Trapframe));
		curenv->env_tf.pc = old->cp0_epc;
	}
	
	/*Step 2: 将新进程设置为当前进程 curenv */
	curenv = e;

	/*Step 3: 使用 lcontext 切换地址空间 */
	lcontext(KADDR(curenv->env_cr3));

	/*Step 4: 使用 env_pop_tf 恢复进程的上下文(寄存器) 并进入进程的用户模式 */
	/* Hint: 这里需要使用 GET_ENV_ASID, 想想为什么? */
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
