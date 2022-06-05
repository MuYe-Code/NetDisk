/*************************************************************************
    > File Name: transferMsg.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月05日 星期日 11时20分44秒
 ************************************************************************/
#ifndef _TRANSFERMSG_H_
#define _TRANSFERMSG_H_

#include <check.h>

#define BUFFLEN 1024

typedef struct train_s{
    off_t fileSize;    
    int size;
    char buff[BUFFLEN];
}train_t,*ptrain_t;


//                                                         
int sendn(int sockfd, const void* buf, int len, int flags);
int recvn(int sockfd, const void* buf, int len, int flags);
//
int sendFile(int sockfd, char *fileName);
int recvFile(int sockfd);
//
int sendfd(int pipefd, int fd, char exitFlag);
int recvfd(int pipefd, int *pfd, char *exitFlag);

#endif
