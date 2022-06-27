/*************************************************************************
    > File Name: timeOutQueue.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月14日 星期二 17时05分28秒
 ************************************************************************/

#include "timeOutQueue.h"
#include "tcp.h"

/*
创建一个参与时间轮转法的基本结构体
 */
pmyTimer_t myTimerCreate(int sockfd){
    pmyTimer_t p = (pmyTimer_t)calloc(1, sizeof(myTimer_t));
    p->sockfd = sockfd;
    return p;
}

/*
添加新的结构体到队列中， 返回加入的队列的下标
 */
int timeOutQueueAdd(ptimeOutQueue_t pqueue, pmyTimer_t p){
    pthread_mutex_lock(&pqueue->mutex);
    int idx = (pqueue->currentIndex + pqueue->queueSize - 1) % pqueue->queueSize;
    pmyTimer_t cur = &pqueue->pqueue[idx];
    while(NULL != cur->next){
        cur = cur->next;
    }
    cur->next = p;
    pthread_mutex_unlock(&pqueue->mutex);
    return idx;
}

/*
更新sockfd位于队列中的位置， 返回新的队列位置
 */
int timeOutQueueUpdate(ptimeOutQueue_t pqueue, int sockfd, int idx){
    pthread_mutex_lock(&pqueue->mutex);
    int newIdx = (pqueue->currentIndex + pqueue->queueSize - 1) % pqueue->queueSize;
    pmyTimer_t cur = &pqueue->pqueue[idx];
    while(NULL != cur->next && sockfd != cur->next->sockfd){
        cur = cur->next;
    }
    if(NULL == cur->next){
        pthread_mutex_unlock(&pqueue->mutex);
        return -1;
    }

    pmyTimer_t target = cur->next;
    cur->next = target->next;

    cur = &pqueue->pqueue[newIdx];

    while(NULL != cur->next){
        cur = cur->next;
    }
    cur->next = target;
    target->next = NULL;
    pthread_mutex_unlock(&pqueue->mutex);
    return newIdx;
}

/*
删除一个对象
 */
int timeOutQueueDel(ptimeOutQueue_t pqueue, int sockfd, int idx){
    pthread_mutex_lock(&pqueue->mutex);
    pmyTimer_t cur = &pqueue->pqueue[idx];
    while(NULL != cur->next && cur->next->sockfd != sockfd){
        cur = cur->next;
    }
    pmyTimer_t target = cur->next;
    if(NULL != target){
        cur->next = target->next;
        free(target);
    }
    pthread_mutex_unlock(&pqueue->mutex);
    return 0;
}

/*
自动时间轮转，要求一秒调用一次
 */
int autoTimer(ptimeOutQueue_t p, int epfd, channelarray_t* pchannel){
    pthread_mutex_lock(&p->mutex);
    p->currentIndex = (1 + p->currentIndex) % p->queueSize;
    /*
    if(0 == p->currentIndex % 10){
        printf("Time index = %d\n", p->currentIndex);
    }
    */
    pmyTimer_t cur = &p->pqueue[p->currentIndex];
    while(NULL != cur->next){
        pmyTimer_t target = cur->next;
        cur->next = target->next;
        printf("sockfd %d time out.\n", target->sockfd);
        epollDelFd(target->sockfd, epfd);
        channelDel(pchannel, target->sockfd);
        close(target->sockfd);
        free(target);
    }
    pthread_mutex_unlock(&p->mutex);
    return p->currentIndex;
}

/*
时间轮转队列初始化
 */
int timeOutQueueInit(ptimeOutQueue_t p, int timeOut){
    p->currentIndex = 0;
    p->queueSize = timeOut;
    p->pqueue = (pmyTimer_t)calloc(timeOut, sizeof(myTimer_t));
    pthread_mutex_init(&p->mutex, NULL);
    return 0;
}

/*
摧毁时间队列
 */
int timeOutQueueDestroy(ptimeOutQueue_t p){
    for(int i = 0; i < p->queueSize; ++i){
        pmyTimer_t cur = &p->pqueue[i];
        while(NULL != cur->next){
            pmyTimer_t target = cur->next;
            cur->next = target->next;
            free(target);
        }
    }
    free(p->pqueue);
}
