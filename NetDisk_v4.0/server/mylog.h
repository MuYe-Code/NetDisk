/**************************************************
 
//File Name: mylog.h
//Author: HZ_Dijkstra
//Created Time: 2022-06-10  星期五  20:37:46
 
**************************************************/
 
#ifndef __mylog_H__
#define __mylog_H__
#include"check.h"
#include"threadPool.h"
//获取时间信息
struct tm *GetTime(void);
int logInit(int fd);
//记录日志
int addlog(parguments_t p,char* command);


#endif
