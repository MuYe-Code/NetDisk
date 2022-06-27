/*************************************************************************
    > File Name: timeOutQueue.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月14日 星期二 16时37分25秒
 ************************************************************************/

#ifndef __TIMEOUTQUEUE_H__
#define __TIMEOUTQUEUE_H__

#include "check.h"
#include "channel.h"

typedef struct myTimer_s{
    int sockfd;
    struct myTimer_s *next;
}myTimer_t, *pmyTimer_t;

typedef struct timeOutQueue_s{
    int currentIndex;
    int queueSize;
    pmyTimer_t pqueue;
    pthread_mutex_t mutex;
}timeOutQueue_t, *ptimeOutQueue_t;

pmyTimer_t myTimerCreate(int sockfd);
int timeOutQueueAdd(ptimeOutQueue_t, pmyTimer_t);
int timeOutQueueUpdate(ptimeOutQueue_t, int sockfd, int idx);
int timeOutQueueDel(ptimeOutQueue_t, int sockfd, int idx);
int autoTimer(ptimeOutQueue_t, int epfd, channelarray_t*);

int  timeOutQueueInit(ptimeOutQueue_t, int timeOut);
int myTimerDestroy(pmyTimer_t);
int timeOutQueueDestroy(ptimeOutQueue_t);

#endif
