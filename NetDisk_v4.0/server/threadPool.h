/*************************************************************************
    > File Name: threadPool.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月06日 星期一 10时34分52秒
 ************************************************************************/

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "check.h"

#define THREADNUM 3

/*
用于表明任务的类型，便于可能的后续拓展
 */
enum task_types {THREADNORMAL, THREADSOCKET, THREADEXIT};

//命令的任务包，交给子线程
typedef struct task_s{
    int taskType;               //任务类型
    void* (*taskFunc)(void*);   //被执行函数的函数指针
    void *funcArgv;             //被执行函数的参数
    struct task_s *next;        //下一个任务
}task_t, *ptask_t;

//任务队列
typedef struct taskQueue_s{
    ptask_t front;              //任务队列头
    ptask_t rear;               //任务队列尾
    int queueSize;              //任务队列中待执行任务数量
    int threadNum;              //活跃的子线程数量
    pthread_mutex_t mutex;      //互斥锁
    pthread_cond_t cond;        //条件变量
}taskQueue_t, *ptaskQueue_t; 

//参数结构体
typedef struct arguments_s{
    int sockfd;
    int userId;
    int epfd;
    char *name;
    MYSQL *mysql;
    ptaskQueue_t pqueue;
    void *param;
}arguments_t, *parguments_t;

//管理线程池
typedef struct factory_s{
    pthread_t *tidArr;          //存放tid的数组，暂时无用
    int threadNum;              //线程池子线程数量
    taskQueue_t taskQueue;      //任务队列
}factory_t, *pfactory_t;

/*
 线程池相关
 */
ptask_t taskInit(int taskType);             //任务初始化
int taskQueueInit(ptaskQueue_t);            //任务队列初始化
int factoryInit(pfactory_t, int threadNum); //工厂结构体初始化
void *threadFunc(void* argv);               //子线程函数
void createOneThread(pfactory_t);           //创建一个子线程

int taskEnQueue(ptaskQueue_t, ptask_t);     //任务加入任务队列
ptask_t taskDeQueue(ptaskQueue_t);          //从任务队列取下任务
void* threadHandleSocket(void* pArgs);

void cleanFunc(void *pArgs);
int threadExit(pfactory_t);                 //子线程退出
int nThreadsExit(pfactory_t, int);
int taskQueueDestroy(ptaskQueue_t);         //销毁任务队列
int factoryDestroy(pfactory_t);             //销毁工厂结构体


/*
 单线程相关
 */

void threadNumAdd(ptaskQueue_t p);          //将活跃线程数加一，该功能被整合进taskDeQueue中了
void threadNumMinus(ptaskQueue_t p);        //将活跃的线程数减一

#endif

