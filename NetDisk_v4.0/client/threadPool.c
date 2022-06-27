#include "threadPool.h"
#include "cliCommand.h"


/*任务队列相关函数*/
//1.单个任务结构体初始化
ptask_t taskInit(char *command, int taskType, char *token, char *serverIP, uint16_t port){
    ptask_t ptask = (ptask_t)malloc(sizeof(task_t));
    if(NULL == ptask){
        perror("malloc");
        return NULL;
    }
    strcpy(ptask->command, command);
    ptask->taskType = taskType;
    strcpy(ptask->token, token);
    strcpy(ptask->serverIP, serverIP);
    ptask->port = port;
    ptask->pnext = NULL;
    return ptask;
}

//2.任务队列初始化
void taskQueueInit(ptaskQueue_t p){
    if(p){
        p->pFront = NULL;
        p->pRear = NULL;
        p->queueSize = 0;
        p->exitFlag = 0;
        int ret = pthread_mutex_init(&p->mutex, NULL);
        THREAD_ERROR_CHECK(ret, "pthread_mutex_init");
        ret = pthread_cond_init(&p->cond, NULL);
        THREAD_ERROR_CHECK(ret, "pthread_cond_init");
    }
}

//3.任务队列销毁
void taskQueueDestroy(ptaskQueue_t p){
    if(p){
        int ret = pthread_mutex_destroy(&p->mutex);
        THREAD_ERROR_CHECK(ret, "pthread_mutex_destroy");
        ret = pthread_cond_destroy(&p->cond);
        THREAD_ERROR_CHECK(ret, "pthread_cond_destory");
    }
}

//4.判断任务队列是否为空
int taskQueueIsEmpty(ptaskQueue_t p){
    return p->queueSize == 0;
}

//5.获取任务队列中任务数量
int getTaskSize(ptaskQueue_t p){
    return p->queueSize;
}

//6.任务入队
void taskEnQueue(ptaskQueue_t p, ptask_t ptask){
    int ret = pthread_mutex_lock(&p->mutex);
    THREAD_ERROR_CHECK(ret, "pthread_mutex_lock");
    if(taskQueueIsEmpty(p)){
        p->pFront = ptask;
        p->pRear = ptask;
    }else{
        p->pRear->pnext = ptask;
        p->pRear = ptask;
    }
    ++(p->queueSize);
    ret = pthread_mutex_unlock(&p->mutex);
    THREAD_ERROR_CHECK(ret, "pthread_mutex_unlock");
    //通知消费者线程取任务
    ret = pthread_cond_signal(&p->cond);
}

//7.任务出队
ptask_t taskDeQueue(ptaskQueue_t p){
    int ret = pthread_mutex_lock(&p->mutex);
    THREAD_ERROR_CHECK(ret, "pthread_mutex_lock");
    //当队列为空时，进入等待状态
    while(!p->exitFlag && taskQueueIsEmpty(p)){ //使用while是为了防止虚假唤醒
        pthread_cond_wait(&p->cond, &p->mutex);
    }
    ptask_t ptask = NULL;
    if(!p->exitFlag){
        ptask = p->pFront;
        if(getTaskSize(p) > 1){
            p->pFront = p->pFront->pnext;
        }else{
            p->pFront = p->pRear = NULL;
        }
        --(p->queueSize);
    }
    ret = pthread_mutex_unlock(&p->mutex);
    THREAD_ERROR_CHECK(ret, "pthread_mutex_unlock");

    return ptask; //任务执行完成记得free该堆空间
}

//8.任务队列唤醒（用于进程池退出）
void taskQueueWakeup(ptaskQueue_t p){
    p->exitFlag = TASKQUEUE_READY_EXIT_FLAG;
    int ret = pthread_cond_broadcast(&p->cond);
    THREAD_ERROR_CHECK(ret, "pthread_cond_broadcast");
}


/*线程池相关函数*/

//1.子线程执行函数
void *threadFunc(void* argv){
    ptaskQueue_t pqueue = (ptaskQueue_t)argv;
    printf("Thread %ld is ready.\n", pthread_self());
    while(1){
        ptask_t ptask = taskDeQueue(pqueue);
        if(NULL == ptask){ //没有任务
            break;
        }
        //printf("Thread %ld gets a taskType %d. (0:gets 1:puts)\n", pthread_self(), ptask->taskType);

        //1.和文件服务器建立TCP连接
        int sockfd = clientInit(ptask->serverIP, ptask->port);
        
        //printf("token = %s\n", ptask->token);
        sendn(sockfd, ptask->token, strlen(ptask->token), 0);

        int tokenCheck = 0;
        int ret = recvn(sockfd, &tokenCheck, sizeof(tokenCheck), 0);
        if(0 != ret && 0 == tokenCheck){
            puts("token验证成功");
            //2.根据任务类型进行交互
            if(0 == ptask->taskType){ //gets命令处理
                CLigets(ptask->command, sockfd);

            }else if(1 == ptask->taskType){ //puts命令处理
                CLiputs(ptask->command, sockfd);
            }
        } else {
            puts("token验证失败");
        }
        free(ptask);    //释放堆空间
        close(sockfd);  //关闭连接
    }
    printf("Thread %ld is exited.\n", pthread_self());
    pthread_exit(0);
}


//2.线程池初始化,包括创建threadNum个子线程
void factoryInit(pfactory_t p, int threadNum){
    p->threadNum = threadNum;
    p->tidAyy = (pthread_t*)calloc(threadNum, sizeof(pthread_t));

    taskQueueInit(&p->taskQueue);

    //创建threadNum个子线程
    for(int idx = 0; idx < threadNum; ++idx){
        int ret = pthread_create(&p->tidAyy[idx], NULL, threadFunc, &p->taskQueue);
        //printf("tid = %lu\n", p->tidAyy[idx]);
        THREAD_ERROR_CHECK(ret, "pthread_create");
    }
}

//3.线程池销毁
void factoryDestroy(pfactory_t p){
    free(p->tidAyy);
    taskQueueDestroy(&p->taskQueue);
}
