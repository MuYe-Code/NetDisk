/*************************************************************************
    > File Name: servertest.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 22时37分33秒
 ************************************************************************/

#include "processPool.h"
#include "tcp.h"
#include "DBCreate.h"
#include "userLogIn.h"
#include "handler.h"
#define MAXEVENTS 1000

int main(int argc, char* argv[]){
    signal(SIGPIPE,SIG_IGN);
    pserManager_t p = serverInit();
    MYSQL *mysql;
    mysql = mysql_init(NULL);
    mysqlConnect(mysql);
    createUserTable(mysql);
    createFileTable(mysql);
    createFileDir();
    mysql_close(mysql);

    while(1){
        int ret = epoll_wait(p->epfd, p->pevtList, MAXEVENTS, -1);
        for(int i=0; i<ret; ++i){
            int fd = p->pevtList[i].data.fd;
            if(fd == p->listenfd){
                handleConnRequest(p->listenfd, p->processArray, p->processNum);
            } else if(fd == p->listenfd2){
                handleLongCommandRequest(p->listenfd2, p->processArray, p->processNum); 
            } else if(fd == p->exitfd){
                childExit(p);
                serDestroy(p);
                return 0;
            } else {
                handleMsg(p->processArray, p->processNum, fd, p->epfd);
            }
        }
    }
    return 0;
}
