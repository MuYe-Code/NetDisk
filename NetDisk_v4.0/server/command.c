/*************************************************************************
    > File Name: command.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月04日 星期六 20时34分25秒
 ************************************************************************/

#include "command.h"
#include "transferMsg.h"
#include "threadPool.h"
#include "userLogIn.h"
#include "mysqlCommand.h"
#include "channel.h"

#define COMMANDLEN 100
#define TOKEN_LEN 10

extern channelarray_t channelArray;

//采用函数指针数组，方便后续拓展
//长命令和短命令分离
char *shortCommand[] = {"cd", "pwd", "ls", "rm", "mkdir"};
void* (*pshortFunc[])(void*) = {NDcd, NDpwd, NDls, NDrm, NDmkdir};

char *longCommand[] = {"puts", "gets"};
void* (*plongFunc[])(void*) = {NDputs, NDgets};


int commandExecute(ptaskQueue_t p, int sockfd, char *command, int userId, MYSQL *mysql){
    char *argv = NULL;
    int idx = 0;
    //获取第一个参数
    for(; command[idx] != ' ' && command[idx] != '\0'; ++idx);
    if(' ' == command[idx]){
        command[idx++] = '\0';
        for(; ' ' == command[idx]; ++idx);
    }
    //获取第二个参数
    /*
    --> 06/09 白肖
        由于指针argv指向了commandBuff的某个位置，因此argv不可以释放
        又由于commandBuff可能在argv使用之前被重置，因此需要开辟新空间存放argv
     */
    if('\0' != command[idx]){
        argv = (char*)calloc(1,COMMANDLEN);
        strcpy(argv, command + idx);
    } else argv = NULL;

    //构建参数结构体
    parguments_t arguments = (parguments_t)malloc(sizeof(arguments_t));
    arguments->sockfd = sockfd;
    arguments->name = argv;
    arguments->mysql = mysql;
    arguments->userId = userId;

    //printf("epfd = %d\n", channelArray.epfd);
    int len = sizeof(shortCommand)/sizeof(shortCommand[0]);
    for(int i=0; i<len; ++i){
        if(strcmp(command, shortCommand[i]) == 0){
            pshortFunc[i](arguments);
            return 0;
        }
    }

    len = sizeof(longCommand)/sizeof(longCommand[0]);
    for(int i=0; i<len; ++i){
        if(strcmp(command, longCommand[i])== 0){
            /* --> 06/09 白肖 取消长短命令分离
            ptask_t ptask = (ptask_t)malloc(sizeof(task_t));
            ptask->taskType = THREADNORMAL;
            ptask->taskFunc = plongFunc[i];
            ptask->funcArgv = arguments;
            ptask->next = NULL;
            taskEnQueue(p, ptask);
            */
            plongFunc[i](arguments);
            return 0;
        }
    }
    //陈强  先注释  会引起gets解析文件错误
    //sendn(sockfd, "Invalid command!\n", 17, MSG_NOSIGNAL);
    return 0;
}

void* NDcd(void* argv){
    printf("function cd\n");
    parguments_t arguments = (parguments_t)argv;
    char *pathname = arguments->name;
    int sockfd = arguments->sockfd;
    MYSQL *mysql = arguments->mysql;
    int userId = arguments->userId;
    int ret = mysqlCd(mysql, userId, pathname);
    if(-1 == ret){
        char sendbuff[1000] = {0};
        sprintf(sendbuff, "Faild, %s is not a directionary.\n", pathname);
        sendToChannel(&channelArray, sockfd, channelArray.epfd, sendbuff, strlen(sendbuff));
        //sendn(sockfd, sendbuff, strlen(sendbuff), MSG_NOSIGNAL);
    }
    return 0;
}

//注意释放参数结构体内存空间
void* NDls(void *argv){
    printf("function ls\n");
    parguments_t arguments = (parguments_t)argv;
    char *dirname = arguments->name;
    int sockfd = arguments->sockfd;
    MYSQL *mysql = arguments->mysql;
    int userId = arguments->userId;

    //确保路径合法
    if(NULL != dirname){
        for(int i = 0; dirname[i] != '\0'; ++i){
            if(' ' == dirname[i]){
                dirname[i] = '\0';
                break;
            }
        }
    }
    
    char lsBuffer[4096] = {0};
    int len = mysqlLs(mysql, userId, lsBuffer);
    lsBuffer[sizeof(lsBuffer) - 1] = '\0';
    lsBuffer[sizeof(lsBuffer) - 2] = '\n';
    if(-1 == len){
        char sendBuffer[] = "Error.\n";
        sendToChannel(&channelArray, sockfd, channelArray.epfd, sendBuffer, strlen(sendBuffer));
        //sendn(sockfd, sendBuffer, strlen(sendBuffer), MSG_NOSIGNAL);
    } else if(0 == len){
        char sendBuffer[] = "Empty directory.\n";
        sendToChannel(&channelArray, sockfd, channelArray.epfd, sendBuffer, strlen(sendBuffer));
        //sendn(sockfd, sendBuffer, strlen(sendBuffer), MSG_NOSIGNAL);
    } else {
        char sendBuffer[4096] = "name\t size\t type\t status\t share\t ctime\t\t\t mtime\n";
        strcat(sendBuffer, lsBuffer);
        sendToChannel(&channelArray, sockfd, channelArray.epfd, sendBuffer, strlen(sendBuffer));
        
        //sendn(sockfd, sendBuffer, strlen(sendBuffer), MSG_NOSIGNAL);
        //sendn(sockfd, lsBuffer, strlen(lsBuffer), MSG_NOSIGNAL);
    }
    free(argv);
    return 0;
}

