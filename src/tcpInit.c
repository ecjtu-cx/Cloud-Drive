#include "../include/factory.h"

int tcpInit(int *pSocketFd,FILE *fp)
{
    char buf[100],ip[20],port[10],*p;
    fgets(buf,100,fp);
    p=buf;
    while(*p++!='=');
    strcpy(ip,p);
    ip[strlen(ip)-1] = 0;
    fgets(buf,100,fp);
    p=buf;
    while(*p++!='=');
    strcpy(port,p);
    port[strlen(port)-1] = 0;
	int socketFd;
//    printf("%s\n%s\n",ip,port);
	socketFd=socket(AF_INET,SOCK_STREAM,0);
	ERROR_CHECK(socketFd,-1,"socket");
	struct sockaddr_in ser;
	bzero(&ser,sizeof(ser));
	ser.sin_family=AF_INET;
	ser.sin_port=htons(atoi(port));
	ser.sin_addr.s_addr=inet_addr(ip);//点分十进制转为32位的网络字节序
	int ret;
	ret=bind(socketFd,(struct sockaddr*)&ser,sizeof(ser));
	ERROR_CHECK(ret,-1,"bind");
	listen(socketFd,100);//缓冲区的大小，一瞬间能够放入的客户端连接信息
	*pSocketFd=socketFd;
	return 0;
}
