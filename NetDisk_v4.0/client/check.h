#ifndef _CHECK_H_
#define _CHECK_H_

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <libgen.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <pthread.h>
#include <error.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/epoll.h>      //epoll
#include <fcntl.h>          //修改文件描述符属性
#include <sys/uio.h>

#define ARGS_CHECK(argc,num) {\
    if(argc!=num){\
        fprintf(stderr,"args error: expected %d arguments\n",num);\
        exit(1);\
    }\
}

#define ERROR_CHECK(retVal, val, msg) {\
    if(retVal==val){\
        perror(msg);\
        exit(1);\
    }\
}

#define THREAD_ERROR_CHECK(en , msg) if(en != 0) do { perror(msg); exit(EXIT_FAILURE); } while (0)
#endif