// 杨金华 pwd
void* NDpwd(void *argv){
    printf("function pwd\n");
    parguments_t arguments = (parguments_t)argv;
    int sockfd = arguments->sockfd;
    if (argv == 0) {
        return 0;
    }

    int userId = arguments->userId;
    MYSQL *mysql = arguments->mysql;

    char pwdBuf[1024] = { 0 };
    mysqlPwd(mysql, userId, pwdBuf);
    strcat(pwdBuf, "\n");
    sendn(sockfd, pwdBuf, strlen(pwdBuf), MSG_NOSIGNAL);
    free(argv);
    return 0;
}

//陈强  文件上传下载  兼任断点重传
void* NDputs(void *argv){
    printf("function puts\n");
    parguments_t arguments = (parguments_t)argv;
    char *p = arguments->name;
    int sockfd = arguments->sockfd;
    MYSQL *mysql = arguments->mysql;
    int userId = arguments->userId;


    char *filename, *md5, *fileSize;
    const char s[2] = " ";

    if(NULL == p){
        return 0;
    }

    filename = strtok(p, s);
    md5 = strtok(NULL, s);
    fileSize = strtok(NULL, s);
    off_t filesize = atoi(fileSize);

    //通过客户端发送的md5查表，如果能查到fileID，表示无需上传
    int ret = getFileIDbyMD5(mysql, md5);
    int complete = 0; //文件是否完整
    int fileStatus; //可用于接收文件状态
    int file_owner_id = 0;
    int fileID;
    off_t offsize;

    if(-1 == ret){
        printf("No such file %s in file table\n", filename);
        offsize = 0;
    }else{ //找到md5对应fileID，分为两种情况：1.文件完整 2.文件不完整 需要发送偏移量
        off_t size = getFileSizebyMD5(mysql, md5, &fileStatus);  
        file_owner_id = getFileOwnerIDbyID(mysql, ret);
        fileID = ret;
        if(filesize == size){
            complete = 1;
            fileStatus = 1;
            //更新一条记录
            if(file_owner_id != userId){
                insertFields(mysql, userId, "VFAT", filename, md5, filesize, fileStatus);
            }
        }else{
            offsize = size;
        }
    }

    if(1 == complete){
        ret = send(sockfd, &complete, 4, 0);
        printf("send complete %d, ret = %d\n", complete, ret);
    }else { //文件不完整或不存在，需要接收客户端数据

        ret = send(sockfd, &complete, 4, 0);
        printf("send complete %d, ret = %d\n", complete, ret);

        ret = send(sockfd, &offsize, sizeof(off_t), 0);

        //接收文件
        recvFile(sockfd, md5);

        //为了防止传输中断，需要判断下文件size再去插表
        char filepath[200] = {0};
        sprintf(filepath, "%s%s%s", DEFAULT_DIR, "/", md5);
        int fileFd = open(filepath, O_RDONLY);

        struct stat st;
        fstat(fileFd, &st);
        printf("have recvfile %s, filesize: %ld\n", filename, st.st_size);
        
        printf("userId = %d, ownerId = %d\n", userId, file_owner_id);
        if(file_owner_id != userId){
            if(filesize == st.st_size){
                fileStatus = 1;
                insertFields(mysql, userId, "VFAT", filename, md5, filesize, fileStatus);
            }
            else{
                fileStatus = 0;
                insertFields(mysql, userId, "VFAT", filename, md5, st.st_size, fileStatus);
            }
        }else if(file_owner_id == userId){
             if(filesize == st.st_size){
                fileStatus = 1;
                char condition[1024] = {0};
                sprintf(condition, "ID = %d", fileID);
                updatefilesize(mysql, "VFAT", "FILESIZE" , st.st_size, condition);
                updatefilestatus(mysql, "VFAT", "STATUS" , fileStatus, condition);
            }else{
                fileStatus = 0;
                char condition[1024] = {0};
                sprintf(condition, "ID = %d", fileID);
                updatefilesize(mysql, "VFAT", "FILESIZE" , st.st_size, condition);
                updatefilestatus(mysql, "VFAT", "STATUS" , fileStatus, condition);
            }
        }

        close(fileFd);
    }

    free(argv);
    return 0;

}

