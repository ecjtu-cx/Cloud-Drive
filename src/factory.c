#include "../include/factory.h"
#include "../include/ser_cli.h"
#include "../include/sql.h"

void log_name(int fd,const char *caozuo,char *name)
{
    time_t now;
    time(&now);
    char *p =ctime(&now);
    p[strlen(p)-5]=0;
    dprintf(fd,"账号: %-10s %-8s  time: %s\n",name,caozuo,p+8);
}
void log_caozuo(int fd,const char *caozuo,char *neirong,char *name)
{
    time_t now;
    time(&now);
    char *p =ctime(&now);
    p[strlen(p)-5]=0;
    dprintf(fd,"账号: %-10s %-4s %-6s time: %s\n",name,caozuo,neirong,p+8);
}
void log_port(int fd,const char *caozuo,char *name,struct sockaddr_in*clien)
{
    time_t now;
    time(&now);
    char *p =ctime(&now);
    p[strlen(p)-5]=0;
    dprintf(fd,"账号: %-10s %-8s  time: %s ip:%s 端口:%d\n",name,caozuo,p+8,
            inet_ntoa(clien->sin_addr),ntohs(clien->sin_port));
}


void factoryInit(process_data* down_thread,process_data* up_thread,client_t* client)
{
    bzero(down_thread,sizeof(process_data)*DOWN_THREAD_NUM);
    bzero(up_thread,sizeof(process_data)*DOWN_THREAD_NUM);
    bzero(client,sizeof(client_t)*SOCKET_NUM);
    for(int i=0;i<SOCKET_NUM;i++)
        client[i].rotate = -1;
}
void factoryStart(process_data* down_thread,process_data* up_thread)
{
    int i;
    int fds[2];
    printf("now will start downlod thread\n");
    for(i=0;i<DOWN_THREAD_NUM;i++)
    {
        socketpair(AF_LOCAL,SOCK_STREAM,0,fds);
        pthread_create(&down_thread[i].pid,NULL,down_func,(void*)fds[1]);
        down_thread[i].fd = fds[0];
    }
    printf("now will start upload thread\n");
    for(i=0;i<UP_THREAD_NUM;i++)
    {
        socketpair(AF_LOCAL,SOCK_STREAM,0,fds);
        pthread_create(&up_thread[i].pid,NULL,upload_func,(void*)fds[1]);
        up_thread[i].fd = fds[0];
    }
    printf("all pthread are ready\n");
}
void epoll_func(int epfd,int fd,int caozuo,int duxie)
{
    struct epoll_event event;
    event.events = duxie;
    event.data.fd = fd;
    epoll_ctl(epfd,caozuo,fd,&event);
}

int math_task(process_data *p,int max,int task_num)
{
    int min =0;
    for(int i=1;i<max;i++)
    {
        if(p[i].busy_num<p[min].busy_num)
            min =i;
    }
    if(p[min].busy_num >=task_num)
        return -1;
    return min;
}

void disconnect(SD_info *p,int fd,int epfd,int epfd_cin,int ret)
{
    close(p->download_fd);
    epoll_func(epfd,p->download_fd,EPOLL_CTL_DEL,EPOLLOUT);
    epoll_func(epfd_cin,p->download_fd,EPOLL_CTL_DEL,EPOLLIN);
    for(int i=0;i<10;i++)
    {
        if(p->cdinfo[i].state==1)
            close(p->cdinfo[i].sfd);
        if(p->cdinfo[i].state==2)
        {
            munmap(p->cdinfo[i].p,p->cdinfo[i].file.filesize);
            close(p->cdinfo[i].sfd);
        }
    }
    bzero(p,sizeof(SD_info));
    write(fd,&ret,4);
    printf("对方退出，所有下载已经取消\n");
}


void add_task(CD_info *ptask,DQ_buf *pdqbuf)
{
    char path[100]={0};
    strcpy(path,DOWN_PATH);
    sprintf(path,"%s%s",path,pdqbuf->file.md5sum);
    ptask->sfd = open(path,O_RDONLY);
    if(ptask->sfd == -1)
    {
        printf("打开文件出错,程序终止\n");
        exit(0);
    }
    ptask->cfd =pdqbuf->fd;
    ptask->pos = pdqbuf->pos;
    memcpy(&ptask->file,&pdqbuf->file,sizeof(File_info));
    if(ptask->file.filesize > 100*1<<20)//mmap
    {
        ptask->state = 2;
        ptask->p = (char*)mmap(NULL,ptask->file.filesize,PROT_READ,
                               MAP_SHARED,ptask->sfd,0);
    }
    else
    {
        ptask->state = 1;
        lseek(ptask->sfd,ptask->pos,SEEK_SET);
    }
    printf("%ld  download thread get task download file : %s\n",pthread_self(),pdqbuf->file.filename);
}

