/*************************************************************************
    > File Name: mysqlCommand.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月11日 星期六 09时09分05秒
 ************************************************************************/

#include "mysqlCommand.h"
#include <limits.h>
#include <stdio.h>

#define MYSQL_HOST "localhost"
#define MYSQL_USER "root"
#define MYSQL_PASSWD "534243"
#define DB_NAME "NetDisk"
#define USER_TABLE_NAME "NDUser"
#define FILE_TABLE_NAME "VFAT"
#define DEFAULT_PWD "/"
#define DIR_LOCATION "./fileDir"
#define USERNAME_LEN 30
#define PASSWD_LEN 30
#define CRYPTPASSWD_LEN 100
#define SALT_LEN 10
#define ENCODETYPE 6

/*
获取文件的id，也可以用来检查文件是否存在
 */
int getFileID(MYSQL *mysql, int userId, char *fileName, char fileType){
    int ret;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT ID FROM %s WHERE (OWNER_ID = %d AND FILENAME = '%s' AND TYPE = '%c')", FILE_TABLE_NAME, userId, fileName, fileType);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    ret = -1;
    if(NULL != res && (row = mysql_fetch_row(res))){
        ret = atoi(row[0]);
    }
    mysql_free_result(res);
    return ret;
}

int getDirID(MYSQL *mysql, int userId, char *fileName, int parentId){
    int ret;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT ID FROM %s WHERE (OWNER_ID = %d AND FILENAME = '%s' AND PARENT_ID = %d)", FILE_TABLE_NAME, userId, fileName, parentId);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    ret = -1;
    if(NULL != res && (row = mysql_fetch_row(res))){
        ret = atoi(row[0]);
        //printf("ID = %d\n", ret);
    }
    mysql_free_result(res);
    return ret;
}


// 获取当前工作路径ID
int getPwdId(MYSQL *mysql, int userId) {
    char pwdBuf[1024] = { 0 };
    mysqlPwd(mysql, userId, pwdBuf);  // 获取用户当前工作路径 
    int len = strlen(pwdBuf);
    if (1 == len) {  // 如果是根目录返回0
        return 0;
    }
    // 正向解析
    char* temp = "/";
    char* token;
    token = strtok(pwdBuf, temp);
    int dirId = 0;
    int curId = 0;
    char query[1024];
    int ret;
    MYSQL_RES* result;
    MYSQL_ROW row;
    while (token != NULL) {  // 正向一级一级进行解析
        puts(token);
        // parser
        bzero(query, sizeof(query));
        sprintf(query, "select id from %s where PARENT_ID = %d \
                and OWNER_ID = %d and FILENAME = '%s' and TYPE = 'd'", 
                FILE_TABLE_NAME, curId, userId, token);
        ret = mysql_real_query(mysql, query, strlen(query));
        if (ret != 0) {  // 查询错误判断
            printf("mysql query error in getPwdId: %s\n", mysql_error(mysql));
            return EXIT_FAILURE;
        }
        result = mysql_store_result(mysql);
        if (result == NULL) {  // 获取结果错误判断
            printf("mysql get result error in getPwdId: %s\n", mysql_error(mysql));
            return EXIT_FAILURE;
        }
        if ((row = mysql_fetch_row(result)) != NULL) {  // 获取结果
            curId = atoi(row[0]);
        } else {  // 获取结果错误判断
            printf("get id error in getPwdId\n");
            return EXIT_FAILURE;
        }  // 取下下一个路径
        token = strtok(NULL, temp);
    }
    dirId = curId;
    return dirId;
}

/*
通用的数据库修改命令，可用于修改字符串类型的数据为newExpr
 */
int updateStrFields(MYSQL *mysql, char* tableName, char *fieldName, char *newExpr, char *condition){
    int ret;
    char query[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "UPDATE %s SET %s = '%s' WHERE (%s)", tableName, fieldName, newExpr, condition);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    //命令成功执行且确实修改了内容
    if(ret == 0 && mysql_affected_rows(mysql) > 0){
        return 0;
    }
    return -1;
}

/*
普通文件插入命令
 */
int insertFields(MYSQL *mysql, int userId, char *tableName, char *fileName, char *md5, off_t fileSize, int fileStatus){
    int parentId = getPwdId(mysql, userId);
    int ret;
    char query[1024] = {0};
    ret = sprintf(query, "INSERT INTO %s (PARENT_ID, FILENAME, OWNER_ID, MD5, FILESIZE, STATUS) VALUE (%d, '%s', %d, '%s', '%lu', %d)", 
                  FILE_TABLE_NAME, parentId, fileName, userId, md5, fileSize, fileStatus);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
    }
    return 0 == ret? 0:-1; 
}

