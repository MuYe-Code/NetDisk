/*************************************************************************
    > File Name: userLogIn.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月10日 星期五 10时38分03秒
 ************************************************************************/

#include "userLogIn.h"
#include "transferMsg.h"

#define MYSQL_HOST "localhost"
#define MYSQL_USER "root"
#define MYSQL_PASSWD "123456"
#define DB_NAME "NetDisk"
#define USER_TABLE_NAME "NDUser"
#define FILE_TABLE_NAME "VFAT"
#define DEFAULT_PWD "/"
#define USERNAME_LEN 30
#define PASSWD_LEN 30
#define CRYPTPASSWD_LEN 100
#define SALT_LEN 10
#define ENCODETYPE "$6$"

/*
服务端用户登陆处理，登陆成功返回用户ID
 */
int logIn(int sockfd, MYSQL *mysql){
    int ret;
    int userId = -1;

    char recvBuff[USERNAME_LEN + PASSWD_LEN];
    char *userName;
    char *password;
    char logType;
    do {
        memset(recvBuff, 0, sizeof(recvBuff));
        ret = recv(sockfd, &logType, 1, 0);
        ret = recv(sockfd, recvBuff, sizeof(recvBuff), 0);
        if(0 >= ret){
            close(sockfd);
            return -1;
        }

        userName = strtok(recvBuff, " ");
        password = strtok(NULL, "\n");

        //已有账户登陆
        if('1' == logType){
            ret = userLogIn(sockfd, mysql, userName, password);
        //注册新账户
        } else if('2' == logType){
            ret = userRegister(sockfd, mysql, userName, password);
        } else {
            send(sockfd, "e", 1, MSG_NOSIGNAL);
            continue;
        }

        if(0 < ret){
            send(sockfd, "y", 1, MSG_NOSIGNAL);
            break;
        } else {
            send(sockfd, "n", 1, MSG_NOSIGNAL);
        }
    } while(1);

    char token[TOKEN_LEN + 1] = {0};
    genToken(token, ret);
    printf("TOKEN = %s\n", token);
    mysqlUpdateToken(mysql, ret, token);
    sendn(sockfd, token, TOKEN_LEN, MSG_NOSIGNAL);

    return ret;
}

/*
通过账户密码登陆
 */
int userLogIn(int sockfd, MYSQL *mysql, char *userName, char *password){
    int ret;
    char salt[SALT_LEN + 1] = {0};
    char *cryptPassword;
    mysqlGetSaltByName(mysql, userName, salt);
    if('\0' != salt[0]){
        cryptPassword = crypt(password, salt);
        ret = mysqlCheckPassword(mysql, userName, cryptPassword);
        if(0 == ret){
            return getUserId(mysql, userName);
        }
    }
    return -1;
}

/*
通过注册新用户登陆
 */
int userRegister(int sockfd, MYSQL *mysql, char *userName, char *password){
    int ret;
    char salt[SALT_LEN + 1] = {0};
    char *cryptPasswd;
    char head[] = "INSERT INTO ";
    char query[300] = {0};
    char field[] = "USERNAME, SALT, CRYPTPASSWD, PWD ";
    char value[] = "VALUE";
    char message[200] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    genRandomString(salt);
    cryptPasswd = crypt(password, salt);   
    sprintf(message, "'%s', '%s', '%s', '%s'", userName, salt, cryptPasswd, DEFAULT_PWD);

    ret = sprintf(query, "%s%s(%s)%s(%s)", head, USER_TABLE_NAME, field, value, message);
    //printf("command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);

    if(ret){
        printf("Insert new user falied, %s\n", mysql_error(mysql));
        return -1;
    } 
    //printf("Insert new user successfully.\n");
    return getUserId(mysql, userName);
}

/*
连接数据库
 */
int mysqlConnect(MYSQL *mysql){
    if(!mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USER, MYSQL_PASSWD, DB_NAME, 0, NULL, 0)){
        printf("Failed to connect: %s\n", mysql_error(mysql));
        return -1;
    }

    printf("Process %d connect successfully.\n", getpid());
    return 0;
}

