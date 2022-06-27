/*************************************************************************
    > File Name: processPool.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月01日 星期三 19时07分48秒
 ************************************************************************/

#ifndef _PROCESSPOOL_H_
#define _PROCESSPOOL_H_

#include "check.h"                                                                                           
#include "command.h"
#include "tcp.h"

#define CHILDNUM 3

enum workerStatus {FREE, BUSY};

typedef struct ProcessDate{
    pid_t pid;                                  //子进程pid
    int fd;                                     //父子进程间pipe的fd
    int threadNum;
    int status;                                 //子进程状态：BUSY  FREE
} processData_t;

typedef struct SerManager{
    int listenfd;                               //处理连接请求的listenfd
    int listenfd2;
    int epfd;                                   //用于监听的实例化对象
    int exitfd;                                 //监听退出信号的管道 SIGUSR1
    struct epoll_event *pevtList;             //存放就绪的文件描述符
    int processNum;                           //子进程数量                                                                                                                                                            
    processData_t *processArray;              //存放每个子进程的信息
}serManager_t,*pserManager_t;

/*
 tcp连接相关操作及初始化
 */

//整个进程的初始化
pserManager_t serverInit(void);
//tcp连接初始化
void tcpInit(int *plistenfd, int *pepfd, struct epoll_event **pevtList);
//初始化子进程，将相关管道加入监听
int makeChild(processData_t p[], int processNum, int epfd);
//创建一个子进程
int makeOneChild(processData_t *p, int epfd, int idx);
//退出信号处理
void exitFunc(int signum);

/*
 运行中的信号处理
 */
//子进程处理方法
void handleEvent(int fd);

/*
 退出时的操作
 */
void childExit(pserManager_t p);
void serDestroy(pserManager_t p);
#endif
