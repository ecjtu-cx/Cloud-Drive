#include "child.h"
#include "md5.h"
#define NM 5

void wwrite(Package *pack)
{
    lseek(pack->tfd,0,SEEK_SET);
    write(pack->tfd,pack->sec,sizeof(Section)*SECTION_NUM);
    fdatasync(pack->tfd);
}

int mtoken_ident(int epfd,struct sockaddr_in ser)
{
    Train_t train;
    int cFd,ret;
    cFd=socket(AF_INET,SOCK_STREAM,0);
    ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
    ERROR_CHECK(ret,-1,"connect");
    epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);
    train.Len = sizeof(Zhuce);
    memcpy(train.buf,&login_msg,train.Len);
    train.ctl_code = 1;
    ret = send(cFd,&train,train.Len+8,0);
    //    printf("认证 successful\n");
    return cFd;
}

int find_can_work(Package *pack)
{
    int j;
    for( j=0;j<SECTION_NUM;j++)
        if(pack->sec[j].state ==0)
            break;
    if(j ==SECTION_NUM)
        return -1;
    return j;
}

//第一个整数是三个描述符的第哪一个，第二个是10个序列的那个小任务；
void send_task(Package *pack,int k,int j,int epfd)
{
    BBQ bq;
    Train_t train;
    pack->sec[j].state = 1;
    bq.start=pack->sec[j].start;
    bq.end=pack->sec[j].end;
    bq.number = pack->sec[j].number;
    strcpy(bq.md5sum,pack->mq.file.md5sum);
    memcpy(&train.buf,&bq,sizeof(BBQ));
    train.Len = sizeof(BBQ);
    train.ctl_code = 2;
    pack->sfd[k]=mtoken_ident(epfd,pack->mq.sfd_in[k]);
    send(pack->sfd[k],&train,8+train.Len,0);

}


void init_task(Package *pack,MQ_buf* pm,int epfd)
{
    char tempath[100]={0};
    char filepath[100]={0};
    strcpy(tempath,DOWN_PATH);
    strcpy(filepath,DOWN_PATH);
    sprintf(tempath,"%s%s.temp",tempath,pm->file.filename);
    sprintf(filepath,"%s%s",filepath,pm->file.filename);
    pack->tfd = open(tempath,O_RDWR);
    if(pack->tfd == -1)
    {
        int slice = pm->file.filesize/SECTION_NUM;
        for(int i=0;i<SECTION_NUM;i++)
        {
            pack->sec[i].number=i;
            pack->sec[i].state =0;
            pack->sec[i].start =i*slice;
            pack->sec[i].end =(i+1)*slice;
        }
        pack->sec[SECTION_NUM-1].end = pm->file.filesize;
        pack->tfd = open(tempath,O_RDWR|O_CREAT,0666);
        printf("path =%s\n",tempath);
        printf("filepath =%s\n",filepath);
        wwrite(pack);
    }else
    {
        read(pack->tfd,pack->sec,sizeof(Section)*SECTION_NUM);
        for(int j=0;j<SECTION_NUM;j++)
            if(pack->sec[j].state ==1)
                pack->sec[j].state = 0;
        printf("qqqq\n");
    }
    memcpy(&pack->mq,pm,sizeof(MQ_buf));
    pack->ffd = open(filepath,O_RDWR|O_CREAT,0666);
    ftruncate(pack->ffd,pack->mq.file.filesize);
    pack->p = (char*)mmap(NULL,pack->mq.file.filesize,PROT_READ|PROT_WRITE,
                          MAP_SHARED,pack->ffd,0);
    for(int i=0;i<SPOT_NUM;i++)
    {
        int j = find_can_work(pack);
        if(j ==-1)
            break;
        send_task(pack,i,j,epfd);
    }
}





void* vip_func(void*pfd)
{
    int pipefd = (int)pfd,epfd;
    Package pk[NM];
    bzero(pk,NM*sizeof(Package ));
    MQ_buf mq;
    BBQ bq;
    Train_t train;
    epfd = epoll_create(1);
    struct epoll_event evs[NM*SPOT_NUM+1];
    epoll_func(epfd,pipefd,EPOLL_CTL_ADD,EPOLLIN);
    int ready_num,i,j,k,ret,len,code;
    time_t newtime,oldtime;
    time(&oldtime);


    while(1)
    {
        ready_num = epoll_wait(epfd,evs,NM*SPOT_NUM+1,-1);
        for(i=0;i<ready_num;i++)
        {
            if(evs[i].events == EPOLLIN && evs[i].data.fd == pipefd)
            {
                read(pipefd,&mq,sizeof(MQ_buf));
                for(j=0;j<NM;j++)
                    if(pk[j].ffd ==0)
                        break;
                if(j == NM)
                {
                    printf("多点下载任务达到上限，请稍后再试\n");
                    continue;
                }
                init_task(pk+j,&mq,epfd);
                printf("file:%s filesize %d\n",mq.file.filename,mq.file.filesize);
            }
            for(j=0;j<NM;j++)
            {
                for(k=0;k<SPOT_NUM;k++)
                {
                    if(evs[i].events == EPOLLIN && evs[i].data.fd == pk[j].sfd[k])
                    {
                        ret =recvCYL(pk[j].sfd[k],&len,4);
                        ret =recvCYL(pk[j].sfd[k],&code,4);
                        if(len !=0)
                        {
                            ret =recvCYL(pk[j].sfd[k],pk[j].p+pk[j].sec[code].start,len);
                            pk[j].sec[code].start += len;
                            wwrite(pk+j);
                            time(&newtime);
                            if(newtime-oldtime>=1)
                            {
                                oldtime = newtime;
                            printf("第 %d 个服务器下载  第 %d 段文件\n",k,code);
                            printf("共有 %d/%d ,本次接收 %d\n",pk[j].sec[code].start,
                                   pk[j].sec[code].end,len);
                            }
                        }else
                        {
                            close(pk[j].sfd[k]);
                            epoll_func(epfd,pk[j].sfd[k],EPOLL_CTL_DEL,EPOLLIN);
                            pk[j].sfd[k]=0;
                            pk[j].sec[code].state = 2;
                            int c = find_can_work(pk+j);
                            if(c != -1)
                            {
                                send_task(pk+j,k,c,epfd);   
                            }
                            wwrite(pk+j);
                            int s;
                            for( s=0;s<SECTION_NUM;s++)
                                if(pk[j].sec[s].state !=2)
                                    break;
                            if(s != SECTION_NUM)
                                break;
                            else //任务结束；
                            {
                                munmap(pk[j].p,pk[j].mq.file.filesize);
                                close(pk[j].ffd);
                                close(pk[j].tfd);
                                char tempath[100]={0};
                                strcpy(tempath,DOWN_PATH);
                                sprintf(tempath,"%s%s.temp",tempath,pk[j].mq.file.filename);
                                printf("temp :%s\n",tempath);
                                remove(tempath);
                                bzero(pk+j,sizeof(Package));
                                printf("file: %s 多点下载完成\n",pk[j].mq.file.filename);
                            }
                        }
                    }

                }
            }
        }
    }


    return NULL;
}
