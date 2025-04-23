#ifndef __CHILD_H__
#define __CHILD_H__
#include "head.h"
#include "cfactory.h"
#define DOWN_PATH "./Cdisk/"

extern struct sockaddr_in ser;
extern Zhuce login_msg;
extern char path[];

typedef struct{
    pthread_t pid;
    int fd;
    int busy_num;
}process_data;

//用于多点下载
typedef struct{
    int number;
    int state;
    int start;      //0未开始下载-1正在下载-2本段已经下载完成
    int end;
}Section;

//用于多点下载
typedef struct{
    Section sec[SECTION_NUM];   //多少段
    int sfd[SPOT_NUM];          //几个点的服务器信息
    int ffd;            
    int tfd;            //临时文件的fd
    char *p;            //mmap的地址p
    MQ_buf mq;
//    char *tp;
}Package;

typedef struct{
    int start;      //单个任务开始位置
    int end;        //结束位置
    int number;     //序列号---没用
    char md5sum[50];    
}BBQ;

void *normal_func(void*);
void *vip_func(void*);


#endif
