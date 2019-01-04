#ifndef _ERROR_H_
#define _ERROR_H_

// �ں˴����� -- �� kern/printf.c ͬ��
#define E_UNSPECIFIED   1       // δָ����δ֪����
#define E_BAD_ENV       2       // ���̲�����
// ������������ʹ��
#define E_INVAL         3       // �Ƿ�����
#define E_NO_MEM        4       // �ڴ治��, ����ʧ��
#define E_NO_FREE_ENV   5       // ���н��̲���, ����ʧ��
// ��������ֵ
#define E_IPC_NOT_RECV  6       // ���͸������յĽ���


// �ļ�ϵͳ���� -- ֻ���û���
#define E_NO_DISK       7       // ������û��ʣ��ռ�
#define E_MAX_OPEN      8       // ���˹�����ļ�
#define E_NOT_FOUND     9       // �Ҳ����ļ�
#define E_BAD_PATH      10      // ����·��
#define E_FILE_EXISTS   11      // �ļ��Ѵ���
#define E_NOT_EXEC      12      // �ļ�����ִ��

#define MAXERROR		12

#endif // _ERROR_H_
