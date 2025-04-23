#include "head.h"
#define SOCKET_NUM 500
typedef struct{
    int fd;
    int cfd;
    int number;
    int pos;
    int end;
    int state;
}Task;

typedef struct{
    int start;
    int end;
    int number;
    char md5sum[50];
}BBQ;


typedef struct{
    char name[30];
    char passward[100];
    char token[50];
}Zhuce;
int sql_connect(MYSQL **conn)
{
    char server[]="localhost";
    char user[]="root";
    char password[]="950711";
    char database[]="Netdisk";//要访问的数据库名称
    *conn=mysql_init(NULL);
    if(!mysql_real_connect(*conn,server,user,password,database,0,NULL,0))
    {
        //    printf("Error connecting to database:%s\n",mysql_error(*conn));
        exit(0);
    }else{
        // printf("Connected...\n");
    }
    return 0;
}

int math_token(MYSQL *conn,char *name,char *token)
{
    int ret =-1;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select token from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    //puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        // printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else if(strcmp(row[0],token)==0)
                {
                    ret = 0;
                }
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
}


void epoll_func(int epfd,int fd,int caozuo,int duxie)
{
    struct epoll_event event;
    event.events = duxie;
    event.data.fd = fd;
    epoll_ctl(epfd,caozuo,fd,&event);
}

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

void disconnect(Task *pk,int epfd)
{
    if(pk->fd >0)
        close(pk->fd);
    epoll_func(epfd,pk->cfd,EPOLL_CTL_DEL,EPOLLOUT);
    epoll_func(epfd,pk->cfd,EPOLL_CTL_DEL,EPOLLIN);
    printf("已经断开连接\n");
    bzero(pk,sizeof(Task));
}

void add_task(Task *pt,BBQ *pb,int epfd)
{
    char path[100]={0};
    strcpy(path,DOWN_PATH);
    sprintf(path,"%s%s",path,pb->md5sum);
    pt->fd = open(path,O_RDONLY);
    printf("fd = %d\n",pt->fd);
    lseek(pt->fd,pb->start,SEEK_SET);
    pt->end = pb->end;
    pt->pos = pb->start;
    pt->number = pb->number;
    epoll_func(epfd,pt->cfd,EPOLL_CTL_DEL,EPOLLIN);
    epoll_func(epfd,pt->cfd,EPOLL_CTL_ADD,EPOLLOUT);
    printf("改变成功\n");
}


int main()
{
	int socketFd;
	socketFd=socket(AF_INET,SOCK_STREAM,0);
	ERROR_CHECK(socketFd,-1,"socket");
	struct sockaddr_in ser;
	bzero(&ser,sizeof(ser));
	ser.sin_family=AF_INET;
	ser.sin_port=htons(3000);
	ser.sin_addr.s_addr=inet_addr("0");//点分十进制转为32位的网络字节序
	int ret;
	ret=bind(socketFd,(struct sockaddr*)&ser,sizeof(ser));
	ERROR_CHECK(ret,-1,"bind");
	listen(socketFd,100);



    MYSQL *conn;
    sql_connect(&conn);
    Task task[SOCKET_NUM];
    bzero(task,SOCKET_NUM*sizeof(Task));
    Train_t train;
    Zhuce info;
    BBQ bq;
    int ready_num,i,j;


    int epfd = epoll_create(1);
    struct epoll_event evs[SOCKET_NUM];
    epoll_func(epfd,socketFd,EPOLL_CTL_ADD,EPOLLIN);

    while(1)
    {
        ready_num = epoll_wait(epfd,evs,SOCKET_NUM,-1);
        usleep(1000);
        for(i=0;i<ready_num;i++)
        {
            if(evs[i].events == EPOLLIN && evs[i].data.fd == socketFd)
            {  
                printf("连接请求\n");
                for(j=0;j<SOCKET_NUM;j++)
                    if(task[j].cfd ==0)
                        break;
                if(j ==SOCKET_NUM)
                    continue;
                task[j].cfd = accept(socketFd,NULL,NULL);
                task[j].state = 1;
                epoll_func(epfd,task[j].cfd,EPOLL_CTL_ADD,EPOLLIN);
                continue;
            }
            for(j=0;j<SOCKET_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && evs[i].data.fd == task[j].cfd)
                {
                    ret = one_recv(task[j].cfd,&train);
                    if(ret == -1)
                    {
                        disconnect(task+j,epfd);
                        break;
                    }
                    if(train.ctl_code == 1)//token认证
                    {
                        printf("token认证\n");
                        memcpy(&info,train.buf,train.Len);
                        ret = math_token(conn,info.name,info.token);
                        printf("ret = %d\n",ret);
                        if(ret == -1)
                            break;
                        else
                            task[j].state =2;
                        printf("name: %s\ntoken %s\n",info.name,info.token);
                    }else
                    {
                        printf("下载任务发送\n");
                        if(task[j].state !=2)
                        {
                            disconnect(task+j,epfd);
                            break;
                        }
                        memcpy(&bq,train.buf,train.Len);
    //                    printf("number =%d,start =%d,end= %d\nmd5sum= %s\n",
    //                           bq.number,bq.start,bq.end,bq.md5sum);
                        add_task(task+j,&bq,epfd);
                    }
                }
                if(evs[i].events == EPOLLOUT && evs[i].data.fd == task[j].cfd)
                {
                    if(task[j].pos < task[j].end)
                    {
                        ret = read(task[j].fd,train.buf,BUFSIZE);
                        train.Len = ret;
                        train.ctl_code = task[j].number;
                        ret = send(task[j].cfd,&train,train.Len+8,0);
                        if(ret == -1)
                            disconnect(task+j,epfd);
                    }else
                    {
                        train.ctl_code = task[j].number;
                        train.Len = 0;
                        send(task[j].cfd,&train,train.Len+8,0);
                        printf("第%d段文件下载完成\n",train.ctl_code);
                        disconnect(task+j,epfd);
                    }
                }
            }
        }
    }
    return 0;
}