void del_task(CD_info *ptask)
{
    printf("file :%s 下载完成\n",ptask->file.filename);
    close(ptask->sfd);
    if(ptask->state ==2)
        munmap(ptask->p,ptask->file.filesize);
    bzero(ptask,sizeof(CD_info));
}

//void factoryDistory(process_data* down_thread,process_data* up_thread,client_t* client)
void* down_func(void *pF)
{
    SD_info client[AVG_CLIENT_NUM];
    bzero(client,AVG_CLIENT_NUM*sizeof(SD_info));
    int fd = (int)pF;
    int ready_num,ret,new_fd,i,j,k,readnnm;
    DQ_buf dqbuf;
    Train_t train;
    // printf("%ld  download thread get fd %d\n",pthread_self(),fd);
    int epfd = epoll_create(1);
    int epfd_cin = epoll_create(1);
    struct epoll_event event,evs[32],evsc[10];
    event.events = EPOLLIN;
    event.data.fd = fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    epoll_func(epfd,epfd_cin,EPOLL_CTL_ADD,EPOLLIN);
    while(1)
    {
        ready_num = epoll_wait(epfd,evs,32,-1);//发文件和管道;
        for( i=0;i<ready_num;i++)
        {
            if(evs[i].events == EPOLLIN && evs[i].data.fd == fd)
            {
                read(fd,&new_fd,4);//一个客户发生控制转移只有一次
                for( j=0;j<AVG_CLIENT_NUM;j++)
                {
                    //一定能找到
                    if(client[j].download_fd == 0)//找到一个空闲的；
                        //     k=j;
                        // if(client[j].download_fd == new_fd)
                        break;
                }
                //  if(j == AVG_CLIENT_NUM)//没找到；
                epoll_func(epfd_cin,new_fd,EPOLL_CTL_ADD,EPOLLIN);
                client[j].download_fd = new_fd;
                printf("增加了一个客户，他的ID %d  我给他的序列号:= %d\n",new_fd,j);
                continue;
            }
            if(evs[i].events == EPOLLIN && evs[i].data.fd == epfd_cin)
            {
                readnnm = epoll_wait(epfd_cin,evsc,10,0);
                for(int s = 0;s<readnnm;s++)
                {
                    for(j=0;j<AVG_CLIENT_NUM;j++)
                    {
                        if(evsc[s].events == EPOLLIN && evsc[s].data.fd ==  client[j].download_fd)
                        {
                            // printf("进来了收任务请求\n");
                            ret = recvCYL(client[j].download_fd,&dqbuf,sizeof(DQ_buf));
                            if(ret == -1)
                            {
                                disconnect(client+j,fd,epfd,epfd_cin,-1);
                                break;
                            }
                            //printf("name %s\nsize %d\n md5 %s\n",dqbuf.file.filename,
                            //       dqbuf.file.filesize,dqbuf.file.md5sum);
                            if(client[j].task_num >=10)
                                disconnect(client+j,fd,epfd,epfd_cin,-1);
                            for(k =0;k<10;k++)
                                if(client[j].cdinfo[k].sfd ==0)
                                    break;
                            client[j].task_num++;
                            add_task(client[j].cdinfo+k,&dqbuf);
                            // printf("增加监控:%d\n",client[j].download_fd);
                            epoll_func(epfd,client[j].download_fd,EPOLL_CTL_ADD,EPOLLOUT);
                            //  printf("cfd = %d,stat =%d,pso = %d,sfd = %d,file = %s\n",client[j].cdinfo[k].cfd,client[j].cdinfo[k].state,
                            //       client[j].cdinfo[k].pos,client[j].cdinfo[k].sfd,client[j].cdinfo[k].file.filename);
                        }
                    }
                }
            }
            for(j=0;j<AVG_CLIENT_NUM;j++)
                if(evs[i].events == EPOLLOUT && evs[i].data.fd ==  client[j].download_fd)
                {
                    usleep(1000);
                    //            printf("client[j].download_fd = %d\n",client[j].download_fd);
                    CD_info *pc=client[j].cdinfo;
                    int slice = (BUFSIZE)/client[j].task_num;
                    for(k =0;k<10;k++)
                    {
                        if(pc[k].state ==  1)//正常小火车
                        {
                            //          printf("client[j].cfd = %d\n",client[j].cdinfo[k].cfd);
                            if(pc[k].pos < pc[k].file.filesize)
                            {
                                ret = read(pc[k].sfd,train.buf,slice);
                                train.ctl_code = pc[k].cfd;
                                train.Len =ret;
                                pc[k].pos += ret;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                {
                                    disconnect(client+j,fd,epfd,epfd_cin,-1);
                                    break;
                                }
                                //            else
                                //                printf("-nomal file :%s have download %d 本次发送 %d\n",pc[k].file.filename,pc[k].pos,ret-8);
                            }else
                            {
                                train.ctl_code = pc[k].cfd;
                                train.Len =0;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                {
                                    disconnect(client+j,fd,epfd,epfd_cin,-1);
                                    break;
                                }
                                client[j].task_num--;
                                //       printf("已经发送len=0\n");
                                if(client[j].task_num ==0)
                                    disconnect(client+j,fd,epfd,epfd_cin,0);
                                else
                                    del_task(&client[j].cdinfo[k]);
                            }
                        }
                        if(pc[k].state == 2)
                        {
                            if(pc[k].pos + slice < pc[k].file.filesize)
                            {
                                memcpy(train.buf,pc[k].p+pc[k].pos,slice);
                                train.ctl_code = pc[k].cfd;
                                train.Len =slice;
                                pc[k].pos += slice;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                {
                                    disconnect(client+j,fd,epfd,epfd_cin,-1);
                                    break;
                                }
                                //            else
                                //                printf("-mmap file :%s have download %d 本次发送 %d\n",pc[k].file.filename,pc[k].pos,ret-8);
                            }else
                            {
                                memcpy(train.buf,pc[k].p+pc[k].pos,pc[k].file.filesize-pc[k].pos);
                                train.ctl_code = pc[k].cfd;
                                train.Len =pc[k].file.filesize-pc[k].pos;
                                pc[k].pos =pc[k].file.filesize;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                {
                                    disconnect(client+j,fd,epfd,epfd_cin,-1);
                                    break;
                                }
                                //              else
                                //                   printf("-trail mmap file :%s have download %d本次发送 %d\n",pc[k].file.filename,pc[k].pos,ret);
                                train.ctl_code = pc[k].cfd;
                                train.Len =0;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                {
                                    disconnect(client+j,fd,epfd,epfd_cin,-1);
                                    break;
                                }
                                client[j].task_num--;
                                if(client[j].task_num ==0)
                                    disconnect(client+j,fd,epfd,epfd_cin,0);
                                else
                                    del_task(&client[j].cdinfo[k]);
                            }

                        }
                    }
                }

        }
    }
}

