/*************************************************************************
    > File Name: processPool.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月01日 星期三 19时15分34秒
 ************************************************************************/

#include "processPool.h"
#include "transferMsg.h"
#include "command.h"

int exitPipeFd[2];

pserManager_t serverInit(void){
    pserManager_t p =(pserManager_t)calloc(1,sizeof(serManager_t));
    pipe(exitPipeFd);
    p->exitfd = exitPipeFd[0];
    p->processNum = CHILDNUM;
    tcpInit(&p->listenfd, &p->epfd, &p->processArray, p->processNum, &p->pevtList);
    epollAddNewFd(p->exitfd,p->epfd);
    signal(SIGUSR1, exitFunc);
    return p;
}

int clientInit(char *serverIP){
    //create socket
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(sfd,-1,"socket");
    struct sockaddr_in serAddr;
    memset(&serAddr,0,sizeof(serAddr));
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(serverIP);
    serAddr.sin_port = htons(PORT);
    
    //connect
    int ret = connect(sfd,(struct sockaddr*)&serAddr,sizeof(serAddr));
    ERROR_CHECK(ret,-1,"connet");                                     
    printf("Connection to %s established\n\n",serverIP);
    
    return sfd;
}

void tcpInit(int *plistenfd, int *pepfd, processData_t **p, int processNum, struct epoll_event **pevtList){
    *plistenfd = tcpSockInit();                                     //初始化TCP连接套接字
    int ret = listen(*plistenfd, 50);                               //监听连接请求
    ERROR_CHECK(ret, -1, "listen");
    *pevtList = (struct epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
    *pepfd = epoll_create1(0);                                      //建立I/O多路复用数据结构
    ERROR_CHECK(*pepfd, -1, "epoll_create1");
    *p = (processData_t*)calloc(processNum, sizeof(processData_t));
    makeChild(*p, processNum, *pepfd);

    if(epollAddNewFd(*plistenfd,*pepfd) < 0){                       //将listenfd加入监听
        exit(EXIT_FAILURE);
    }
    
    usleep(50000);
    puts("Status: Listening...");                                   //初始化阶段完成
}                                                                                                  


int makeChild(processData_t p[], int processNum, int epfd){
    for(int i=0; i<processNum;){
        int fds[2];
        //int *fds = (int*)calloc(2,sizeof(int));
        socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);                  //为每一个子进程建立一个管道
        pid_t pid = fork();                                         //开辟子进程
        if(pid == -1 && EINTR != errno){
            perror("fork");
        } 
        if(0 == pid) {
            close(fds[1]);
            //printf("Child process %d is ready.\n",getpid());
            handleEvent(fds[0]);                                    //子进程处理函数
            close(fds[0]);
            exit(0);
        }
        //puts("parant process");
        close(fds[0]);
        p[i].pid = pid;                                             //父进程保存子进程信息
        p[i].fd = fds[1];
        p[i].status = FREE;
        epollAddNewFd(fds[1], epfd);
        ++i;
    }
    return 0;
}

int tcpSockInit(){
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); //建立套接字
    ERROR_CHECK(listenfd, -1, "socket");
    struct sockaddr_in serAddr;
    memset(&serAddr, 0, sizeof(struct sockaddr_in));
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serAddr.sin_port = htons(PORT);
    int reuse = 1;                                                  //地址复用
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_CHECK(ret, -1, "setsockopt");                             //设置属性
    ret = bind(listenfd, (struct sockaddr*)&serAddr, sizeof(struct sockaddr_in));
    ERROR_CHECK(ret, -1, "bind");
    return listenfd;
}

void exitFunc(int signum){
    printf("Received a signal: %d\n", signum);
    write(exitPipeFd[1],"1",1);
}

void handleEvent(int pipefd){
    printf("Child %d is ready.\n", getpid());
    while(1){
        char exitFlag;
        int peerfd;
        int ret = recvfd(pipefd, &peerfd, &exitFlag);                                //阻塞点，接收父进程发送的套接字文件描述符
        if(0 == ret || 'y' == exitFlag){
            printf("Child process %d exited.\n", getpid());
            exit(0);
        }
        //printf("exitFlag = %c\n", exitFlag);
        printf("\nChild process %d is busy.\n", getpid());
        //char fileName[] = "深入理解计算机系统(原书第2版)].pdf";
        commandParser(peerfd);
        //sendFile(peerfd, fileName);
        close(peerfd);
        int sig = 1;
        write(pipefd, &sig, sizeof(sig));                           //向父进程发送信号
        printf("Child process %d is free.\n\n", getpid());
    }
}

void myecho(int fd){
    char buff[1024] = {0};
    while(1){
        memset(buff, 0, sizeof(buff));
        int ret = recv(fd, buff, sizeof(buff), 0);                  //接收对端消息
        if(ret > 0){
            ret = send(fd, buff, strlen(buff), MSG_DONTWAIT);
        } else if(0 > ret){
            perror("recv");
            return;
        } else{                                                     //ret = 0，表明连接断开
            close(fd);
            return;
        }
    }
}

int epollAddNewFd(int newfd, int epfd){
    //setNonblock(newfd);
    struct epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.data.fd = newfd;
    evt.events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, newfd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    }
    return ret;
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
    int newfd = accept(listenfd, (struct sockaddr*)&cliAddr,&len);
    ERROR_CHECK(newfd, -1, "accept");
    struct in_addr cliIP = cliAddr.sin_addr;
    printf("Connection from IP: %s, PORT: %d\n", inet_ntoa(cliIP), ntohs(cliAddr.sin_port));
    return newfd;
}

void handleConnRequest(int listenfd, processData_t p[], int processNum){
    int peerfd = myaccept(listenfd);                                //先获取就绪的套接字文件描述符
    int idx;
    do{
        idx = getIndex(p, processNum, -1);                          //为该连接寻找空闲的子进程
    }while(-1 == idx);
    /*
    if(-1 == idx){
        puts("No free child program.");
        return;
    }
    */
    p[idx].status = BUSY;                                           //将子进程设置为BUSY
    sendfd(p[idx].fd, peerfd, 'n');                                      //将该文件描述符发送给子进程
    close(peerfd);
}

void handleMsg(processData_t p[], int processNum, int fd){
    int ret;
    do{
        char buff[10] = {0};
        ret = read(fd, buff, sizeof(buff));                         //接收子进程发送的信号
    }while(ret == -1 && EINTR == errno);

    int idx = getIndex(p, processNum, fd);                          //修改子进程状态为FREE
    p[idx].status = FREE;
}

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

int epollDelFd(int fd, int epfd){
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt);
    if(ret < 0){
        perror("epoll_ctl");
    }
    close(fd);
    return ret;
}

void childExit(pserManager_t p){
    if(NULL == p){
        return ;
    }
    for(int i=0; i<CHILDNUM ; ++i){
        sendfd(p->processArray[i].fd, p->processArray[i].fd, 'y');
        epollDelFd(p->processArray[i].fd, p->epfd);
    }

    for(int i=0; i<CHILDNUM; ++i){
        wait(NULL);
    }

    for(int i=0; i<CHILDNUM; ++i){
        close(p->processArray[i].fd);
    }
}

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

