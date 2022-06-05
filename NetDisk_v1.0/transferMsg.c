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

int sendFile(int sockfd, char *fileName){                                         
    int fileFd = open(fileName, O_RDONLY);
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
        /*
        if(uploadSize / slice > percentage){
            ++percentage;
            printf("\r%5.2lf%%",(double)uploadSize/slice);
            fflush(stdout);
        }
        */
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