void updisconect(UP_INFO *p,int epfd,int fd,int ret)
{
    close(p->soketfd);
    epoll_func(epfd,p->soketfd,EPOLL_CTL_DEL,EPOLLIN);
    if(p->state==2)
        munmap(p->p,p->file.filesize);
    if(p->fd >0)
    {
        close(p->fd);
        if(ret ==-1)
        {
            char path[100];
            strcpy(path,DOWN_PATH);
            sprintf(path,"%s%s",path,p->file.md5sum);
            remove(path);
            printf("对方退出，file :%s 上传任务已经取消\n",p->file.filename);
        }
    }
    if(ret ==0)//更新数据库
    {
        add_file(p->code,p->name,&p->file);
    }
    write(fd,&ret,4);
    bzero(p,sizeof(UP_INFO));
}

void add_uptask(UP_INFO *ptask,DQ_buf *pdqbuf)
{
    char path[100]={0};
    strcpy(path,DOWN_PATH);
    sprintf(path,"%s%s",path,pdqbuf->file.md5sum);
    ptask->fd = open(path,O_RDWR|O_CREAT,0666);
    if(ptask->fd == -1)
    {
        printf("打开文件出错,程序终止\n");
        exit(0);
    }
    ptask->pos = pdqbuf->pos;
    memcpy(&ptask->file,&pdqbuf->file,sizeof(File_info));
    if(ptask->file.filesize > 100*1<<20)//mmap
    {
        ftruncate(ptask->fd,ptask->file.filesize);
        ptask->state = 2;
        ptask->p = (char*)mmap(NULL,ptask->file.filesize,PROT_READ|PROT_WRITE,
                               MAP_SHARED,ptask->fd,0);
    }
    else
    {
        ptask->state = 1;
    }
    printf("%ld  download thread get task upload file : %s\n",pthread_self(),pdqbuf->file.filename);
    strcpy(ptask->name,pdqbuf->client_name);
    ptask->code = pdqbuf->code;
}

