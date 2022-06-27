/*************************************************************************
    > File Name: processPool.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月01日 星期三 19时15分34秒
 ************************************************************************/

#include "processPool.h"
#include "transferMsg.h"
#include "threadPool.h"
#include "handler.h"
#include "userLogIn.h"
#include "channel.h"
#include "timeOutQueue.h"
#include "mylog.h"

#define SETTIMEOUT 60

//接收进程退出信号的管道
int exitPipeFd[2];
channelarray_t channelArray;
timeOutQueue_t timeOutQueue;

/*
服务端的初始化，无需参数
会返回一个指向 服务端管理结构体 的指针
将exitFunc注册到了SIGUSR1信号下
*/
pserManager_t serverInit(void){
    pserManager_t p =(pserManager_t)calloc(1,sizeof(serManager_t));
    pipe(exitPipeFd);
    p->exitfd = exitPipeFd[0];
    p->processNum = CHILDNUM;
    tcpInit(&p->listenfd, &p->epfd, &p->pevtList);
    p->listenfd2 = tcpSockInit(PORT2);
    listen(p->listenfd2, 100);
    if(epollAddNewFd(p->listenfd2, p->epfd) >= 0){
        puts("PORT2 status: Listening...");                                   //初始化阶段完成
    }
    p->processArray = (processData_t*)calloc(p->processNum, sizeof(processData_t));
    makeChild(p->processArray, p->processNum, p->epfd);
    epollAddNewFd(p->exitfd,p->epfd);
    signal(SIGUSR1, exitFunc);
    return p;
}

/*
tcp连接的初始化
并将listenfd加入监听
 */
void tcpInit(int *plistenfd, int *pepfd, struct epoll_event **pevtList){
    //printf("Before: listenfd = %d\n", *plistenfd);
    *plistenfd = tcpSockInit(PORT1);                                     //初始化TCP连接套接字
    int ret = listen(*plistenfd, 100);
    //printf("After: listenfd = %d\n", *plistenfd);
    epollInit(pepfd, pevtList);
    if(epollAddNewFd(*plistenfd, *pepfd) < 0){
        exit(EXIT_FAILURE);
    }
    
    usleep(50000);
    puts("PORT1 status: Listening...");                                   //初始化阶段完成
}                  

/*
创建子进程
将子进程相关信息记录在进程数组中
子线程执行handleEvent函数
 */
int makeChild(processData_t p[], int processNum, int epfd){
    for(int i=0; i<processNum;){
        int fds[2];
		int logfd;
        socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);                  //为每一个子进程建立一个管道
        pid_t pid = fork();                                         //开辟子进程
        if(pid == -1 && EINTR != errno){
            perror("fork");
        } 
        if(0 == pid) {
            close(fds[1]);
			//为每个子进程单独创建日志文件
			char logFileName[100] = { 0 };
			sprintf(logFileName, "serverlog%d.txt", i);
			logfd = open(logFileName, O_RDWR | O_APPEND | O_CREAT, 0666);
			logInit(logfd);
            handleEvent(fds[0]);                                    //子进程处理函数
            close(fds[0]);
            exit(0);
        }
        close(fds[0]);
        p[i].pid = pid;                                             //父进程保存子进程信息
        p[i].fd = fds[1];
        p[i].status = FREE;
        epollAddNewFd(fds[1], epfd);
        p[i].threadNum = 0;
        ++i;
    }
    return 0;
}

int makeOneChild(processData_t *p, int epfd, int idx){
        int fds[2];
        socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);                  //为每一个子进程建立一个管道
        pid_t pid = fork();                                         //开辟子进程
        if(pid == -1 && EINTR != errno){
            perror("fork");
        } 
        if(0 == pid) {
            close(fds[1]);
            handleEvent(fds[0]);                                    //子进程处理函数
            close(fds[0]);
            exit(0);
        }
        close(fds[0]);
        p->pid = pid;                                             //父进程保存子进程信息
        p->fd = fds[1];
        p->status = FREE;
        epollAddNewFd(fds[1], epfd);
        p->threadNum = 0;
        return pid;
}

