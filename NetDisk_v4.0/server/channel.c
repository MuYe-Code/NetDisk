/*************************************************************************
    > File Name: channel.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月14日 星期二 08时46分47秒
 ************************************************************************/

#include "channel.h"
#include "tcp.h"

//channel_t相关                                                                         
bool channelInit(pchannel_t p){
    if(NULL == p) return false;
    p->lastTime = time(NULL);
    p->fd = -1;
    p->userId = 0;
    p->index = -1;
    p->taskNumber = 0;
    //p->rdBufLen = 0;
    p->wrBufLen = 0;
    //memset(p->rdBuf,0,CHANNELBUFLEN);
    memset(p->wrBuf,0,CHANNELBUFLEN);
    pthread_mutex_init(&p->mutex, 0);
    pthread_cond_init(&p->cond, 0);
    return true;
}

bool channelArrayInit(channelarray_t* pchannels){
    if(NULL == pchannels) return false;
    pchannel_t pchannel = pchannels->array;
    for(int i=0; i<MAXEVENTS; ++i){
        channelInit(pchannel);
        ++pchannel;
    }
    pchannels->count = 0;
    pchannels->maxFdIndex = 0;
    pthread_mutex_init(&pchannels->mutex, 0);
    return true;
}

bool channelAdd(channelarray_t *pchannels, int fd, int userId, int idx){
    pthread_mutex_lock(&pchannels->mutex);
    
    if(NULL == pchannels || pchannels->count == MAXEVENTS) return false;
    int index = channelGetIndex(pchannels, -1);
    pchannels->maxFdIndex = pchannels->maxFdIndex < index? index:pchannels->maxFdIndex;
    bool ret = false;
    if(index >= 0){
        pchannels->array[index].fd = fd;
        pchannels->array[index].userId = userId;
        pchannels->array[index].index = idx;
        ++(pchannels->count);
        ret = true;
    }

    pthread_mutex_unlock(&pchannels->mutex);
    return ret;
}

bool channelDel(channelarray_t* pchannels, int fd){
    if(NULL == pchannels) return false;
    pthread_mutex_lock(&pchannels->mutex);
    int index = channelGetIndex(pchannels, fd);
    pchannels->maxFdIndex -= pchannels->maxFdIndex == index? 1:0;
    if(index >= 0){
        channelInit(&pchannels->array[index]);
        --(pchannels->count);
    }
    pthread_mutex_unlock(&pchannels->mutex);
    return true;
}

int channelGetIndex(channelarray_t* pchannels, int fd){
    if(NULL == pchannels) return -1;
    pchannel_t array = pchannels->array;
    int n = MAXEVENTS > pchannels->maxFdIndex+2? pchannels->maxFdIndex+2:MAXEVENTS;
    for(int i=0; i<n; ++i){
        if(fd == array[i].fd){
            return i;
        }
    }
    return -1;
}

int sendToChannel(channelarray_t* pchannels, int fd, int epfd, const char *sendBuff, int len){
    if(NULL == pchannels){
        return 0;
    }
    pchannel_t array = pchannels->array;
    int idx = channelGetIndex(pchannels, fd);
    pthread_mutex_lock(&array[idx].mutex);
    while(array[idx].wrBufLen >= CHANNELBUFLEN){
        pthread_cond_wait(&array[idx].cond, &array[idx].mutex);
    }
    memcpy(array[idx].wrBuf+array[idx].wrBufLen, sendBuff, len);
    array[idx].wrBufLen += len;
    pthread_cond_broadcast(&array[idx].cond);
    pthread_mutex_unlock(&array[idx].mutex);
    epollSetEvent(epfd, fd, EPOLLOUT|EPOLLIN|EPOLLET);
    return len;
}

void epollCloseAll(channelarray_t *pchannels, int epfd){
    if(pchannels == NULL) return;
    pchannel_t channel = pchannels->array;
    for(int i=0; i<MAXEVENTS; ++i){
        if(channel[i].fd != -1){
            //handleMsgOut(epfd, channel[i].fd, pchannels);
            epollDelFd(channel[i].fd, epfd);
            channelInit(&channel[i]);
        }
    }
}

//其他
bool closeTimeoutConnection(int epfd, channelarray_t *pchannels){
    time_t nowTime = time(NULL);
    int maxIndex;
    for(int i=0; i<=pchannels->maxFdIndex; ++i){
        pchannel_t p = &(pchannels->array[i]);
        if(p->fd == -1) continue;
        if(nowTime - p->lastTime > TIMEOUT && p->wrBufLen == 0){
            char sendbuf[] = ">>Server: Time out, server will close the connection.\n";
            send(p->fd, sendbuf, strlen(sendbuf), 0);
            printf("Time out. ");
            epollDelFd(p->fd, epfd);
            channelDel(pchannels, p->fd);
        } else {
            maxIndex = i;
        }
    }
    pchannels->maxFdIndex = maxIndex;
    return true;

}
