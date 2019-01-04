#ifndef _ENV_H_
#define _ENV_H_

#include "types.h"
#include "queue.h"
#include "trap.h"
#include "mmu.h"

#define LOG2NENV        10
#define NENV            (1<<LOG2NENV)			// ���̸���
#define ENVX(envid)     ((envid) & (NENV - 1))	// ������ envs �е����
#define GET_ENV_ASID(envid) (((envid)>>11)<<6)	// ���� TLB ������

// Env �� env_status ��ֵ
#define ENV_FREE			0	// ���, ���ڽ��̿���������
#define ENV_RUNNABLE        1	// ����״̬, �������������е�, Ҳ���ܲ���������
#define ENV_NOT_RUNNABLE    2	// ��������״̬

struct Env {
	struct Trapframe env_tf;        // ����Ĵ���(�����Ļ���)
	LIST_ENTRY(Env) env_link;       // �����������
	u_int env_id;                   // ���̱�ʶ��
	u_int env_parent_id;            // �����̵� env_id
	u_int env_status;               // ����״̬(3��ȡֵ)
	Pde  *env_pgdir;                // ����ҳĿ¼���ں����ַ
	u_int env_cr3;					// ����ҳĿ¼�������ַ
	LIST_ENTRY(Env) env_sched_link;	// �����������״̬��������
	u_int env_pri;					// ���ȼ�

	// Lab 4 ���̼�ͨ��
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;            // va at which to map received page
	u_int env_ipc_perm;             // perm of page mapping received

	// Lab 4 ȱ�ݴ���
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 ���ȼ���
	u_int env_runs;                 // number of times been env_run'ed
	u_int env_nop;                  // align to avoid mul instruction 
};

LIST_HEAD(Env_list, Env);				// ��������
extern struct Env *envs;				// ���н���
extern struct Env *curenv;				// ��ǰ���еĽ���
extern struct Env_list env_sched_list[2];	// ���Ƚ�������

void env_init(void);
int  env_alloc(struct Env **new, u_int parent_id);
void env_free(struct Env *);
void env_create_priority(u_char *binary, int size, int priority);
void env_create(u_char *binary, int size);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e);

void env_check();


// ����ָ�����ȼ��Ľ���
#define ENV_CREATE_PRIORITY(x, y) \
{\
        extern u_char binary_##x##_start[]; \
        extern u_int binary_##x##_size;\
        env_create_priority(binary_##x##_start, \
                (u_int)binary_##x##_size, y);\
}

// ����Ĭ�����ȼ�(1)�Ľ���
#define ENV_CREATE(x) \
{ \
        extern u_char binary_##x##_start[];\
        extern u_int binary_##x##_size; \
        env_create(binary_##x##_start, \
                (u_int)binary_##x##_size); \
}

// �������̼��װ�
#define ENV_CREATE2(x, y) \
{ \
        extern u_char x[], y[]; \
        env_create(x, (int)y); \
}


#endif // !_ENV_H_