void* upload_func(void *pF)
{
    int fd = (int)pF,new_fd;
    UP_INFO task[UP_TASK_NUM];
    bzero(task,UP_TASK_NUM*sizeof(UP_INFO));
    int front =0,rear =0;
    Train_t train;
    DQ_buf dqbuf;
    int ready_num,i,j,k,ret;
    int epfd = epoll_create(1);
    struct epoll_event event,evs[UP_TASK_NUM];
    event.events = EPOLLIN;
    event.data.fd = fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    while(1)
    {
        ready_num = epoll_wait(epfd,evs,UP_TASK_NUM,-1);
        for(i=0;i<ready_num;i++)
        {
            if(evs[i].events == EPOLLIN && evs[i].data.fd == fd)
            {
    //            printf("收到主线程请求\n");
                read(fd,&new_fd,4);
                task[rear].soketfd = new_fd;
                epoll_func(epfd,new_fd,EPOLL_CTL_ADD,EPOLLIN);
                MOVE(rear);
    //         printf("rear = %d\n",rear);
                continue;
            }
            for(j=0;j<UP_TASK_NUM;j++)
            {
                if((evs[i].events == EPOLLIN && evs[i].data.fd == task[j].soketfd))
                {
                    ret = recvCYL( task[j].soketfd,&train.Len,4);
                    ret = recvCYL( task[j].soketfd,&train.ctl_code,4);
                    if(ret == -1)
                    {
                        updisconect(task+j,epfd,fd,ret);
                        MOVE(front);
                        break;
                    }
                    if(train.ctl_code == UPLOAD_Q)
                    {
                        ret = recvCYL( task[j].soketfd,train.buf,train.Len);
                        if(ret == -1)
                        {
                            updisconect(task+j,epfd,fd,ret);
                            MOVE(front);
                            break;
                        }
                        memcpy(&dqbuf,train.buf,train.Len);
                        //          printf("name:%s  file: %s  code: %d\n ",dqbuf.client_name,
                        //                 dqbuf.file.filename,dqbuf.code);
                        add_uptask(task+j,&dqbuf);
   //                     printf("name:%s  file: %s  code: %d md5sum %s\n ",task[j].name,
   //                            task[j].file.filename,task[j].code,task[j].file.md5sum);
                    }
                    if(train.ctl_code == UPLOAD_OK)
                    {
                        if(train.Len !=0)
                        {
                            if(task[j].state ==1)
                            {
                                ret = recvCYL(task[j].soketfd,train.buf,train.Len);
                                if(ret == -1)
                                {
                                    updisconect(task+j,epfd,fd,ret);
                                    MOVE(front);
                                    break;
                                }
                                task[j].pos += train.Len;
                                write(task[j].fd,train.buf,train.Len);
                        //                                        printf("-normal file :%s have upload %d,ret =%d\n",
                        //                                               task[j].file.filename,task[j].pos,train.Len);
                            }
                            if(task[j].state == 2)
                            {
                                ret = recvCYL(task[j].soketfd,task[j].p+task[j].pos,train.Len);
                                if(ret == -1)
                                {
                                    updisconect(task+j,epfd,fd,ret);
                                    MOVE(front);
                                    break;
                                }
                                task[j].pos += train.Len;
             //                                                   printf("-mmap file :%s have upload %d,ret =%d\n",
             //                                                          task[j].file.filename,task[j].pos,train.Len);
                            }
                        }else
                        {
                            MOVE(front);
                            updisconect(task+j,epfd,fd,0);
  //                          printf("front = %d\n",front);
                        }
                    }
                }
            }
        }
    }
}
