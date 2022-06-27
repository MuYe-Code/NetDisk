/*************************************************************************
    > File Name: mysqlCommand.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月11日 星期六 08时58分31秒
 ************************************************************************/

#include "check.h"
#include <mysql/mysql.h>

//根据文件名获取文件ID
int getFileID(MYSQL *mysql, int userId, char *fileName, char fileType);
int getDirID(MYSQL *mysql, int userId, char *fileName, int parentId);
//用于cd ..
int pwdBackward(MYSQL *mysql, int userId);
int pwdForward(MYSQL *mysql, int userId, char* targetDir);
int getPwdId(MYSQL *mysql, int userId);
//用于修改字符类型的表项， condition是约束条件
int updateStrFields(MYSQL *mysql, char* tableName, char *fieldName, char *newExpr, char *condition);
//用于文件插入
int insertFields(MYSQL *mysql, int userId, char *tableName, char *fileName, char *md5, off_t fileSize, int fileStatus);
//cd命令
int mysqlCd(MYSQL *mysql, int userId, char *pathName);
int checkPath(MYSQL *mysql, int userId, char *path);
//ls命令
int mysqlLs(MYSQL *mysql, int userId, char lsBuffer[]);
//pwd命令
int mysqlPwd(MYSQL *mysql, int userId, char *pwd);
//mkdir命令
int mysqlMkdir(MYSQL *mysql, int userId, char *dirName);
//rm命令：根据文件名删除虚拟文件表里相关文件项
int mysqlRM(MYSQL *mysql, int userId, char *fileName);


/*
陈强----用于puts命令
*/

//1. 根据md5值获取文件ID(查询整个虚拟文件表)  
//返回值： -1：未找到   ret：ID 
int getFileIDbyMD5(MYSQL *mysql, char *md5);

//2. 根据md5获取文件大小，其中通过fileStatus接收文件状态：0-正常 1-异常
off_t getFileSizebyMD5(MYSQL *mysql, char *md5, int *fileStatus);

//3.根据文件ID（非目录）获取md5值（用于服务端根据md5找到存储的文件 发送给客户端，即gets命令）
int getMD5byFileID(MYSQL *mysql, int fileID, char *md5);

//4.根据文件ID获取文件ownerID
int getFileOwnerIDbyID(MYSQL *mysql, int fileid);

//用于修改文件表中的filesize和filestatus， condition是约束条件
int updatefilesize(MYSQL *mysql, char* tableName, char *fieldName, off_t filesize, char *condition);

int updatefilestatus(MYSQL *mysql, char* tableName, char *fieldName, int filestatus, char *condition);
