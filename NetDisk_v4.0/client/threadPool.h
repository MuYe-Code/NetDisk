/*************************************************************************
    > File Name: threadPool.h
    > Author: 陈强
    > Function: 用于客户端开启子线程用于gets、puts。
    > 线程数量: 暂定10个（支持10个上传下载任务同时执行）
 ************************************************************************/

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
#include "check.h"
#include "tcp.h"

#define TASKQUEUE_NOT_EXIT_FLAG 0
#define TASKQUEUE_READY_EXIT_FLAG 1

//任务
typedef struct task_s{
    char command[1024];          //客户端输入的命令
    int taskType;           //任务类型，用于区分gets--0和puts--1
    char token[200];            //后续用于Token验证拓展
	char serverIP[200];         //服务器文件传输模块的IP地址
    uint16_t port;          //服务器文件传输模块的端口号
	struct task_s * pnext;
} task_t, *ptask_t;

//任务队列
typedef struct taskQueue_s{
	task_t * pFront;
	task_t * pRear;
	int queueSize;
	int exitFlag;           //退出的标志位
	pthread_mutex_t mutex;  //互斥锁
	pthread_cond_t cond;    //条件变量
} taskQueue_t, *ptaskQueue_t;


//线程池
typedef struct factory_s{
    pthread_t *tidAyy;
    int threadNum;          //线程池子线程数量
    taskQueue_t taskQueue;  //任务队列
}factory_t, *pfactory_t;


/*任务队列相关函数*/
//1.单个任务结构体初始化
ptask_t taskInit(char *command, int taskType, char *token, char *serverIP, uint16_t port);
//2.任务队列初始化
void taskQueueInit(ptaskQueue_t p);
//3.任务队列销毁
void taskQueueDestroy(ptaskQueue_t p);
//4.判断任务队列是否为空
int taskQueueIsEmpty(ptaskQueue_t p);
//5.获取任务队列中任务数量
int getTaskSize(ptaskQueue_t p);
//6.任务入队
void taskEnQueue(ptaskQueue_t p, ptask_t ptask);
//7.任务出队
ptask_t taskDeQueue(ptaskQueue_t p);
//8.任务队列唤醒（用于进程池退出）
void taskQueueWakeup(ptaskQueue_t p);



/*线程池相关函数*/

//1.子线程执行函数
void *threadFunc(void*);
//2.线程池初始化,包括创建threadNum个子线程
void factoryInit(pfactory_t p, int threadNum);
//3.线程池销毁
void factoryDestroy(pfactory_t p);

#endif