int mysqlCd(MYSQL *mysql, int userId, char *pathName){
    //返回家目录
    if(NULL == pathName){
        char tableName[] = USER_TABLE_NAME;
        char fieldName[] = "PWD";
        char pwd[] = DEFAULT_PWD;
        char condition[1024] = {0};
        sprintf(condition, "ID = %d", userId);
        return updateStrFields(mysql, tableName, fieldName, pwd, condition);
    }
    char *fullPath = NULL;
    char pwd[1024] = {0};
    int size = strlen(pathName);
    //若末尾存在'/'则去掉
    if(size > 1 && '/' == pathName[size - 1]){
        pathName[size - 1] = '\0';
    }
    if('/' == pathName[0]){
        fullPath = pathName;
    } else {
        mysqlPwd(mysql, userId, pwd);
        fullPath = pwd;
        char *dir = strtok(pathName, "/");
        do{
            //printf("fullPath: %s\n", fullPath);
            //printf("dir: %s\n", dir);
            //.的情况
            if(0 == strcmp(".", dir)){
                continue;
            //..的情况
            } else if(0 == strcmp("..", dir)){
                //根目录无法返回上级
                if(0 == strcmp("/", fullPath)){
                    continue;
                //非根目录则去掉最后一级
                } else {
                    int len = strlen(fullPath);
                    for(--len; fullPath[len] != '/'; --len);
                    //若去掉最后一级为根目录，则保留为根目录
                    if(0 == len){
                        fullPath[1] = '\0';
                    //否则直接去掉最后一级
                    } else {
                        fullPath[len] = '\0';
                    }
                }
            //不再是.和..的目录
            } else {
                if('\0' != fullPath[1]){
                    sprintf(fullPath, "%s/%s", fullPath, dir);
                } else {
                    sprintf(fullPath, "%s%s", fullPath, dir);
                }
                break;
            }
        }while(dir = strtok(NULL, "/"));
    }

    char path[1024] = {0};
    strcpy(path, fullPath);
    //printf("path: %s\n", path);

    int ret = checkPath(mysql, userId, path);
    if(0 == ret){
        char condition[100] = {0};
        char field[] = "PWD";
        char tableName[] = USER_TABLE_NAME;
        sprintf(condition, "ID = %d", userId);
        updateStrFields(mysql, tableName, field, fullPath, condition);
        return 0;
    }
    return -1;
}

/*
检查绝对路径是否正确
 */
int checkPath(MYSQL *mysql, int userId, char *path){
    //char *fullPath = path;
    if(0 == strcmp("/", path)){
        return 0;
    } else{
        ++path;
    }

    //printf("fullPath in checkPath: %s\n", path);
    int parentId = 0;
    char *dir = strtok(path, "/");
    while(NULL != dir){
        printf("dir: %s\n", dir);
        parentId = getDirID(mysql, userId, dir, parentId);
        if(0 >= parentId){
            return -1;
        }
        dir = strtok(NULL, "/");
    }
    return 0;
}

/*
ls命令
 */
int mysqlLs(MYSQL *mysql, int userId, char lsBuffer[]){
    int parentId = getPwdId(mysql, userId); 
    int ret;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024] = {0};

    ret = sprintf(query, "SELECT FILENAME, FILESIZE, TYPE, STATUS, SHARE, \
                  CTIME , MTIME FROM %s WHERE (OWNER_ID = %d AND PARENT_ID = %d)", 
                  FILE_TABLE_NAME, userId, parentId);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    if(NULL == res){
        return -1;
    }

    while(row = mysql_fetch_row(res)){
        for(int i = 0; i < mysql_num_fields(res); ++i){
            ret = sprintf(lsBuffer, "%s%s\t ", lsBuffer, row[i]);
            if(0 > ret){
                mysql_free_result(res);
                return 1;
            }
        }
        lsBuffer[ret] = '\n';
    }
    mysql_free_result(res);
    return ret;
}

/*
获取当前工作路径，通过字符数组pwd接收
 */
int mysqlPwd(MYSQL *mysql, int userId, char *pwd){
    int ret;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT PWD FROM %s WHERE (ID = %d)", USER_TABLE_NAME, userId);
    ret = mysql_real_query(mysql, query, ret);
    printf("Command: %s\n", query);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    if(NULL != res && (row = mysql_fetch_row(res))){
        strcpy(pwd, row[0]);
    }
    mysql_free_result(res);
    return '\0' == pwd[0]? -1:0;
}

/*
命令mkdir
 */
int mysqlMkdir(MYSQL *mysql, int userId, char *dirName){
    int parentId = getPwdId(mysql, userId);
    int ret;
    char query[1024] = {0};
    ret = sprintf(query, "INSERT INTO %s (PARENT_ID, FILENAME, OWNER_ID, TYPE) VALUE (%d, '%s', %d, 'd')", FILE_TABLE_NAME, parentId, dirName, userId);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
    }
    return 0 == ret? 0:-1; 
}

