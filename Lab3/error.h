#ifndef _ERROR_H_
#define _ERROR_H_

// 内核错误码 -- 与 kern/printf.c 同步
#define E_UNSPECIFIED   1       // 未指明或未知错误
#define E_BAD_ENV       2       // 进程不存在
// 不能在请求活动中使用
#define E_INVAL         3       // 非法参数
#define E_NO_MEM        4       // 内存不足, 请求失败
#define E_NO_FREE_ENV   5       // 空闲进程不足, 创建失败
// 允许的最大值
#define E_IPC_NOT_RECV  6       // 发送给不接收的进程


// 文件系统错误 -- 只在用户级
#define E_NO_DISK       7       // 磁盘上没有剩余空间
#define E_MAX_OPEN      8       // 打开了过多的文件
#define E_NOT_FOUND     9       // 找不到文件
#define E_BAD_PATH      10      // 错误路径
#define E_FILE_EXISTS   11      // 文件已存在
#define E_NOT_EXEC      12      // 文件不可执行

#define MAXERROR		12

#endif // _ERROR_H_
