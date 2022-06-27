/*************************************************************************
    > File Name: transferMsg.c
    > Author: masker
    > Mail: https://github.com/MuYe-Code
    > Created Time: 2022年06月05日 星期日 11时24分56秒
 ************************************************************************/

#include "transferMsg.h"

int sendn(int fd, const void* buf, int len, int flags){
    int left= len, ret;
    char* pbuf = (char*)buf;
    while(left > 0){
        ret = send(fd, pbuf, left, flags);
        if(-1 == ret && EINTR != errno){
            perror("send");
            return -1;
        }
        left -= ret;
        pbuf += ret;
    }
    return len - left;
}

int recvn(int fd, const void* buf, int len, int flags){
    int left= len, ret;
    char* pbuf = (char*)buf;
    while(left > 0){
        ret = recv(fd, pbuf, left, flags);
        if(-1 == ret && EINTR != errno){
            perror("send");
            return -1;
        }
        left -= ret;
        pbuf += ret;
    }
    return len - left;
}

/*
int sendFile(int sockfd, char *fileName){                                         
    int fileiFd = open(fileName, O_RDONLY);
    if(-1 == fileFd){
        perror("open");
        return -1;
    }

    struct stat st;
    fstat(fileFd, &st);

    train_t train;
    memset(&train, 0, sizeof(train));
    train.size = strlen(fileName);
    train.fileSize = st.st_size;
    strncpy(train.buff, fileName, train.size);
    sendn(sockfd, &train, sizeof(off_t) + sizeof(int) + train.size, MSG_NOSIGNAL);
    
    int pipefds[2];
    pipe(pipefds);

    printf("File name: %s\n", train.buff);
    printf("File size: %ld\n", train.fileSize);
    puts("Uploading...");

    off_t uploadSize = 0;

    //int percentage = 0;
    //off_t slice = train.fileSize / 100;
    //slice = slice == 0? 1:slice;
    //printf("Percent: \n");
    //printf("0.00%%");
    //fflush(stdout);
    while(uploadSize < train.fileSize){
        int ret;
        ret = splice(fileFd, NULL, pipefds[1], NULL, 4096, SPLICE_F_MORE);
        uploadSize += ret;
        splice(pipefds[0], NULL, sockfd, NULL, ret, SPLICE_F_MORE);
        
        if(uploadSize / slice > percentage){
            ++percentage;
            printf("\r%5.2lf%%",(double)uploadSize/slice);
            fflush(stdout);
        }
        *//*
        //puts("cycle");
        fflush(stdout);
    }
    close(pipefds[0]);
    close(pipefds[1]);
    //puts("\r100.00%");
    printf("Upload %s complete.\n", fileName);
    fflush(stdout);
    return 0;
}

int recvFile(int sockfd){
    train_t train;
    memset(&train, 0, sizeof(train));

    recvn(sockfd, &train.fileSize, sizeof(train.fileSize), MSG_WAITALL);
    recvn(sockfd, &train.size, sizeof(train.size), MSG_WAITALL);
    recvn(sockfd, &train.buff, train.size, MSG_WAITALL);
    
    int fileFd = open(train.buff, O_WRONLY | O_CREAT, 0666);
    ftruncate(fileFd, train.fileSize);
    int pipefds[2];
    pipe(pipefds);
    
    printf("File name: %s\n", train.buff);
    printf("File size: %ld\n", train.fileSize);
    puts("Downloading...");

    off_t downloadSize = 0;
    int percentage = 0;
    off_t slice = train.fileSize / 100;
    slice = slice == 0? 1:slice;
    printf("Percent: \n");
    printf("0.00%%");
    fflush(stdout);

    while(downloadSize < train.fileSize){
        int ret = splice(sockfd, NULL, pipefds[1], NULL, 4096, SPLICE_F_MORE | SPLICE_F_MOVE);
        downloadSize += ret;
        splice(pipefds[0], NULL, fileFd, NULL, ret, SPLICE_F_MORE | SPLICE_F_MOVE);
        if(downloadSize / slice > percentage){
            ++percentage;
            printf("\r%5.2lf%%",(double)downloadSize/slice);
            fflush(stdout);
        }
    }
    puts("\r100.00%");
    puts("Download complete.");
}
*/


