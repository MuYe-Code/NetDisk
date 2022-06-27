/*************************************************************************
    > File Name: clientLogIn.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月10日 星期五 16时40分02秒
 ************************************************************************/

#include "check.h"

#define TOKEN_LEN 10

int clientLogIn(int sockfd, char token[]);
int logByPassword(int sockfd, char *password);
int logByNewAcoount(int sockfd);