void* NDgets(void *argv){
    printf("function gets\n");
    parguments_t arguments = (parguments_t)argv;
    char *p = arguments->name;
    int sockfd = arguments->sockfd;
    MYSQL *mysql = arguments->mysql;
    int userId = arguments->userId;


    char *filename, *filesize;
    const char s[2] = " ";

    //检查传入的argv合法
    if(NULL == p){
        return 0;
    }

    filename = strtok(p, s);
    filesize = strtok(NULL, s);
    off_t offsize = atoi(filesize);

    printf(">> NDgets(): filename: %s ; offsize : %ld\n", filename, offsize);

    int isExist = 0;  //该文件名是否在表中存在
    char md5[200] = {0}; //接收表中查询到的md5字符串
    int ret = getFileID(mysql, userId, filename, 'f');
    if(-1 == ret){
        isExist = 0;
        printf("No such file %s in file table\n", filename);
    }
    else{
        isExist = 1;
        printf("Find file %s in file table\n", filename);
        ret = getMD5byFileID(mysql, ret, md5);
        printf("file %s md5: %s\n", filename, md5);
    }
    send(sockfd, &isExist, 4, 0);
    if(1 == isExist){
        sendFile(sockfd, filename, offsize, md5);
    }

    free(argv);
    return 0;

    /*
    //通过是否能够打开该文件判断文件是否存在
    int fd=open(filename,O_RDONLY);
    if(-1==fd)//文件不存在
    {
    //向客户端发送“0”,代表此文件无效
    sendn(sockfd,"0",1,MSG_NOSIGNAL);
    return 0;
    }
    else
    {
    close(fd);
    sendn(sockfd,"1",1,MSG_NOSIGNAL);//发送“1”，代表文件名存在
    usleep(5000);
    sendFile(sockfd, filename);
    }

    free(argv);
    return 0;
    */
}

//hezhou rm file
void* NDrm(void *argv) {
	printf("function rm\n");
	parguments_t arguments = (parguments_t)argv;
	char *filename = arguments->name;
	int sockfd = arguments->sockfd;
	int userId = arguments->userId;
	MYSQL *mysql = arguments->mysql;

	if (NULL == filename)
		return 0;
	char sendbuf[100] = { 0 };
	int flag;
	//通过是否能够打开该文件判断文件是否存在
	//int fd=open(filename,O_RDONLY);
	//if(-1==fd)//文件不存在
	//{
	//    sprintf(sendbuf,"File %s does not exist or not find.\n",filename);
	//    sendn(sockfd,sendbuf,strlen(sendbuf),MSG_NOSIGNAL);
	//    return 0;
	//}
	//else
	//{
	//    close(fd);
	//}
	//删除文件
	/*if(0==remove(filename))
	{
		sprintf(sendbuf,"Remove file %s  successed.\n",filename);
		sendn(sockfd,sendbuf,strlen(sendbuf),MSG_NOSIGNAL);
	}
	else
	{
		sprintf(sendbuf,"Remove file %s failed.\n",filename);
		sendn(sockfd,sendbuf,strlen(sendbuf),MSG_NOSIGNAL);
	}*/
	flag = mysqlRM(mysql, userId,filename);
	if (flag == -1)
	{
		sprintf(sendbuf, "File %s does not exist or not find.\n", filename);
		sendn(sockfd, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
		return 0;
	}
	else if (flag == 0)
	{
		sprintf(sendbuf, "Remove file %s  succeed.\n", filename);
		sendn(sockfd, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
	}
	else if (flag == 1)
	{
		sprintf(sendbuf, "Remove file %s failed.\n", filename);
		sendn(sockfd, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
	}
	else
	{
		printf("remove failed\n");
	}
	free(argv);
	return 0;

}


//陈强 mkdir
void* NDmkdir(void *argv){
    printf("function mkdir\n");
    parguments_t arguments = (parguments_t)argv;
    char *dirname = arguments->name;
    int sockfd = arguments->sockfd;
    MYSQL *mysql = arguments->mysql;
    int userId = arguments->userId;

    if(NULL == dirname){
        return 0;
    }
    //确保路径合法
    for(int i = 0; dirname[i] != '\0'; ++i){
        if(' ' == dirname[i]){
            dirname[i] = '\0';
            break;
        }
    }
    
    int ret = mysqlMkdir(mysql, userId, dirname);
    char sendbuff[1024] = {0};
    if(-1 == ret){
        sprintf(sendbuff, "Faild create a directioary named %s, please check whether is reapted or anything else.\n", dirname);
        sendToChannel(&channelArray, sockfd, channelArray.epfd, sendbuff, strlen(sendbuff));
        //sendn(sockfd, sendbuff, strlen(sendbuff), MSG_NOSIGNAL);
    }
    else{
        sprintf(sendbuff, "create directionary %s success\n", dirname);
        sendToChannel(&channelArray, sockfd, channelArray.epfd, sendbuff, strlen(sendbuff));
        //sendn(sockfd, sendbuff, strlen(sendbuff), MSG_NOSIGNAL);
    }
    free(argv);
    return 0;
}

