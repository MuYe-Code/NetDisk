#include "clientLogIn.h"
#include "transferMsg.h"
#include "check.h"

#define SALT_LEN 10

int clientLogIn(int sockfd, char token[]){
    int ret;

    char logType[] = "1. 登录\n2. 注册\n3. 退出\n";
    
    do {
        char flag;
        char select[10] = {0};
        char userName[30] = {0};
        char password[30] = {0};

        printf("%s", logType);
        ret = scanf("%s", select);
        fflush(stdin);

        if(0 >= ret){
            close(sockfd);
            return -1;
        }

        if('\n' == select[ret - 1]){
            select[ret - 1] = '\0';
        }

        if(0 == strcmp(select, "1") || 0 == strcmp(select, "2")){
            puts("请输入账户名：");
            ret = read(STDIN_FILENO, userName, sizeof(userName));
            userName[ret - 1] = '\0';

            puts("请输入账户密码：");
            ret = read(STDIN_FILENO, password, sizeof(password));
            password[ret - 1] = '\0';
            fflush(stdin);

            char sendBuff[60] = {0};
            sprintf(sendBuff, "%s %s", userName, password);
            send(sockfd, select, 1, 0);
            sendn(sockfd, sendBuff, strlen(sendBuff), 0);
            ret = select[0] == '1'? logByPassword(sockfd, password):logByNewAcoount(sockfd);
            if(0 == ret){
                ret = recvn(sockfd, token, TOKEN_LEN, 0);
                return 0;
            } else if(-2 == ret){
                return -1;
            }
        } else if(0 == strcmp(select, "3")){
            close(sockfd);
            return -1;
        //无效输入
        } else { puts("无效输入，请重新选择。"); }
    }while(1);

    return 0;
}

int logByPassword(int sockfd, char *password){
    int ret;
    char flag;
    ret = recv(sockfd, &flag, 1, 0);
    if(ret < 0){
        return -1;
    } else if(0 == ret){
        return -2;
    }

    if('y' == flag){
        printf("登陆成功。\n");
        return 0;
    } else if('n' == flag){
        printf("账户不存在或密码错误。\n");
    } else {
        printf("连接错误，请重试。\n");
    }

    return -1;
}

int logByNewAcoount(int sockfd){
    char flag;
    int ret = recv(sockfd, &flag, 1, 0);
    if(ret < 0){
        return -1;
    } else if(0 == ret){
        return -2;
    }

    if('y' == flag){
        printf("注册成功，已登陆。\n");
        return 0;
    } else if('n' == flag){
        printf("账户名重复，清重新输入。\n");
    } else {
        printf("连接错误，请重试。\n");
    }

    return -1;
}
