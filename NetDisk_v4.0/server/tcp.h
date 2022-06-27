/*************************************************************************
    > File Name: tcp.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年05月30日 星期一 19时09分54秒
 ************************************************************************/
#ifndef _TCP_H_
#define _TCP_H_
#include "check.h"

#define PORT1 1234
#define PORT2 1235

//socket相关操作
int clientInit(char *serverIP, uint16_t port);
int tcpSockInit(uint16_t port);                                      //初始化套接字，返回套接字文件描述符
void setNonblock(int fd);                               //为文件描述符添加非阻塞特性
int myaccept(int listenfd);                             //accept函数的封装，返回accept函数的返回值

//epoll相关操作
int epollInit(int *pepfd, struct epoll_event **pevtList);
int epollAddNewFd(int newfd, int epfd);                 //将文件描述符newfd加入读监听
int epollDelFd(int fd, int epfd);                       //将文件描述符fd移除监听并关闭连接
int epollDelFd1(int fd, int epfd);                       //将文件描述符fd移除监听并关闭连接
void epollSetEvent(int epfd, int fd, uint32_t events);  //修改文件描述符的监听属性为events
void epollAddEvent(int epfd, int fd, uint32_t events);  //修改文件描述符的监听属性为events
void epollDelEvent(int epfd, int fd, uint32_t events);  //修改文件描述符的监听属性为events

#endif
