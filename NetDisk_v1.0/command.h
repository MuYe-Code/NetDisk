/*************************************************************************
    > File Name: command.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 20时27分34秒
 ************************************************************************/
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <check.h>

void commandParser(int sockfd);
int commandExecute(int sockfd, char *command);

int NDcd(int sockfd, char *pathname);
int NDls(int sockfd, char*);
int NDpwd(int sockfd, char*);
int NDputs(int sockfd, char *filename);
int NDgets(int sockfd, char *filename);
int NDrm(int sockfd, char *filename);
int NDmkdir(int sockfd, char *dirname);


void printmode(mode_t mode, char modBuff[]);

#endif
