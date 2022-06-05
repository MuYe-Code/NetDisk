/*************************************************************************
    > File Name: command.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 20时34分25秒
 ************************************************************************/

#include "command.h"
#include "transferMsg.h"

#define COMMANDLEN 100

//采用函数指针数组，方便后续拓展
char *commandType[] = {"cd", "ls", "pwd", "puts", "gets", "rm", "mkdir"};
int (*pFunc[])(int, char*) = {NDcd, NDls, NDpwd, NDputs, NDgets, NDrm, NDmkdir};

void commandParser(int sockfd){
    char commandBuff[COMMANDLEN] = {0};
    printf("Waiting...\n");
    while(1){
        int ret = recv(sockfd, commandBuff, COMMANDLEN, 0);
        printf("ret = %d, command = %s\n", ret, commandBuff);
        if(-1 == ret){

        } else if(0 == ret){
            break;
        } else {
            // '\n' --> '\0'
            if('\n' == commandBuff[ret - 1]){
                commandBuff[ret - 1] = '\0';
            }
            int idx = commandExecute(sockfd, commandBuff);
        }
    }
}

int commandExecute(int sockfd, char *command){
    char *argv = NULL;
    int idx = 0;
    //获取第一个参数
    for(; command[idx] != ' ' && command[idx] != '\0'; ++idx);
    if(' ' == command[idx]){
        command[idx++] = '\0';
        for(; ' ' == command[idx]; ++idx);
    }
    //获取第二个参数
    if('\0' != command[idx]){
        argv = command + idx;
        for(; command[idx] != ' ' && command[idx] != '\0'; ++idx);
        //忽略后续
        command[idx] = '\0';
    }
    
    int len = sizeof(commandType);
    for(int i=0; i<len; ++i){
        if(strcmp(command, commandType[i]) == 0){
            pFunc[i](sockfd, argv);
            break;
        }
    }
    
    return 0;
}

int NDcd(int sockfd, char *pathname){
    printf("function cd\n");
    int ret = chdir(pathname);
    if(-1 == ret){
        char sendbuff[1000] = {0};
        sprintf(sendbuff, "Faild, %s is not a directionary.\n", pathname);
        sendn(sockfd, sendbuff, strlen(sendbuff), MSG_NOSIGNAL);
    }
    return 0;
}

void printmode(mode_t mode, char modBuff[]){
     switch (mode & S_IFMT) {
        case S_IFBLK:  modBuff[0] = 'b';    break;
        case S_IFCHR:  modBuff[0] = 'c';    break;
        case S_IFDIR:  modBuff[0] = 'd';    break;
        case S_IFIFO:  modBuff[0] = 'p';    break;
        case S_IFLNK:  modBuff[0] = 'l';    break;
        case S_IFREG:  modBuff[0] = '-';    break;
        case S_IFSOCK: modBuff[0] = 's';    break;
        default:       modBuff[0] = '*';    break;
    }
    int ugo[3]={0};
    for(int i=0;i<3;i++){
        ugo[2-i]=mode&07;
        mode>>=3;
    }
    for(int i=0;i<3;i++){
        switch(ugo[i]){
        case 0: strcat(modBuff,"---");    break;
        case 1: strcat(modBuff,"--x");    break;
        case 2: strcat(modBuff,"-w-");    break;
        case 3: strcat(modBuff,"-wx");    break;
        case 4: strcat(modBuff,"r--");    break;
        case 5: strcat(modBuff,"r-x");    break;
        case 6: strcat(modBuff,"rw-");    break;
        case 7: strcat(modBuff,"rwx");    break;
        default:strcat(modBuff,"***");    break;
        }
    }
}

int NDls(int sockfd, char *dirname){
    printf("function ls\n");
    DIR* pdir;
    if(NULL == dirname){
        pdir = opendir(".");
        if(NULL == pdir){
            perror("opendir");
            return 0;
        }
    } else{
        pdir = opendir(dirname);
        if(NULL == pdir){
            perror("opendir");
            return 0;
        }
        chdir(dirname);
    }

    struct dirent* pdirent;
    while((pdirent=readdir(pdir))!=NULL){
        struct stat statbuf;
        struct tm *tmbuf;
        int retVal=stat(pdirent->d_name,&statbuf);
        //ERROR_CHECK(retVal,-1,"stat");
        tmbuf=localtime(&statbuf.st_mtime);
        //ERROR_CHECK(tmbuf,NULL,"localtime");
        char lsBuff[BUFFLEN] = {0};
        printmode(statbuf.st_mode, lsBuff);
        sprintf(lsBuff + strlen(lsBuff), " %3ld %8s %8s %10ld %2d月  %2d %2d:%2d  %-s\n",statbuf.st_nlink,getpwuid(statbuf.st_uid)->pw_name,getgrgid(statbuf.st_gid)->gr_name,
                                        statbuf.st_size,tmbuf->tm_mon,tmbuf->tm_mday,tmbuf->tm_hour,tmbuf->tm_min,pdirent->d_name);
        sendn(sockfd, lsBuff, strlen(lsBuff), MSG_NOSIGNAL);
    }
    closedir(pdir);
    return 0;
}

int NDpwd(int sockfd, char *dirname){
    printf("function pwd\n");
    return 0;
}

int NDputs(int sockfd, char *filename){
    printf("function puts\n");
    return 0;
}

int NDgets(int sockfd, char *filename){
    printf("function gets\n");
    return 0;
}
int NDrm(int sockfd, char *filename){
    printf("function rm\n");
    return 0;
}
int NDmkdir(int sockfd, char *dirname){
    printf("function mkdir\n");
    return 0;
}
