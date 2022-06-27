/*************************************************************************
    > File Name: tcp.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年05月30日 星期一 19时13分22秒
 ************************************************************************/

#include "tcp.h"

#define MAXEVENTS 1000

//socket相关操作

/*
客户端初始化，需要服务端的IP地址作为参数
返回连接好的套接字文件描述符
 */
int clientInit(char *serverIP, uint16_t port){
    //create socket
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(sfd,-1,"socket");
    struct sockaddr_in serAddr;
    memset(&serAddr,0,sizeof(serAddr));
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(serverIP);
    serAddr.sin_port = htons(port);
    
    //connect
    int ret = connect(sfd,(struct sockaddr*)&serAddr,sizeof(serAddr));
    ERROR_CHECK(ret,-1,"connet");                                     
    printf("Connection to %s established\n\n",serverIP);
    
    return sfd;
}

int tcpSockInit(uint16_t port){
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    ERROR_CHECK(listenfd, -1, "socket");
    struct sockaddr_in serAddr;
    memset(&serAddr, 0, sizeof(struct sockaddr_in));
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serAddr.sin_port = htons(port);
    int reuse = 1;
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_CHECK(ret, -1, "setsockopt");
    ret = bind(listenfd, (struct sockaddr*)&serAddr, sizeof(struct sockaddr_in));
    ERROR_CHECK(ret, -1, "bind");
    return listenfd;
}

void setNonblock(int fd){
    int ret = 0, status = 0;
    status = fcntl(fd, F_GETFL, 0);
    status |= O_NONBLOCK;
    fcntl(fd, F_SETFL, status);
}

int myaccept(int listenfd){
    struct sockaddr_in cliAddr;
    socklen_t len = sizeof(cliAddr);
    memset(&cliAddr,0,len);
    //printf("Listenfd = %d\n", listenfd);
    int newfd = accept(listenfd, (struct sockaddr*)&cliAddr,&len);
    //printf("Process %d\n", getpid());
    //ERROR_CHECK(newfd, -1, "accept");
    if(-1 == newfd){
        perror("accept");
    }
    struct in_addr cliIP = cliAddr.sin_addr;
    printf("Connection from IP: %s, PORT: %d\n", inet_ntoa(cliIP), ntohs(cliAddr.sin_port));
    return newfd;
}

//epoll相关操作
/*
建立监听的数据结构
 */
int epollInit(int *pepfd, struct epoll_event **pevtList){
    *pevtList = (struct epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
    *pepfd = epoll_create1(0);                                      //建立I/O多路复用数据结构
    ERROR_CHECK(*pepfd, -1, "epoll_create1");
    return 0;
}

int epollAddNewFd(int newfd, int epfd){
    setNonblock(newfd);
    struct epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.data.fd = newfd;
    evt.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, newfd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    }
    return ret;
}

int epollDelFd(int fd, int epfd){
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    } else {
        puts("One connection closed.");
    }
    close(fd);
    return ret;
}

void epollSetEvent(int epfd, int fd, uint32_t events){
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = events;
    int ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    }
}

void epollAddEvent(int epfd, int fd, uint32_t events){
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = events;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    }
}

void epollDelEvent(int epfd, int fd, uint32_t events){
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = events;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    }
}