//陈强   断点重传——暂使用小火车，可以根据文件size在本函数使用零拷贝（待完成）

int sendFile(int sockfd, const char *fileName, off_t offSize, char *md5){

    
    int existTag;  //发送方是否存在该文件
    train_t train;
    off_t offsize = offSize;


    //在本地打开对应文件
    char filepath[200] = {0};
    sprintf(filepath, "%s%s%s", DEFAULT_DIR, "/", md5);
    int fileFd = open(filepath, O_RDONLY);

    if(-1 == fileFd){ //本地不存在该文件，通知对方
        existTag = 0;
        sendn(sockfd, &existTag, 4, 0);
        printf("本地无此文件，无法发送!\n");
        return 0;
    }

    //本地存在该文件，通知接收方准备接收
    existTag = 1;
    sendn(sockfd, &existTag, 4, 0);

    

    //1.发送文件名
    memset(&train, 0, sizeof(train));
    train.length = strlen(fileName);
    strncpy(train.data, fileName, train.length);
    int ret = sendn(sockfd, &train, 4 + train.length, 0);
    printf(">>file name:%s, ret:%d\n", train.data, ret);
    

    //2.发送文件的size
    struct stat st;
    fstat(fileFd, &st);
    memset(&train, 0, sizeof(train));
    train.length = sizeof(off_t);
    strncpy(train.data, (char*)&st.st_size, sizeof(off_t));
    ret = sendn(sockfd, &train, 4 + train.length, 0);
    printf(">>file size:%ld, ret:%d\n", st.st_size, ret);

    //3.发送文件的偏移量
    memset(&train, 0, sizeof(train));
    train.length = sizeof(off_t);
    strncpy(train.data, (char*)&offsize, sizeof(off_t));
    ret = sendn(sockfd, &train, 4 + train.length, 0);
    printf(">>file offsize:%ld, ret:%d\n", offsize,ret);

    //4.发送文件内容
    
    off_t needsize = st.st_size - offsize;

    off_t retVal = lseek(fileFd, offsize, SEEK_SET);
    if(retVal != offsize){
        printf("lseek failed!\n");
        return -1;
    }

    
    if(needsize > 104857600){
        int pipefds[2];
        pipe(pipefds);

        off_t sendSize = 0;

        while(sendSize <  needsize){
            int ret;
            ret = splice(fileFd, NULL, pipefds[1], NULL, 4096, SPLICE_F_MORE);
            ret = splice(pipefds[0], NULL, sockfd, NULL, ret, SPLICE_F_MORE);
            sendSize += ret;
        }
        close(pipefds[0]);
        close(pipefds[1]);
        close(fileFd);
        printf("Send %s complete.\n", fileName);
        return 0;
    }



    off_t total = 0;

    while(total < needsize){
        memset(&train, 0, sizeof(train));
        ret = read(fileFd, train.data, BUFFLEN);
        if(0 == ret){
            printf("file send over!\n");
            break;
        }
        ERROR_CHECK(ret, -1, "read");
        train.length = ret;
        ret = sendn(sockfd, &train, 4 + train.length, 0);
        total += train.length;
    }
    printf("file send over!\n");

    close(fileFd);
    return 0;
}

