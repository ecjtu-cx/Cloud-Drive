#ifndef __CFACTORY_H__
#define __CFACTORY_H__ 
#include "head.h"
#include "md5.h"

extern struct sockaddr_in ser;
extern Zhuce login_msg;
extern char path[];
void epoll_func(int epfd,int fd,int caozuo,int duxie);
void ssend(int cFd,int epfd,Train_t *ptrain);
int recvCYL(int fd,void *pbuf,int len);
int one_recv(int fd,Train_t *ptrain);
int token_ident(int epfd,int);
int get_file_info(char *path,File_info *pfile);


#endif
