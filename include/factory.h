#ifndef __FACTORY_H__
#define __FACTORY_H__
#include "head.h"
#define DOWN_THREAD_NUM 2      //下载线程数 
#define UP_THREAD_NUM 2        //上传线程数 
#define DOWN_TASK_NUM 1000      //忘了是干啥的，感觉没用呀
#define UP_TASK_NUM 300         //单个线程最大上传任务量
#define MAX_CONNECT_NUM 3000    //主线程最多同时服务客户数量
#define SOCKET_NUM 5000         //多申请点空间
#define AVG_CLIENT_NUM 30       //单个下载线程最大服务客户数量，
                                //每个客户可同时下载最多10个文件

#define MOVE(i) (i = (i+1)%UP_TASK_NUM) //用于入队出队移动

typedef struct{
    pthread_t pid;
    int fd;
    int busy_num;//正在服务的客户数量
}process_data;
typedef struct{
    int state;          //客户状态 0,未连接，1已经连接，2已经是登录状态
    char name[30];       //客户姓名   
    int code;           //客户当前的code值，用于查数据库
    int rotate;         //当前轮盘的值，用于超时断开
}client_t;

void factoryInit(process_data*,process_data*,client_t*);
void factoryStart(process_data*,process_data*);
//void factoryDistory(client_t*);
int tcpInit(int*,FILE *);
void epoll_func(int,int,int,int);
void log_port(int fd,const char *caozuo,char *name,struct sockaddr_in*clien);
void log_name(int fd,const char *caozuo,char *name);
void log_caozuo(int fd,const char *caozuo,char *neirong,char *name);
int math_task(process_data *p,int max,int);
void* down_func(void *pF); //下载线程
void* upload_func(void *pF);//上传线程
int tranFile(int);
#endif
