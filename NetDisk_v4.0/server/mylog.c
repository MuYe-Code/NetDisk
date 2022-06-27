/**************************************************
 
//File Name: mylog.c
//Author: HZ_Dijkstra
//Created Time: 2022-06-10  星期五  16:11:32
 
**************************************************/
#include"mylog.h"
int logfd;
int num;

struct tm*  GetTime(void)
{
    time_t t;
    struct tm* tt;
    time(&t);
    tt=localtime(&t);

    return tt;
}

int logInit(int fd) {
	logfd = fd;
	num = 0;
	char loadbuf[100] = { 0 };
	sprintf(loadbuf, "序号\t客户ID\t时间\t\t\t动作\n");
	write(logfd, loadbuf, strlen(loadbuf));
	return 0;
}

int addlog(parguments_t p,char *command)
{
    char logbuf[100]={0};
	char timebuf[30] = { 0 };
    char commandbuf[30];

	num++;
    //struct flock f_lock;//文件锁
    //f_lock.l_type=F_WRLCK;//选用写锁
    //f_lock.l_whence=0;
    //f_lock.l_len=0;//表示整个文件加锁
    struct tm * Time=GetTime();
	sprintf(timebuf, "%d-%d-%d %d:%d:%d", Time->tm_year+1900,Time->tm_mon+1,
		Time->tm_mday,Time->tm_hour,Time->tm_min,Time->tm_sec);
    sprintf(logbuf, "%d\t%d\t%s\t\t%s\n",num,p->userId,timebuf,command);

	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
   
	write(logfd, logbuf, strlen(logbuf));
	
   /* fcntl(ffd,F_SETLKW,&f_lock);
    write(ffd,logbuf,strlen(logbuf));
    f_lock.l_type=F_UNLCK;
    fcntl(ffd,F_SETLKW,&f_lock);*/

	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);

    return 0;
    
}
