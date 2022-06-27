/*************************************************************************
    > File Name: command.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 20时27分34秒
 ************************************************************************/
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "check.h"
#include "threadPool.h"

int commandExecute(ptaskQueue_t p, int sockfd, char *command, int userId, MYSQL *mysql);

void* NDcd(void*);
void* NDls(void*);
void* NDpwd(void*);
void* NDputs(void*);
void* NDgets(void*);
void* NDrm(void*);
void* NDmkdir(void*);


void printmode(mode_t mode, char modBuff[]);

#endif