int recvFile(int sockfd, char *md5){

    int existTag;
    recv(sockfd, &existTag, 4, MSG_WAITALL);
    if(0 == existTag){
        printf("对方无此文件，接受失败!\n");
        return 0;
    }
    
    //1.接收文件名
    int length = 0;
    int ret = recv(sockfd, &length, 4, MSG_WAITALL);
    printf(">>1.filename length:%d, ret:%d\n", length, ret);
    
    char recvBuff[BUFFLEN] = {0};
    ret = recv(sockfd, recvBuff, length, MSG_WAITALL);
    printf(">>2.recv filename:%s\n", recvBuff);

    //在本地打开对应文件，如没有需要创建
    char filepath[200] = {0};
    sprintf(filepath, "%s%s%s", DEFAULT_DIR, "/", md5);
    int fileFd = open(filepath, O_RDWR | O_CREAT, 0666);

    //2.接收文件大小
    off_t filesize;
    ret = recv(sockfd, &length, 4, MSG_WAITALL);
    printf(">>3.length of filesize:%d\n", length);
    ret = recv(sockfd, &filesize, sizeof(filesize), MSG_WAITALL);
    printf(">>4.filesize:%ld\n", filesize);

    //3.接收文件偏移量
    off_t offsize;
    ret = recv(sockfd, &length, 4, MSG_WAITALL);
    printf(">>5.length of offsize:%d\n", length);
    ret = recv(sockfd, &offsize, sizeof(offsize), MSG_WAITALL);
    printf(">>6.offsize:%ld\n", offsize);


    off_t needsize = filesize - offsize;
    printf("start recv file, total neadsize =  filesize - offsize = %ld bytes\n", needsize);

    
    
    lseek(fileFd, offsize, SEEK_SET);
    


    //4.接收文件内容
    
    if(needsize > 104857600){  //需传输size大于100M，启用sendfile零拷贝技术

        ret = ftruncate(fileFd, filesize);//扩大size

        int pipefds[2];
        pipe(pipefds);
    
        off_t downloadSize = 0;
        int percentage = 0;
        off_t slice = needsize / 100;
        slice = slice == 0? 1:slice;
        printf("Percent: \n");
        printf("0.00%%");
        fflush(stdout);

        while(downloadSize < needsize){
            int ret = splice(sockfd, NULL, pipefds[1], NULL, 4096, SPLICE_F_MORE | SPLICE_F_MOVE);
            downloadSize += ret;
            splice(pipefds[0], NULL, fileFd, NULL, ret, SPLICE_F_MORE | SPLICE_F_MOVE);
            if(downloadSize / slice > percentage){
                ++percentage;
                printf("\r%5.2lf%%",(double)downloadSize/slice);
                fflush(stdout);
            }
        }
        puts("\r100.00%");
        puts("Download complete.");

        close(pipefds[0]);
        close(pipefds[1]);
        close(fileFd);
        return 0;
    }

    off_t slice = needsize / 100;
    off_t downloadSize = 0;
    off_t lastSize = 0;
    while(downloadSize < needsize){
        ret = recv(sockfd, &length, 4, MSG_WAITALL);
        if(0 == ret){
            printf("file has recv all\n");
            break;
        }
        memset(recvBuff, 0, sizeof(recvBuff));
        ret = recv(sockfd, recvBuff, length, MSG_WAITALL);
        downloadSize += ret;
        
        if(downloadSize - lastSize > slice){
            printf(">>download percent:%5.2lf%%\n",(double)downloadSize / needsize * 100);
            lastSize = downloadSize;
        }
        //写入本地
        ret = write(fileFd, recvBuff, ret);
        ERROR_CHECK(ret, -1, "write");
    }
    printf(">>download percent:100%%\n");
    close(fileFd);

    return 0;
}

int sendfd(int pipefd, int fd, char exitFlag)
{
	//1. 构造iov结构体
	struct iovec iov;
	iov.iov_base = &exitFlag;
	iov.iov_len = 1;

	//2. 构造cmsghdr
	int len = CMSG_LEN(sizeof(int));
	struct cmsghdr *p = (struct cmsghdr*)calloc(1, len);
	p->cmsg_len = len;
	p->cmsg_level = SOL_SOCKET;
	p->cmsg_type = SCM_RIGHTS;
	int * pfd = (int*)(CMSG_DATA(p));
	*pfd = fd;
	//3. 构造msghdr
	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = p;
	msg.msg_controllen = len;

	int ret = sendmsg(pipefd, &msg, 0);
	ERROR_CHECK(ret, -1, "sendmsg");
}

int recvfd(int pipefd, int *pfd, char *pexitFlag) 
{
	//1. 构造iov结构体
	struct iovec iov;
	iov.iov_base = pexitFlag;
	iov.iov_len = 1;

	//2. 构造cmsghdr
	int len = CMSG_LEN(sizeof(int));
	struct cmsghdr *p = (struct cmsghdr*)calloc(1, len);
	p->cmsg_len = len;
	p->cmsg_level = SOL_SOCKET;
	p->cmsg_type = SCM_RIGHTS;
	//3. 构造msghdr
	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = p;
	msg.msg_controllen = len;

	int ret = recvmsg(pipefd, &msg, 0);
	ERROR_CHECK(ret, -1, "recvmsg");
	*pfd = *(int*)CMSG_DATA(p);
	return ret;
}

