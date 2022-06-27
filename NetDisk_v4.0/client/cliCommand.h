#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "check.h"
#include "threadPool.h"
#include "transferMsg.h"
#include "md5.h"


//客户端命令解析，返回值-1: 命令非法；0：gets; 1：puts; 2：短命令
int commandParser(char *command);
//客户端gets、puts对应处理函数
void CLigets(char *command, int sockfd);
void CLiputs(char *command, int sockfd);

#endif
