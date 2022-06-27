/*************************************************************************
    > File Name: clienttest.c
    > Author: chenqiang
    > Function: 客户端多线程，每一个gets/puts任务使用一个线程去完成TCP连接
                、文件传输任务。父进程、子进程、和子进程创建的线程池。通过注册
                10号信号，父进程通过管道通知子线程退出

 ************************************************************************/

#include "transferMsg.h"
#include "tcp.h"
#include "cliCommand.h"
#include "clientLogIn.h"
#include "md5.h"
#include "threadPool.h"

#define THREADNUM 5

int exitpipe[2];

void sigFunc(int num){
    printf("signal %d is coming.\n", num);
    //通过管道通知子进程退出
    char ch = 1;
    write(exitpipe[1], &ch, 1);
}

int main(int argc, char* argv[]){
    ARGS_CHECK(argc, 2);

    pipe(exitpipe);//创建父子进程间退出使用管道
    pid_t pid = fork();
    if(pid > 0){
        close(exitpipe[0]);
        signal(SIGUSR1, sigFunc);
        wait(NULL);
        exit(0);
    }

    char token[TOKEN_LEN + 1] = {0};

    //子进程
    close(exitpipe[1]);
    //1.创建TCP连接
    int sockfd = clientInit(argv[1], PORT1);
    //2.进行客户端登录
    if(-1 == clientLogIn(sockfd, token)){
        return 0;
    }
    //3.创建线程池
    factory_t factory;
    factoryInit(&factory, THREADNUM);
    //4.创建select实例
    fd_set rdset;

    while(1){
        FD_ZERO(&rdset);
        FD_SET(STDIN_FILENO, &rdset);
        FD_SET(sockfd, &rdset);
        FD_SET(exitpipe[0], &rdset);
        int maxfd = sockfd > exitpipe[0] ? sockfd : exitpipe[0];
        
        select(maxfd+1, &rdset, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO, &rdset)){
            char sendBuff[1024] = {0};
            int ret = read(STDIN_FILENO, sendBuff, sizeof(sendBuff));
            int len = strlen(sendBuff);
            sendBuff[len - 1] = '\0';   //去除换行符
            //判断是否为 getfile
            char command[1024] = {0};
            strcpy(command, sendBuff);
            //printf("命令:%s\n", sendBuff);

            //解析命令
            int taskType = commandParser(command);
            if(-1 == taskType){
                printf("Invalid command! Please check!\n");
            }else if(0 == taskType || 1 == taskType){
                //printf("TaskType %d Enqueue.\n", taskType);
                ptask_t ptask = taskInit(command, taskType, token, argv[1], PORT2);
                taskEnQueue(&factory.taskQueue, ptask);
                //printf("taskQueue size: %d\n", factory.taskQueue.queueSize);
            }else if(2 == taskType){
                ret = sendn(sockfd, sendBuff, strlen(sendBuff), 0);
            }
        }else if(FD_ISSET(sockfd, &rdset)){
            char recvBuff[1024] = {0};
            int ret = recv(sockfd, recvBuff, sizeof(recvBuff), 0);
            if(0 == ret){
                break;
            }
            printf("%s", recvBuff);
        }else if(FD_ISSET(exitpipe[0], &rdset)){
            char exitflag;
            read(exitpipe[0], &exitflag, 1);
            //printf("Parent process ready to exit.\n");
            //收到父进程的退出通知后，子线程逐步退出
            taskQueueWakeup(&factory.taskQueue);
            for(int i = 0; i < THREADNUM; ++i){
                pthread_join(factory.tidAyy[i], NULL);
            }
            //printf("Thread pool exit.\n");
            factoryDestroy(&factory);
            exit(0);
        }
    }
    close(sockfd);
    return 0;
}

