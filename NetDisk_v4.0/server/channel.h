/*************************************************************************
    > File Name: channel.h
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月14日 星期二 08时44分02秒
 ************************************************************************/

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#define CHANNELBUFLEN 1024      //应用层缓冲区大小
#define MAXEVENTS 1000          //监听的最大连接数量
#define TIMEOUT 60

#include "check.h"

typedef struct Channel{
    int taskNumber;             //任务序号
    int fd;                     //文件描述符
    int userId;                 //用户ID
    int index;                  //用户在时间转盘的下标
    //int rdBufLen;             //读缓冲区数据长度
    int wrBufLen;               //写缓冲区数据长度
    //char rdBuf[CHANNELBUFLEN];//读缓冲区
    char wrBuf[CHANNELBUFLEN];  //写缓冲区
    time_t lastTime;            //最后活跃时间
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}channel_t,*pchannel_t;

typedef struct ChannelArray{ 
    int epfd;
    int count;                  //处于连接的文件描述符总数
    int maxFdIndex;             //最大的文件描述符在数组中的下标，减少轮询次数
    channel_t array[MAXEVENTS]; //存放通道结构的数组
    pthread_mutex_t mutex;
}channelarray_t;

//channel_t相关操作
bool channelInit(pchannel_t p);                         //通道初始化
bool channelArrayInit(channelarray_t* pchannels);       //通道数组初始化
bool channelAdd(channelarray_t* pchannels, int fd, int userId, int index);     //向数组添加通道
bool channelDel(channelarray_t* pchannels, int fd);     //删除文件描述符为fd的通道
int channelGetIndex(channelarray_t* pchannels, int fd); //获取文件描述符为fd的通道的下标
int sendToChannel(channelarray_t* pchannels, int fd, int epfd, const char *sendBuff, int len);
void epollCloseAll(channelarray_t *pchannels, int epfd);

//其他
bool closeTimeoutConnection(int epfd, channelarray_t *pchannelarray);


#endif
