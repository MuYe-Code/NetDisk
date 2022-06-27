#include "cliCommand.h"


//长命令和短命令的字符串数组
char *shortCommand[] = {"cd", "pwd", "ls", "rm", "mkdir"};

char *longCommand[] = {"gets", "puts"};

//客户端命令解析，返回值-1: 命令非法；0：gets; 1：puts; 2：短命令
int commandParser(char *command){
    char commandstr[1024] = {0};
    strcpy(commandstr, command);

    char delim[2] = " ";
    char *comhead = strtok(commandstr, delim);

    int len = sizeof(shortCommand)/sizeof(shortCommand[0]);
    for(int i = 0; i < len; ++i){
        if(0 == strcmp(comhead, shortCommand[i])){
            return 2;
        }
    }
    len = sizeof(longCommand)/sizeof(longCommand[0]);
    for(int i = 0; i < len; ++i){
        if(0 == strcmp(comhead, longCommand[i])){
            return i;
        }
    }
    //非法命令
    return -1;
}


//客户端gets、puts对应处理函数
void CLigets(char *command, int sockfd){
    char commandstr[1024] = {0};
    strcpy(commandstr, command);
    char delim[2] = " ";

    char *comhead = strtok(commandstr, delim);
    char *filename = strtok(NULL, delim);

    off_t offsize;  //记录本地文件的偏移量，不存在则为0
    int fd = open(filename, O_RDONLY);
    if(-1 == fd){
        offsize = 0;
        //printf("No such file %s in local directory\n", filename);
    }else{
        struct stat st;
        fstat(fd, &st);
        offsize = st.st_size;
        //printf("Have file %s in local directory, filesize: %ld bytes\n", filename, st.st_size);
    }

    //发送command+filesize给服务器
    char sendBuff[1024] = {0};
    sprintf(sendBuff, "%s%s%ld", command, " ", offsize);
    int ret = sendn(sockfd, sendBuff, strlen(sendBuff), 0);

    //接收服务器反馈，服务器是否存在该文件
    int isExist;
    ret = recv(sockfd, &isExist, 4, MSG_WAITALL);
    //printf("recv isExist: %d from server, ret = %d\n", isExist, ret);
    if(0 == isExist){
        printf("No such file %s in the server directory!\n", filename);
    }else{
        recvFile(sockfd);
    }
}

void CLiputs(char *command, int sockfd){
    char commandstr[1024] = {0};
    strcpy(commandstr, command);
    char delim[2] = " ";

    char *comhead = strtok(commandstr, delim);
    char *filename = strtok(NULL, delim);

    int fd = open(filename, O_RDONLY);
    if(-1 == fd){
        printf("No such file %s in local directory\n", filename);
    }else{  //本地存在
        struct stat st;
        fstat(fd, &st);
        off_t filesize  = st.st_size;
        //printf("Have file %s in local directory, filesize: %ld bytes\n", filename, st.st_size);
        
        //计算文件的md5值
        char md5_str[MD5_STR_LEN + 1] = {0};
        Compute_file_md5(filename, md5_str);
        
        //发送command+file_md5+filesize给服务器
        char sendBuff[1024] = {0};
        sprintf(sendBuff, "%s%s%s%s%ld", command, " ", md5_str, " ", filesize);
        int ret = sendn(sockfd, sendBuff, strlen(sendBuff), 0);

        //接收服务器反馈，服务器是否存在该文件,存在的话是否完整。只有isComplete=1才无需发送
        int isComplete;
        ret = recv(sockfd, &isComplete, 4, MSG_WAITALL);
        //printf("recv isComplete: %d from server, ret = %d\n", isComplete, ret);
        if(1 == isComplete){    //无需发送
            //printf("File %s already in the server directory, puts task cancel.\n", filename);
        }else{
            off_t offsize;
            ret = recv(sockfd, &offsize, sizeof(off_t), MSG_WAITALL);
            //printf("recv offsize: %ld from server, ret = %d\n", offsize, ret);
            sendFile(sockfd, filename, offsize);
        }
    }
}


