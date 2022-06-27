/*************************************************************************
    > File Name: threadPool.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月06日 星期一 12时34分53秒
 ************************************************************************/

#include "threadPool.h"

#define COMMANDLEN 100

/*
初始化一个任务，将任务类型设置为taskType
taskType: THREADEXIT THREADNORMAL
 */
ptask_t taskInit(int taskType){
    ptask_t p = (ptask_t)malloc(sizeof(task_t));
    if(NULL == p){
        perror("malloc");
        return NULL;
    }
    p->taskType = taskType;
    p->taskFunc = NULL;
    p->funcArgv = NULL;
    p->next = NULL;
    return p;
}

/*
初始化任务队列 
 */
int taskQueueInit(ptaskQueue_t p){
    p->front = NULL;
    p->rear = NULL;
    p->queueSize = 0;
    p->threadNum = 0;
    pthread_cond_init(&p->cond, NULL);
    pthread_mutex_init(&p->mutex, NULL);
    return 0;
}

/*
初始化工厂结构体
启动THREADNUM个子线程
让子线程执行threadFunc函数
将任务队列当作参数传入threadFunc函数
 */
int factoryInit(pfactory_t p, int threadNum){
    if(NULL == p || threadNum <= 0){
        return -1;
    }
    p->tidArr = (pthread_t*)calloc(threadNum, sizeof(pthread_t));
    p->threadNum = threadNum;

    memset(&p->taskQueue, 0, sizeof(p->taskQueue));
    taskQueueInit(&p->taskQueue);
    
    for(int i=0; i<p->threadNum; ++i){
        int ret = pthread_create(p->tidArr + i, NULL, threadFunc, &p->taskQueue);
        if(-1 == ret){
            perror("pthread_create");
        }
    }
    return 0;
}

/*
子线程函数体
从任务队列获取任务
其参数包括 要执行的函数的指针，被执行函数的参数
当任务类型为THREADEXIT时退出
注意：获取到的任务结构体需要在本函数释放

--> 06/09
    设置分离属性，使得线程退出时自动释放资源（线程号等）
 */
void *threadFunc(void* argv){
    ptaskQueue_t pqueue = (ptaskQueue_t)argv;
    pthread_detach(pthread_self());
    printf("Thread %ld is ready.\n", pthread_self());
    while(1){
        ptask_t ptask = taskDeQueue(pqueue);
        if(THREADEXIT == ptask->taskType){
            free(ptask);
            break;
        }
        printf("Thread %ld gets a task.\n", pthread_self());
        void* (*pFunc)(void*) = ptask->taskFunc;
        void* arguments = ptask->funcArgv;
        pFunc(arguments);
        threadNumMinus(pqueue);
        printf("Thread %ld finished a task.\n", pthread_self());
        free(ptask);
    }
    
    //printf("Thread %ld exited.\n", pthread_self());
    pthread_exit(NULL);
}

/*
将一个任务加入任务队列
 */
int taskEnQueue(ptaskQueue_t p, ptask_t ptask){
    if(NULL == p || NULL == ptask){
        return -1;
    }
    pthread_mutex_lock(&p->mutex);
    if(0 == p->queueSize){
        p->front = ptask;
        p->rear = ptask;
        pthread_cond_broadcast(&p->cond);
    } else {
        p->rear->next = ptask;
        p->rear = ptask;
    }

    ++(p->queueSize);
    pthread_mutex_unlock(&p->mutex);
    return 0;
}

/*
将一个任务从任务队列取下
返回取下的任务结构体的指针
 */
ptask_t taskDeQueue(ptaskQueue_t p){
    if(NULL == p){
        return NULL;
    }
    pthread_mutex_lock(&p->mutex);
    while(0 == p->queueSize){
        pthread_cond_wait(&p->cond, &p->mutex);
    }
    
    if(1 == p->queueSize){
        p->rear = NULL;
    }
    ptask_t pret = p->front;
    p->front = pret->next;
    --(p->queueSize);                               //任务队列中任务数减一
    ++(p->threadNum);                               //活跃的线程数加一
    pthread_mutex_unlock(&p->mutex);
    return pret;
}

/*
暂未使用
 */
void cleanFunc(void *pArgs){
    printf("Thread %ld exited.\n", pthread_self());
}

/*
线程退出函数
向任务队列中发布threadNum个线程退出任务

检查任务队列中的任务数量
当任务数量为0时表明所有线程都取到了退出任务
且每个线程都已完成其任务
-->2020/6/7 修改了判断线程全部退出的语句
 */
int threadExit(pfactory_t pfactory){
    for(int i=0; i<pfactory->threadNum; ++i){
        ptask_t ptask = taskInit(THREADEXIT);
        taskEnQueue(&pfactory->taskQueue, ptask);
    }
    
    /*
    for(int i=0; i<pfactory->threadNum; ++i){
        pthread_join(*(pfactory->tidArr + i), NULL);
    }
    */
    while(0 < pfactory->taskQueue.queueSize);
    return 0;
}

int nThreadsExit(pfactory_t pfactory, int num){
    if(num > pfactory->threadNum){
        return -1;
    }

    for(int i=0; i<pfactory->threadNum; ++i){
        ptask_t ptask = taskInit(THREADEXIT);
        taskEnQueue(&pfactory->taskQueue, ptask);
    }

    return 0;
}

/*
线程队列销毁
若在线程退出任务加入到任务队列后还有新的任务被加入到队列
则之后的任务将会被销毁
注意销毁锁和条件变量
 */
int taskQueueDestroy(ptaskQueue_t p){
    if(NULL == p){
        return 0;
    }
    pthread_mutex_lock(&p->mutex);
    ptask_t ptask = p->front;
    while(p->front != NULL){
        ptask = p->front;
        p->front = ptask->next;
        free(ptask);
    }
    p->queueSize = -10000;
    pthread_mutex_unlock(&p->mutex);
    pthread_mutex_destroy(&p->mutex);
    pthread_cond_destroy(&p->cond);
    return 0;
}

/*
 销毁工厂结构体
 */
int factoryDestroy(pfactory_t p){
    taskQueueDestroy(&p->taskQueue);
    free(p->tidArr);
    return 0;
}


void threadNumAdd(ptaskQueue_t p){
    pthread_mutex_lock(&p->mutex);
    ++(p->threadNum);
    pthread_mutex_unlock(&p->mutex);
}

void threadNumMinus(ptaskQueue_t p){
    pthread_mutex_lock(&p->mutex);
    --(p->threadNum);
    //printf("Active thread of proccess %d: %d\n", getpid(), p->threadNum);
    pthread_mutex_unlock(&p->mutex);
}
