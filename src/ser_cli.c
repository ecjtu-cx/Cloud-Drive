#ifndef __SER_CLI_H__
#define __SER_CLI_H__
#include "../include/ser_cli.h"
#include "../include/head.h"
int recvCYL(int fd,void *pbuf,int len)
{
    char *buf = (char*)pbuf;
    int total = 0,ret;
    while(total<len)
    {
        ret = recv(fd,buf+total,len-total,0);
        if(ret == 0)
        {
            return -1;
        }
        total += ret;
    }
    return 0;
}

int one_recv(int fd,Train_t *ptrain)
{
    int ret = recvCYL(fd,&ptrain->Len,4);
    ERROR_CHECK(ret,-1,"client quit");
    ret = recvCYL(fd,&ptrain->ctl_code,4);
    ERROR_CHECK(ret,-1,"client quit");
    ret = recvCYL(fd,&ptrain->buf,ptrain->Len);
    ERROR_CHECK(ret,-1,"client quit");
    return 0;
}

#endif
