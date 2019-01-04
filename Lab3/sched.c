#include "env.h"
#include "pmap.h"
#include "printf.h"

/*  ��鵱ǰ�����Ƿ�������ʱ��Ƭ, �����, ���������һ������״̬��������
	������� env_sched_list[pos] �ǲ��ǿյ�, �����, ����ָ��ָ����һ������
	����, �����ǰָ����б�ǿ�, ������һ�����̲�Ϊ�����ʱ�� */
void sched_yield(void) {
	static int pos = 0;			// ��¼��ǰ��������, 0 ���� 1
	static u_int times = 0;		// ʱ��Ƭ����
	static struct Env *e;		// ������һ������

	times++;

	// ���û�н�����ִ��
	if (curenv == NULL) {
		times = 0;
		// ���ָ��������ǲ��ǿյ�
		if (LIST_EMPTY(&env_sched_list[pos])) {
			pos = 1 - pos;
		}
		// ��ǰָ����б�ǿ�
		if (!LIST_EMPTY(&env_sched_list[pos])) {
			// ������һ������, ����ʱ��
			e = LIST_FIRST(&env_sched_list[pos]);
		}
	}

	// ʱ��Ƭ����
	else if (curenv->env_pri == times) {
		times = 0;
		LIST_REMOVE(curenv, env_sched_link);
		LIST_INSERT_HEAD(&env_sched_list[1 - pos], curenv, env_sched_link);

		// ���ָ��������ǲ��ǿյ�
		if (LIST_EMPTY(&env_sched_list[pos])) {
			pos = 1 - pos;
		}

		// ��ǰָ����б�ǿ�
		if (!LIST_EMPTY(&env_sched_list[pos])) {
			// ������һ������, ����ʱ��
			e = LIST_FIRST(&env_sched_list[pos]);
		}
	}

	// ʱ��Ƭû������
	else {
		e = curenv;
	}

	env_run(e);
	return;
}
