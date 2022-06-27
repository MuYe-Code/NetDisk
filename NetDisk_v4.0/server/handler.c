/*************************************************************************
    > File Name: handler.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月14日 星期二 01时07分08秒
 ************************************************************************/

#include "handler.h"
#include "userLogIn.h"
#include "command.h"
#include "transferMsg.h"
#include "channel.h"
#include "timeOutQueue.h"
#include "mylog.h"

extern timeOutQueue_t timeOutQueue;

/*
线程池相关
 */
void handleSocketEvent(MYSQL *mysql, int peerfd, int epfd, ptaskQueue_t p, channelarray_t *pchannelArr){
    ptask_t ptask = taskInit(THREADSOCKET);                                                               
    ptask->taskFunc = socketTask;
    parguments_t pArgs = (parguments_t)calloc(1,sizeof(arguments_t));
    pArgs->mysql = mysql;
    pArgs->sockfd = peerfd;
    pArgs->pqueue = p;
    pArgs->epfd = epfd;
    pArgs->param = (void*)pchannelArr;
    ptask->funcArgv = (void*)pArgs;
	//开始记录日志
	char connectcommand[20] = "client connected";
	addlog(pArgs, connectcommand);
    taskEnQueue(p, ptask);
}

void handleCommandEvent(MYSQL *mysql, int fd, int epfd,  ptaskQueue_t p, channelarray_t *pchannelArr){
    ptask_t ptask = taskInit(THREADNORMAL);                                                               
    ptask->taskFunc = commandTask;
    parguments_t pArgs = (parguments_t)calloc(1,sizeof(arguments_t));
    pArgs->mysql = mysql;
    pArgs->sockfd = fd;
    pArgs->pqueue = p;
    pArgs->epfd = epfd;
    pArgs->param = (void*)pchannelArr;
    ptask->funcArgv = (void*)pArgs;
    taskEnQueue(p, ptask);
}

void handleLongCommandEvent(MYSQL *mysql, int fd,  ptaskQueue_t p){
    ptask_t ptask = taskInit(THREADNORMAL);                                                               
    ptask->taskFunc = longCommandTask;
    parguments_t pArgs = (parguments_t)calloc(1,sizeof(arguments_t));
    pArgs->mysql = mysql;
    pArgs->sockfd = fd;
    pArgs->pqueue = p;
    pArgs->epfd = -1;
    pArgs->userId = -1;
    ptask->funcArgv = (void*)pArgs;
    taskEnQueue(p, ptask);
}

void handleMsgOut(int sockfd, int epfd, ptaskQueue_t p, channelarray_t* pchannelArr){
    ptask_t ptask = taskInit(THREADNORMAL);                                                               
    ptask->taskFunc = sendTask;
    parguments_t pArgs = (parguments_t)calloc(1,sizeof(arguments_t));
    pArgs->pqueue = p;
    pArgs->sockfd = sockfd;
    pArgs->epfd = epfd;
    pArgs->param = (void*)pchannelArr;
    ptask->funcArgv = (void*)pArgs;
    taskEnQueue(p, ptask);
}

/*
用于处理套接字任务的函数
 */
void *socketTask(void *pArgs){
    //解析参数
    parguments_t p = (parguments_t)pArgs;
    int sockfd = p->sockfd;
    MYSQL *mysql = p->mysql;
    int epfd = p->epfd;
    channelarray_t *pchannelArr = (channelarray_t*)p->param;

    int userId = logIn(sockfd, mysql);
    if(0 >= userId){
        free(pArgs);
        return 0;
    }
    int index = timeOutQueueAdd(&timeOutQueue, myTimerCreate(sockfd));
    epollAddNewFd(sockfd, epfd);
    //epollSetEvent(epfd, sockfd, EPOLLIN);
    channelAdd(pchannelArr, sockfd, userId, index);
    printf("User %d log in.\n", userId);
    free(pArgs);
}

/*
用于处理命令任务的函数
 */
