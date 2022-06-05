/*************************************************************************
    > File Name: processPool.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月01日 星期三 19时07分48秒
 ************************************************************************/

#ifndef _PROCESSPOOL_H_
#define _PROCESSPOOL_H_

#include <check.h>                                                                                           

#define MAXEVENTS 1000
#define PORT 1234
#define CHILDNUM 4

enum workerStatus {FREE, BUSY};

typedef struct ProcessDate{
    pid_t pid;                                  //子进程pid
    int fd;                                     //父子进程间pipe的fd
    int status;                                 //子进程状态：BUSY  FREE
} processData_t;

typedef struct SerManager{
    int listenfd;                               //处理连接请求的listenfd
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
int clientInit(char *serverIP);
//tcp连接初始化
void tcpInit(int *plistenfd, int *pepfd, processData_t **pProcessArray, int processNum, struct epoll_event **pevtList);
//初始化tcp套接字，返回套接字文件描述符
int tcpSockInit();
//初始化子进程，将相关管道加入监听
int makeChild(processData_t p[], int processNum, int epfd);
//将文件描述符加入监听
int epollAddNewFd(int newfd, int epfd);
//设置文件描述符为非阻塞
void setNonblock(int fd);
//退出信号处理
void exitFunc(int signum);

/*
 运行中的信号处理
 */
//子进程处理方法
void handleEvent(int fd);
//处理连接请求
void handleConnRequest(int listenfd, processData_t p[], int processNum);
//处理其他请求
void handleMsg(processData_t p[], int processNum, int fd);
//取下一个连接请求，返回套接字
int myaccept(int listenfd);
//当pipefd为负时，寻找第一个空闲的子进程；否则寻找管道文件描述符为pipefd的子进程，返回下标
int getIndex(processData_t p[], int processNum, int pipefd);
//回响操作
void myecho(int fd);
/*
 退出时的操作
 */
int epollDelFd(int fd, int epfd);
void childExit(pserManager_t p);
void serDestroy(pserManager_t p);
#endif
