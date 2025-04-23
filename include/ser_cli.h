#ifndef __SER_CLI_H__
#define __SER_CLI_H__
#include "head.h"
typedef enum MSG_code{
    LOGIN_PRE=1,LOGIN_NO,LOGIN_POK,LOGIN_Q,LOGIN_OK,
    REGISTER_PRE,REGISTER_NO,REGISTER_POK,REGISTER_Q,REGISTER_OK,TOKEN_PLESE,
    OPERATE_Q,OPERATE_NO,OPERATE_OK,LS_Q,LS_OK ,
    DOWNLOAD_PRE,DOWNLOAD_POK,DOWNLOAD_Q,
    UPLOAD_PRE,UPLOAD_POK,UPLOAD_OK,UPLOAD_Q,
    DOWN_MORE_PRE,DOWN_MORE_POK
}MSG_code;//各个控制码，基本就申请，同意，操作。。
typedef struct{
    char name[30];
    char passward[100];
    char token[50];
}Zhuce;
typedef struct{
    char  buf1[100];//第一个存操作，第二个存对象
	char buf[200];
}QUR_msg;
int recvCYL(int fd,void *pbuf,int len);
int one_recv(int fd,Train_t *ptrain);
#endif

