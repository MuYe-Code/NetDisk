/*************************************************************************
    > File Name: userLogIn.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月10日 星期五 10时36分19秒
 ************************************************************************/

#include "check.h"

#define TOKEN_LEN 10

int logIn(int sockfd, MYSQL *mysql);
int userLogIn(int sockfd, MYSQL *mysql, char *userName, char *password);
int userRegister(int sockfd, MYSQL *mysql, char *userName, char *password);
void genRandomString(char *salt);
void genToken(char token[], int id);
int mysqlUpdateToken(MYSQL *mysql, int userId, char token[]);
int checkToken(MYSQL *mysql, char token[]);
int mysqlConnect(MYSQL *mysql);
int mysqlGetSaltByName(MYSQL *mysql, char *userName, char *ret);
int mysqlCheckPassword(MYSQL *mysql, char *userName, char *cryptPassword);
int getUserId(MYSQL *mysql, char *userName);