/*
根据用户名获取盐值
 */
int mysqlGetSaltByName(MYSQL *mysql, char *userName, char *result){
    int ret;
    char *salt;
    char head[] = "SELECT SALT FROM ";
    char query[200] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "%s%s WHERE USERNAME = '%s'", head, USER_TABLE_NAME, userName);
    mysql_real_query(mysql, query, strlen(query));
    res = mysql_store_result(mysql);
    if(NULL == res){
        return -1;
    }

    while(row = mysql_fetch_row(res)){
        strcpy(result, row[0]);
    }

    mysql_free_result(res);
    return 0;
}

/*
检查生成的密文与数据库中的密文是否匹配
 */
int mysqlCheckPassword(MYSQL *mysql, char *userName, char *cryptPasswd){
    int ret;
    char head[] = "SELECT * FROM ";
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "%s%s WHERE (USERNAME = '%s' AND CRYPTPASSWD = '%s')", head, USER_TABLE_NAME, userName, cryptPasswd);
    //printf("command: %s\n", query);
    mysql_real_query(mysql, query, strlen(query));
    //puts("Query finished.");
    res = mysql_store_result(mysql);
    if(NULL != res && (row = mysql_fetch_row(res))){
        ret = 0;
    } else {
        ret = -1;
    }
    
    mysql_free_result(res);
    return ret;
}

int mysqlUpdateToken(MYSQL *mysql, int userId, char token[]){
    int ret;
    char head[] = "UPDATE";
    char query[300] = {0};
    char field[] = "TOKEN";
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "%s %s SET %s = '%s' WHERE ID = %d", head, USER_TABLE_NAME, field, token, userId);
    printf("command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);

    if(ret || 0 == mysql_affected_rows(mysql)){
        printf("Update token falied, %s\n", mysql_error(mysql));
        return -1;
    } 
    
    return 0;
}

int checkToken(MYSQL *mysql, char token[]){
    int ret;
    char head[] = "SELECT ID FROM ";
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;
    int userId = -1;

    ret = sprintf(query, "%s%s WHERE (TOKEN = '%s')", head, USER_TABLE_NAME, token);
    printf("command: %s\n", query);
    mysql_real_query(mysql, query, strlen(query));
    //puts("Query finished.");
    res = mysql_store_result(mysql);
    if(NULL != res && (row = mysql_fetch_row(res))){
        userId = atoi(row[0]);
    }
    mysql_free_result(res);
    
    return userId;
}

/*
生成盐值
 */
void genRandomString(char *salt){
    srand((unsigned)time(NULL));
    strcpy(salt, ENCODETYPE);
    for(int i = strlen(salt); i < SALT_LEN - 1; ++i){
        int flag = rand() % 3;
        switch(flag){
        case 0: salt[i] = 'A' + rand() % 26; break;
        case 1: salt[i] = 'a' + rand() % 26; break;
        case 2: salt[i] = '0' + rand() % 10; break;
        default: salt[i] = 'x'; break;
        }
    }
    salt[SALT_LEN - 1] = '$';
}

/*
生成token
 */
void genToken(char token[], int id){
    srand((unsigned)time(NULL));
    sprintf(token, "%d$", id);
    for(int i = strlen(token); i < TOKEN_LEN - 1; ++i){
        int flag = rand() % 3;
        switch(flag){
        case 0: token[i] = 'A' + rand() % 26; break;
        case 1: token[i] = 'a' + rand() % 26; break;
        case 2: token[i] = '0' + rand() % 10; break;
        default: token[i] = 'x'; break;
        }
    }
    token[TOKEN_LEN - 1] = '$';
}

/*
根据用户名获取用户ID
 */
int getUserId(MYSQL *mysql, char *userName){
    int ret;
    int userId = -1;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT ID FROM %s WHERE USERNAME = '%s'", USER_TABLE_NAME, userName);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("SELECT USER_ID failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    if(NULL != res && (row = mysql_fetch_row(res))){
        userId = atoi(row[0]);
    }
    mysql_free_result(res);
    printf("用户%s的ID为%d\n", userName, userId);
    return userId;
}
