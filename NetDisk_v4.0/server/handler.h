/*************************************************************************
    > File Name: handler.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月14日 星期二 01时04分05秒
 ************************************************************************/

#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "check.h"
#include "tcp.h"
#include "threadPool.h"
#include "processPool.h"
#include "channel.h"

/*
线程池相关
 */
void handleSocketEvent(MYSQL *mysql, int peerfd, int epfd, ptaskQueue_t pqueue, channelarray_t*);
void handleCommandEvent(MYSQL *mysql, int peerfd, int epfd, ptaskQueue_t pqueue, channelarray_t*);
void handleLongCommandEvent(MYSQL *mysql, int peerfd, ptaskQueue_t pqueue);
void handleMsgOut(int sockfd, int epfd, ptaskQueue_t pqueue, channelarray_t*);
void *socketTask(void *pArgs);    
void *commandTask(void *pArgs);
void *longCommandTask(void *pArgs);
void *sendTask(void *pArgs);

/*
进程池相关
 */
//处理连接请求                                                                            
void handleConnRequest(int listenfd, processData_t p[], int processNum);
void handleLongCommandRequest(int listenfd, processData_t p[], int processNum);
//处理其他请求
void handleMsg(processData_t p[], int processNum, int fd, int epfd);
//当pipefd为负时，寻找第一个空闲的子进程；否则寻找管道文件描述符为pipefd的子进程，返回下标
int getIndex(processData_t p[], int processNum, int pipefd);
//寻找活跃线程数最少的子进程                                
int findTheMostFreeChild(processData_t p[], int processNum);

#endif