/*
根据信号退出
向进程退出管道写入信息
主进程收到信号后将有序退出
 */
void exitFunc(int signum){
    printf("Received a signal: %d\n", signum);
    write(exitPipeFd[1],"1",1);
}

/*
子进程执行函数
先创建子线程
 */
void handleEvent(int pipefd){
    printf("Child %d is ready.\n", getpid());
    
    //建立线程池
    factory_t factory;
    memset(&factory, 0, sizeof(factory));
    int ret = factoryInit(&factory, THREADNUM);
    
    //连接数据库
    MYSQL *mysql = mysql_init(NULL);
    mysqlConnect(mysql);

    //建立监听结构
    int epfd;
    struct epoll_event *pevtList;
    epollInit(&epfd, &pevtList);
    epollAddNewFd(pipefd, epfd);

    //初始化通道数组管理连接
    channelArrayInit(&channelArray);
    channelArray.epfd = epfd;

    //初始化时间轮转退出机制结构S
    timeOutQueueInit(&timeOutQueue, SETTIMEOUT);

    struct timeval lastTime, nowTime;
    gettimeofday(&lastTime, NULL);
    while(1){
        int peerfd;
        gettimeofday(&nowTime, NULL);
        if(nowTime.tv_sec > lastTime.tv_sec){
            autoTimer(&timeOutQueue, epfd, &channelArray);
            lastTime.tv_sec = nowTime.tv_sec;
            lastTime.tv_usec = nowTime.tv_usec;
        }

        int ret = epoll_wait(epfd, pevtList, MAXEVENTS, 1000);
        //printf("%d个请求.\n", ret);
        for(int i = 0; i < ret; ++i){
            int fd = pevtList[i].data.fd;
            if(fd == pipefd){
                char exitFlag;
                ret = recvfd(pipefd, &peerfd, &exitFlag);
                if(0 == ret || 'y' == exitFlag){
                    puts("子进程收到退出信号");
                    mysql_close(mysql);
                    threadExit(&factory);
                    factoryDestroy(&factory);
                    timeOutQueueDestroy(&timeOutQueue);
                    epollCloseAll(&channelArray, epfd);
                    printf("Child process %d exited.\n", getpid());
                    exit(0);
                } else if('l' == exitFlag){
                    printf("\n长命令请求\n");
                    handleLongCommandEvent(mysql, peerfd, &factory.taskQueue);
                } else {
                    printf("\n连接请求\n");
                    handleSocketEvent(mysql, peerfd, epfd, &factory.taskQueue, &channelArray);
                }
            } else if(pevtList[i].events & EPOLLIN){
                printf("\n命令请求.\n");
                handleCommandEvent(mysql, fd, epfd, &factory.taskQueue, &channelArray);
            } else if(pevtList[i].events & EPOLLOUT){
                printf("\n写请求.\n");
                handleMsgOut(fd, epfd, &factory.taskQueue,  &channelArray);
            } else {
                printf("\n其他请求.\n");
            }
        }
        write(pipefd, &channelArray.count, sizeof(int));
    }

    printf("Child process %d exited unexpectedlly.\n", getpid());
    exit(0);
}


/*
子进程退出
 */
void childExit(pserManager_t p){
    if(NULL == p){
        return ;
    }
    for(int i=0; i<p->processNum ; ++i){
        sendfd(p->processArray[i].fd, p->processArray[i].fd, 'y');
        epollDelFd(p->processArray[i].fd, p->epfd);
    }

    for(int i=0; i<p->processNum; ++i){
        wait(NULL);
    }

    for(int i=0; i<p->processNum; ++i){
        close(p->processArray[i].fd);
    }
}

/*
服务端管理结构体的销毁
 */
void serDestroy(pserManager_t p){
    if(NULL == p){
        return;
    }
    free(p->pevtList);
    free(p->processArray);
    close(p->listenfd);
    close(exitPipeFd[1]);
    close(exitPipeFd[0]);
    close(p->epfd);
}