void *commandTask(void *pArgs){
    char commandBuff[256] = {0};
    int ret;

    //解析参数
    parguments_t p = (parguments_t)pArgs;
    int sockfd = p->sockfd;
    MYSQL *mysql = p->mysql;
    ptaskQueue_t pqueue = p->pqueue;
    int epfd = p->epfd;
    channelarray_t *pchannelArr = (channelarray_t*)p->param;
    int idx = channelGetIndex(pchannelArr, sockfd);
    int userId = pchannelArr->array[idx].userId;
    //获取命令
    ret = recv(sockfd, commandBuff, 256, 0);
	//写入日志
	addlog(p, commandBuff);
    //出现错误
    if(-1 == ret){
        char tmp[] = "There are some errors.\n";
        sendn(sockfd, tmp, strlen(tmp), MSG_NOSIGNAL);
        printf("%s\n", tmp);
    } else if(0 == ret){
        //客户端断开连接
		//记录客户端退出
		char closecommand[20] = "client closed";
		addlog(p, closecommand);
        epollDelFd(epfd, sockfd);
        timeOutQueueDel(&timeOutQueue, sockfd, pchannelArr->array[idx].index);
        close(sockfd);
        channelDel(pchannelArr, sockfd);
    } else {
        ret = timeOutQueueUpdate(&timeOutQueue, sockfd, pchannelArr->array[idx].index);
        pchannelArr->array[idx].index = ret;
        if(0 == strlen(commandBuff)){
            puts("Empty command.");
            free(pArgs);
            return 0;
        }
        // '\n' --> '\0'
        if('\n' == commandBuff[ret - 1]){
            commandBuff[ret - 1] = '\0';
        }
        commandExecute(pqueue, sockfd, commandBuff, userId, mysql);
    }

    free(pArgs);
    return 0;
}

/*
用于处理命令任务的函数
 */
void *longCommandTask(void *pArgs){
    char commandBuff[256] = {0};
    int ret;

    //解析参数
    parguments_t p = (parguments_t)pArgs;
    int sockfd = p->sockfd;
    MYSQL *mysql = p->mysql;
    ptaskQueue_t pqueue = p->pqueue;

    int tokenCheckRet = -1;
    char token[TOKEN_LEN + 1] = {0};
    ret = recvn(sockfd, token, TOKEN_LEN, 0);
    if(0 == ret){
        close(sockfd);
        return 0;
    }

    int userId = checkToken(mysql, token);

    if(-1 == userId){
        sendn(sockfd, &tokenCheckRet, sizeof(tokenCheckRet), MSG_NOSIGNAL);
        return 0;
    }
    tokenCheckRet = 0;
    sendn(sockfd, &tokenCheckRet, sizeof(tokenCheckRet), MSG_NOSIGNAL);

    //获取命令
    ret = recv(sockfd, commandBuff, 256, 0);
	//写入日志
	addlog(p, commandBuff);
    //出现错误
    if(-1 == ret){
        char tmp[] = "There are some errors.\n";
        sendn(sockfd, tmp, strlen(tmp), MSG_NOSIGNAL);
        printf("%s\n", tmp);
    } else if(0 == ret){
        //客户端断开连接
		//记录客户端退出
		char closecommand[20] = "client closed";
		addlog(p, closecommand);
        close(sockfd);
    } else {
        if(0 == strlen(commandBuff)){
            puts("Empty command.");
            free(pArgs);
            return 0;
        }
        // '\n' --> '\0'
        if('\n' == commandBuff[ret - 1]){
            commandBuff[ret - 1] = '\0';
        }
        commandExecute(pqueue, sockfd, commandBuff, userId, mysql);
    }

    free(pArgs);
    return 0;
}

