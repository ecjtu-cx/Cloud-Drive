#ifndef __FUNC_H__
#define __FUNC_H__
#include <crypt.h>
//#include<openssl/md5.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/msg.h>
#include <strings.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define ARGS_CHECK(argc,val) {if(argc!=val) \
	{printf("error args\n");return -1;}}
#define ERROR_CHECK(ret,retval,funcName) {if(ret==retval) \
	{printf("%d:",__LINE__);fflush(stdout);perror(funcName);return -1;}}
#define THREAD_ERROR_CHECK(ret,funcName) {if(ret!=0) \
	{printf("%s:%s\n",funcName,strerror(ret));return -1;}}
#define BUFSIZE 10240
#define SECTION_NUM 10
#define SPOT_NUM 3
typedef enum MSG_code{
    LOGIN_PRE=1,LOGIN_NO,LOGIN_POK,LOGIN_Q,LOGIN_OK,
    REGISTER_PRE,REGISTER_NO,REGISTER_POK,REGISTER_Q,REGISTER_OK,TOKEN_PLESE,
    OPERATE_Q,OPERATE_NO,OPERATE_OK,LS_Q,LS_OK,
    DOWNLOAD_PRE,DOWNLOAD_POK,DOWNLOAD_Q,
    UPLOAD_PRE,UPLOAD_POK,UPLOAD_OK,UPLOAD_Q,
    DOWN_MORE_PRE,DOWN_MORE_POK
}MSG_code;
typedef struct{
    char name[30];
    char passward[100];     //登录成功后，用于存放当前code值
    char token[50];
}Zhuce;
typedef struct{
	int Len;
    int  ctl_code;
	char buf[BUFSIZE];
}Train_t;
typedef struct{
    char  buf1[100];
	char buf[200];
}QUR_msg;
typedef struct {
    char filename[30];
    int filesize;
    char md5sum[50];
}File_info;
typedef struct {
    int fd;
    int state;
    char *p;
    int pos;
    File_info file;
    struct timeval oldtime;         //计时用于计算网速
    struct timeval newtime;
    int eve_len;             //单词下载长度
    int sokectfd;
}CD_info;
typedef struct {
    int fd;
    int pos;
    char client_name[30];
    int code ;
    File_info file;
}DQ_buf;
typedef struct {
    struct sockaddr_in sfd_in[SPOT_NUM];
    File_info file;
}MQ_buf;

#endif
