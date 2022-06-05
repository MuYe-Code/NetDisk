/*************************************************************************
    > File Name: clienttest.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 22时49分41秒
 ************************************************************************/

#include "processPool.h"
#include "transferMsg.h"

int main(int argc, char* argv[]){
    ARGS_CHECK(argc, 2);
    int sockfd = clientInit(argv[1]);
    fd_set rdset;
    while(1){
        FD_ZERO(&rdset);
        FD_SET(STDIN_FILENO, &rdset);
        FD_SET(sockfd, &rdset);
        
        select(sockfd+1, &rdset, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO, &rdset)){
            char sendBuff[1024] = {0};
            int ret = read(STDIN_FILENO, sendBuff, sizeof(sendBuff));
            ret = sendn(sockfd, sendBuff, strlen(sendBuff), 0);
        }

        if(FD_ISSET(sockfd, &rdset)){
            char readBuff[1024] = {0};
            int ret = recv(sockfd, readBuff, sizeof(readBuff), 0);
            if(0 == ret){
                break;
            }
            printf("%s", readBuff);
        }
    }
    close(sockfd);
    return 0;
}
