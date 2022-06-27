/*************************************************************************
    > File Name: DBCreate.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月11日 星期六 17时14分40秒
 ************************************************************************/

#include "DBCreate.h"

#define USER_TABLE_NAME "NDUser"
#define FILE_TABLE_NAME "VFAT"
#define DEFAULT_DIR  "fileDir"

int createUserTable(MYSQL *mysql){
    int ret;
    char query[] = "CREATE TABLE IF NOT EXISTS NDUser (\
                    ID INT NOT NULL AUTO_INCREMENT,\
                    USERNAME VARCHAR(30) NOT NULL UNIQUE,\
                    SALT CHAR(11) NOT NULL,\
                    TOKEN CHAR(11),\
                    CRYPTPASSWD VARCHAR(200) NOT NULL,\
                    PWD VARCHAR(200) NOT NULL,\
                    CTIME TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
                    MTIME TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
                    PRIMARY KEY(ID)\
                    )";
    
    ret = mysql_real_query(mysql, query, strlen(query));
    if(0 != ret){
        printf("用户表创建失败。\n");
    } else {
        printf("用户表创建成功。\n");
    }
    return 0 == ret? 0:-1;
}

int createFileTable(MYSQL *mysql){
    int ret;
    char query[] = "CREATE TABLE IF NOT EXISTS VFAT (\
                    ID INT NOT NULL AUTO_INCREMENT,\
                    PARENT_ID INT  NOT NULL DEFAULT 0,\
                    FILENAME VARCHAR(30) NOT NULL,\
                    OWNER_ID INT NOT NULL,\
                    MD5 CHAR(33) NOT NULL DEFAULT 0,\
                    FILESIZE BIGINT NOT NULL DEFAULT 0,\
                    STATUS INT NOT NULL DEFAULT 0 COMMENT '0-normal, 1-abnormal',\
                    TYPE CHAR(2) NOT NULL DEFAULT 'f' COMMENT 'f-normal, d-directory',\
                    SHARE INT NOT NULL DEFAULT 0 COMMENT '0-not share, 1-share',\
                    CTIME TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
                    MTIME TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\
                    PRIMARY KEY(ID),\
                    UNIQUE KEY UNIQUE_FILE(PARENT_ID, FILENAME, OWNER_ID),\
                    FOREIGN KEY(OWNER_ID) REFERENCES NDUser(ID)\
                    )";
    
    ret = mysql_real_query(mysql, query, strlen(query));
    if(0 != ret){
        printf("虚拟文件表创建失败。\n");
    } else {
        printf("虚拟文件表创建成功。\n");
    }
    return 0 == ret? 0:-1;
}

int createFileDir(){
    int ret = access(DEFAULT_DIR, F_OK);
    if(0 == ret){
        ret = access(DEFAULT_DIR, R_OK|W_OK|X_OK);
        if(-1 == ret){
            printf("默认文件夹权限错误。\n");
            return -1;
        }
    } else {
        ret = mkdir(DEFAULT_DIR, 0777);
    }

    if(0 == ret){
        printf("默认文件夹创建成功。\n");
    }
    return ret;
}
