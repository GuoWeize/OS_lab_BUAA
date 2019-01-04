#include "env.h"
#include "pmap.h"
#include "printf.h"

/*  检查当前进程是否用完了时间片, 如果是, 将其插入另一个就绪状态进程链表
	检查链表 env_sched_list[pos] 是不是空的, 如果是, 更改指针指向另一个链表
	接着, 如果当前指向的列表非空, 弹出第一个进程并为其分配时间 */
void sched_yield(void) {
	static int pos = 0;			// 记录当前就绪链表, 0 或者 1
	static u_int times = 0;		// 时间片计数
	static struct Env *e;		// 保存上一个进程

	times++;

	// 如果没有进程在执行
	if (curenv == NULL) {
		times = 0;
		// 检查指向的链表是不是空的
		if (LIST_EMPTY(&env_sched_list[pos])) {
			pos = 1 - pos;
		}
		// 当前指向的列表非空
		if (!LIST_EMPTY(&env_sched_list[pos])) {
			// 弹出第一个进程, 分配时间
			e = LIST_FIRST(&env_sched_list[pos]);
		}
	}

	// 时间片用完
	else if (curenv->env_pri == times) {
		times = 0;
		LIST_REMOVE(curenv, env_sched_link);
		LIST_INSERT_HEAD(&env_sched_list[1 - pos], curenv, env_sched_link);

		// 检查指向的链表是不是空的
		if (LIST_EMPTY(&env_sched_list[pos])) {
			pos = 1 - pos;
		}

		// 当前指向的列表非空
		if (!LIST_EMPTY(&env_sched_list[pos])) {
			// 弹出第一个进程, 分配时间
			e = LIST_FIRST(&env_sched_list[pos]);
		}
	}

	// 时间片没有用完
	else {
		e = curenv;
	}

	env_run(e);
	return;
}