//根据文件名删除虚拟文件表里的文件项
int mysqlRM(MYSQL *mysql, int userId, char *fileName)
{
	int ret;
	int parentId = getPwdId(mysql, userId);
	//-1表示文件在当前用户虚拟目录不存在，0表示存在且删除成功，1表示存在但删除失败
	int flag = 0;
	char query[1024] = { 0 };
	MYSQL_RES *res;
	MYSQL_ROW row;
	//判断当前文件目录项是否有该文件
	ret = sprintf(query, "SELECT ID FROM %s WHERE (PARENT_ID=%d AND OWNER_ID=%d AND FILENAME = '%s')", FILE_TABLE_NAME, parentId, userId, fileName);
	ret = mysql_real_query(mysql, query, ret);
	if (0 != ret) {
		printf("Failed: %s\n", mysql_error(mysql));
	}
	res = mysql_store_result(mysql);
	if (res == NULL)
	{
		mysql_free_result(res);
		flag = -1;
		return flag;
	}
	memset(query, 0, sizeof(query));
	//删除文件表里的文件项
	row = mysql_fetch_row(res);
    
	ret=sprintf(query, "DELETE FROM %s WHERE ID = %d", FILE_TABLE_NAME, atoi(row[0]));
    puts(query);
	ret = mysql_real_query(mysql, query, ret);
	if (0 != ret) {
		printf("Failed: %s\n", mysql_error(mysql));
		flag = 1;
		return flag;
	}
	mysql_free_result(res);
	return flag;
}

/*
陈强----用于puts命令
*/

//1. 根据md5值获取文件ID(查询整个虚拟文件表)  
//返回值： -1：未找到   ret：ID
int getFileIDbyMD5(MYSQL *mysql, char *md5){
    int ret;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT ID FROM %s WHERE (MD5 = '%s' AND TYPE = 'f')", FILE_TABLE_NAME, md5);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    ret = -1;
    if(NULL != res && (row = mysql_fetch_row(res))){
        ret = atoi(row[0]);
    }
    mysql_free_result(res);
    return ret;
}


/*
2.根据md5值获取对应文件的大小，若文件不存在则返回0, fileStatus可用于接收文件状态 
 */
off_t getFileSizebyMD5(MYSQL *mysql, char *md5, int *fileStatus){
    int ret;
    off_t size = 0;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT FILESIZE, STATUS FROM %s WHERE (MD5 = '%s' AND TYPE = 'f')", FILE_TABLE_NAME, md5);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    if(NULL != res && (row = mysql_fetch_row(res))){
        size = strtoul(row[0], NULL, 10);
        if(NULL != fileStatus){
            *fileStatus = atoi(row[1]);
        }
    }
    mysql_free_result(res);
    return size;
}

//3.根据文件ID（非目录）获取md5值（用于服务端根据md5找到存储的文件 发送给客户端，即gets命令）
int getMD5byFileID(MYSQL *mysql, int fileID, char *md5){
    int ret;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT MD5 FROM %s WHERE (ID = %d)", FILE_TABLE_NAME, fileID);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    if(NULL != res && (row = mysql_fetch_row(res))){
        strcpy(md5, row[0]);
    }
    mysql_free_result(res);
    return '\0' == md5[0]? -1:0;
}

int getFileOwnerIDbyID(MYSQL *mysql, int fileid){
    int ret;
    char query[1024] = {0};
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "SELECT OWNER_ID FROM %s WHERE (ID = %d)", FILE_TABLE_NAME, fileid);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);
    ret = -1;
    if(NULL != res && (row = mysql_fetch_row(res))){
        ret = atoi(row[0]);
    }
    mysql_free_result(res);
    return ret;
}

//用于修改字符类型的表项， condition是约束条件

int updatefilesize(MYSQL *mysql, char* tableName, char *fieldName, off_t filesize, char *condition){
    int ret;
    char query[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "UPDATE %s SET %s = %ld WHERE (%s)", tableName, fieldName, filesize, condition);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    //命令成功执行且确实修改了内容
    if(ret == 0 && mysql_affected_rows(mysql) > 0){
        return 0;
    }
    return -1;
}

int updatefilestatus(MYSQL *mysql, char* tableName, char *fieldName, int filestatus, char *condition){
    int ret;
    char query[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;

    ret = sprintf(query, "UPDATE %s SET %s = %d WHERE (%s)", tableName, fieldName, filestatus, condition);
    printf("Command: %s\n", query);
    ret = mysql_real_query(mysql, query, ret);
    if(0 != ret){
        printf("Failed: %s\n", mysql_error(mysql));
        return -1;
    }
    //命令成功执行且确实修改了内容
    if(ret == 0 && mysql_affected_rows(mysql) > 0){
        return 0;
    }
    return -1;
}
