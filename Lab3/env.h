#ifndef _ENV_H_
#define _ENV_H_

#include "types.h"
#include "queue.h"
#include "trap.h"
#include "mmu.h"

#define LOG2NENV        10
#define NENV            (1<<LOG2NENV)			// 进程个数
#define ENVX(envid)     ((envid) & (NENV - 1))	// 进程在 envs 中的序号
#define GET_ENV_ASID(envid) (((envid)>>11)<<6)	// 进程 TLB 的索引

// Env 中 env_status 的值
#define ENV_FREE			0	// 不活动, 处于进程空闲链表中
#define ENV_RUNNABLE        1	// 就绪状态, 可以是正在运行的, 也可能不在运行中
#define ENV_NOT_RUNNABLE    2	// 处于阻塞状态

struct Env {
	struct Trapframe env_tf;        // 保存寄存器(上下文环境)
	LIST_ENTRY(Env) env_link;       // 链表遍历变量
	u_int env_id;                   // 进程标识符
	u_int env_parent_id;            // 父进程的 env_id
	u_int env_status;               // 进程状态(3种取值)
	Pde  *env_pgdir;                // 进程页目录的内核虚地址
	u_int env_cr3;					// 进程页目录的物理地址
	LIST_ENTRY(Env) env_sched_link;	// 用来构造就绪状态进程链表
	u_int env_pri;					// 优先级

	// Lab 4 进程间通信
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;            // va at which to map received page
	u_int env_ipc_perm;             // perm of page mapping received

	// Lab 4 缺陷处理
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 调度计数
	u_int env_runs;                 // number of times been env_run'ed
	u_int env_nop;                  // align to avoid mul instruction 
};

LIST_HEAD(Env_list, Env);				// 进程链表
extern struct Env *envs;				// 所有进程
extern struct Env *curenv;				// 当前运行的进程
extern struct Env_list env_sched_list[2];	// 调度进程链表

void env_init(void);
int  env_alloc(struct Env **new, u_int parent_id);
void env_free(struct Env *);
void env_create_priority(u_char *binary, int size, int priority);
void env_create(u_char *binary, int size);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e);

void env_check();


// 创建指定优先级的进程
#define ENV_CREATE_PRIORITY(x, y) \
{\
        extern u_char binary_##x##_start[]; \
        extern u_int binary_##x##_size;\
        env_create_priority(binary_##x##_start, \
                (u_int)binary_##x##_size, y);\
}

// 创建默认优先级(1)的进程
#define ENV_CREATE(x) \
{ \
        extern u_char binary_##x##_start[];\
        extern u_int binary_##x##_size; \
        env_create(binary_##x##_start, \
                (u_int)binary_##x##_size); \
}

// 创建进程简易版
#define ENV_CREATE2(x, y) \
{ \
        extern u_char x[], y[]; \
        env_create(x, (int)y); \
}


#endif // !_ENV_H_
