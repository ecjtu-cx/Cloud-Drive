#ifndef __FUNC_H__
#define __FUNC_H__
#include<openssl/md5.h>
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
#include <mysql/mysql.h>
#define ARGS_CHECK(argc,val) {if(argc!=val) \
	{printf("error args\n");return -1;}}
#define ERROR_CHECK(ret,retval,funcName) {if(ret==retval) \
	{printf("%d:",__LINE__);fflush(stdout);perror(funcName);return -1;}}
#define THREAD_ERROR_CHECK(ret,funcName) {if(ret!=0) \
	{printf("%s:%s\n",funcName,strerror(ret));return -1;}}
#define BUFSIZE 10240
#define DOWN_PATH "./Sdisk/"
#define SPOT_NUM 3  //多点下载的服务器数量
typedef struct{
	int Len;
    int ctl_code;
	char buf[BUFSIZE];
}Train_t;

typedef struct {
    char filename[30];
    int filesize;
    char md5sum[50];
}File_info;
typedef struct {
    int cfd;    //客户的打开文件fd
    int state;  //0未下载；1用普通下载，2用mmap映射
    char *p;    //mmap的堆区指针
    int pos;    //当前下载的位置
    File_info file;     //文件信息
    int sfd;    //我的打开文件fd
}CD_info;
typedef struct {
    int fd;     //客户的下载文件fd，用于表示是哪个文件/应该用序号，我这个不好
    int pos;    //在哪开始下载
    char client_name[30];   //客户姓名----上传有用
    int code ;      //客户下载请求的code值-------上传有用
    File_info file;     //文件信息
}DQ_buf;
typedef struct {
    int download_fd;    //客户socketfd
    int task_num;       //他当前下载了几个文件
    CD_info cdinfo[10];//发生变化必改状态位；
//    int s_fd[10];
}SD_info;
typedef struct {
    int fd;     
    int state;
    int soketfd;
    int pos;
    char *p;
    char name[30];
    int code ;
    File_info file;
}UP_INFO;
typedef struct {
    struct sockaddr_in sfd_in[SPOT_NUM];//服务器给的多点下载的服务器地址
    File_info file;
}MQ_buf;
#endif
