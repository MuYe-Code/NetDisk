/*************************************************************************
    > File Name: servertest.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 22时37分33秒
 ************************************************************************/

//#include "command.h"
#include "processPool.h"

int main(int argc, char* argv[]){
    signal(SIGPIPE,SIG_IGN);
    pserManager_t p = serverInit();
    while(1){
        int ret = epoll_wait(p->epfd, p->pevtList, MAXEVENTS, -1);
        for(int i=0; i<ret; ++i){
            int fd = p->pevtList[i].data.fd;
            if(fd == p->listenfd){
                //int peerfd = myaccept(p->listenfd);
                //commandParser(peerfd);
                handleConnRequest(p->listenfd, p->processArray, p->processNum);
            } else if(fd == p->exitfd){
                childExit(p);
                serDestroy(p);
                return 0;
            }
        }
    }
    return 0;
}