void *sendTask(void *pArgs){
    parguments_t pArgument = (parguments_t)pArgs;
    channelarray_t *pchannelArr = (channelarray_t*)pArgument->param;
    int sockfd = pArgument->sockfd;
    int epfd = pArgument->epfd;

    int idx = channelGetIndex(pchannelArr, sockfd);
    pchannel_t p = &(pchannelArr->array[idx]);
    if(p->wrBufLen > 0){
        int ret = timeOutQueueUpdate(&timeOutQueue, sockfd, pchannelArr->array[idx].index);
        pchannelArr->array[idx].index = ret;
        ret = send(p->fd, p->wrBuf, p->wrBufLen, MSG_NOSIGNAL);
        if(ret >= 0){
            p->wrBufLen -= ret;
            if(p->wrBufLen == 0){
                epollSetEvent(epfd, p->fd, EPOLLIN|EPOLLET);
            } else {
                memmove(p->wrBuf, p->wrBuf+ret, p->wrBufLen);
            }
        }
        else perror("send");
    } else{
        epollSetEvent(epfd, p->fd, EPOLLIN|EPOLLET);
    }                                                 
    free(pArgs);
}

/*
进程池相关
 */

/*
处理连接请求
将新连接交给一个空闲子进程处理
 */
void handleConnRequest(int listenfd, processData_t p[], int processNum){
    int peerfd = myaccept(listenfd);                                        //先获取就绪的套接字文件描述符
    if(-1 == peerfd){
        return;
    }

    int idx;
    do{
        idx = findTheMostFreeChild(p, processNum);                          //为该连接寻找空闲的子进程
    }while(-1 == idx);
    /*
    if(-1 == idx){
        puts("No free child program.");
        return;
    }
    */
    p[idx].status = BUSY;                                                   //将子进程设置为BUSY
    sendfd(p[idx].fd, peerfd, 'n');                                         //将该文件描述符发送给子进程
    close(peerfd);
}

void handleLongCommandRequest(int listenfd, processData_t p[], int processNum){
    int peerfd = myaccept(listenfd);                                        //先获取就绪的套接字文件描述符
    if(-1 == peerfd){
        return;
    }

    int idx;
    do{
        idx = findTheMostFreeChild(p, processNum);                          //为该连接寻找空闲的子进程
    }while(-1 == idx);
    /*
    if(-1 == idx){
        puts("No free child program.");
        return;
    }
    */
    p[idx].status = BUSY;                                                   //将子进程设置为BUSY
    sendfd(p[idx].fd, peerfd, 'l');                                         //将该文件描述符发送给子进程
    close(peerfd);
}
/*
处理子程发来的消息
本项目中为子进程发来的活跃的线程数的提醒消息
 */
void handleMsg(processData_t p[], int processNum, int fd, int epfd){
    int ret, num;
    do{
        num = 0;
        ret = read(fd, &num, sizeof(num));                          //接收子进程发送的信号
    }while(ret == -1 && EINTR == errno);

    int idx = getIndex(p, processNum, fd);                          //更新子进程信息
    if(0 == ret){                                                   //子进程意外断开
        waitpid(p[idx].pid, NULL, 0);
        printf("Child process %d exited unexpectedly.\n", p[idx].pid);
        epollDelFd(p[idx].fd, epfd);
        close(p[idx].fd);
        p[idx].pid = makeOneChild(&p[idx], epfd, idx);
    } else {
        p[idx].threadNum = num;
        //printf("Child process %d, threadNum = %d\n", getpid(), num);
    }
}

/*
 如果pipefd为负，寻找第一个空闲子进程下标
 否则寻找pipefd的下标
 */
int getIndex(processData_t p[], int processNum, int pipefd){
    if(pipefd >= 0){
        for(int i=0; i<processNum; ++i){
            if(p[i].fd == pipefd){
                return i;
            }
        }
        return -1;
    }

    for(int i=0; i<processNum; ++i){
        if(p[i].status == FREE){
            return i;
        }
    }
    return -1;
}

int findTheMostFreeChild(processData_t p[], int processNum){
    int idx = 0;
    for(int i = 0; i < processNum; ++i){
        if(0 == p[i].threadNum){
            return i;
        }

        if(p[i].threadNum < p[idx].threadNum){
            idx = i;
        }
    }
    return idx;
}

