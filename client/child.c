#include "child.h"
#include "md5.h"

void add_task(DQ_buf *pdqbuf,CD_info *ptask,File_info *pfile,int cfd)
{
    char path[100]={0};
    
    struct stat stat;
    strcpy(path,DOWN_PATH);
    sprintf(path,"%s%s",path,pfile->filename);
    ptask->fd = open(path,O_RDWR|O_CREAT,0666);
    //如果文件大小已经等于最大，比对MD5，不等重新下；
    fstat(ptask->fd,&stat);
    ptask->pos = stat.st_size;
    if(ptask->pos == pfile->filesize)
    {
        char md5_str[MD5_STR_LEN + 1];
        Compute_file_md5(path, md5_str);
        //        printf("now md5sum : %s\nreal md5sum : %s\n",md5_str,pfile->md5sum);
        if(strcmp(md5_str,pfile->md5sum) !=0)
        {
            printf("文件被破坏，将重新下载\n");
            printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
            fflush(stdout);
            ptask->pos = 0;
            lseek(ptask->fd,0,SEEK_SET);
        }

    }
    memcpy(&ptask->file,pfile,sizeof(File_info));
    if(ptask->file.filesize > 100*1<<20)//mmap
    {
        ptask->state = 2;
        ftruncate(ptask->fd,ptask->file.filesize);
        ptask->p = (char*)mmap(NULL,ptask->file.filesize,PROT_READ|PROT_WRITE,
                               MAP_SHARED,ptask->fd,0);
    }else
    {
        ptask->state = 1;
        lseek(ptask->fd,ptask->pos,SEEK_SET);
    }
    pdqbuf->pos = ptask->pos;
    pdqbuf->fd = ptask->fd;
    memcpy(&pdqbuf->file,&ptask->file,sizeof(File_info));
    send(cfd,pdqbuf,sizeof(DQ_buf),0);
}

void add_uptask(DQ_buf *pdqbuf,CD_info *ptask,File_info *pfile,int cfd,char*uppath,int code)
{
    Train_t train;
    struct stat stat;
    ptask->fd = open(uppath,O_RDONLY);
    //如果文件大小已经等于最大，比对MD5，不等重新下；
    fstat(ptask->fd,&stat);
    pfile->filesize = stat.st_size;
    ptask->pos = 0;
    memcpy(&ptask->file,pfile,sizeof(File_info));
    if(ptask->file.filesize > 100*1<<20)//mmap
    {
        ptask->state = 2;
        ptask->p = (char*)mmap(NULL,ptask->file.filesize,PROT_READ,
                               MAP_SHARED,ptask->fd,0);
    }else
    {
        ptask->state = 1;
    }
    pdqbuf->pos = 0;
    pdqbuf->fd = 0;
    pdqbuf->code = code;
    strcpy(pdqbuf->client_name,login_msg.name);
    memcpy(&pdqbuf->file,&ptask->file,sizeof(File_info));
    train.Len = sizeof(DQ_buf);
    train.ctl_code = UPLOAD_Q;
    memcpy(train.buf,pdqbuf,train.Len);
    send(cfd,&train,8+train.Len,0);
}
void error_jude(int ret)
{
    if (ret ==-1)
    {
        printf("服务器断开,程序终止\n");
        exit(0);
    }
}
void del_uptask(CD_info*ptask,int epfd)
{
    epoll_func(epfd,ptask->sokectfd,EPOLL_CTL_DEL,EPOLLOUT);
    close(ptask->sokectfd);
    close(ptask->fd);
    if(ptask->state ==2)
        munmap(ptask->p,ptask->file.filesize);
    bzero(ptask,sizeof(CD_info));
}

void print_file(CD_info *task,CD_info *uptask)
{
    system("clear");
    int k=0;
    printf("\n\n正在下载的任务:\n");
    for(int i=0;i<10;i++)
    {
        if(task[i].fd>0)
        {
            float s = (float)task[i].eve_len/((task[i].newtime.tv_sec-task[i].oldtime.tv_sec)*1000000
                                                   +task[i].newtime.tv_usec-task[i].oldtime.tv_usec);
            k++;
            printf("--%d、FileName: %-10s  download: %7.3f/%7.3fMb  %7.3fMb/s\n",
                   k,task[i].file.filename,task[i].pos/1000000.0,task[i].file.filesize/1000000.0,s);
        }
    }
     k=0;
    printf("\n\n正在上传的任务:\n");
    for(int i=0;i<10;i++)
    {
        if(uptask[i].fd>0)
        {
            float s = (float)uptask[i].eve_len/((uptask[i].newtime.tv_sec-uptask[i].oldtime.tv_sec)*1000000
                                                   +uptask[i].newtime.tv_usec-uptask[i].oldtime.tv_usec);
            k++;
            printf("--%d、FileName: %-10s  up_load:  %7.3f/%7.3fMb  %7.3fMb/s\n",
                   k,uptask[i].file.filename,uptask[i].pos/1000000.0,uptask[i].file.filesize/1000000.0,s);
        }
    }
    printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
    fflush(stdout);
}

void *normal_func(void*pF)
{
    int fd = (int)pF,cfd=0;
    int ready_num;
    Train_t train;
    File_info file;
    CD_info task[10],uptask[10];
    DQ_buf dqbuf;
    char uppath[100];
    int task_num =0,i,j,k,len,ret,code;
    bzero(task,10*sizeof(CD_info));
    bzero(uptask,10*sizeof(CD_info));
    int epfd = epoll_create(1);
    struct epoll_event event,evs[15];
    epoll_func(epfd,fd,EPOLL_CTL_ADD,EPOLLIN);

    while(1)
    {
        ready_num = epoll_wait(epfd,evs,15,-1);
        for( i=0;i<ready_num;i++)
        {
            //主线程分任务啦
            if(evs[i].events == EPOLLIN && evs[i].data.fd == fd)
            {

                read(fd,&file,sizeof(File_info));
                //上传任务;
                if(file.filesize == -2)
                {
                    print_file(task,uptask);
                    break;
                }
                if(file.filesize == -1)
                {
                    read(fd,uppath,100);
                    read(fd,&code,4);
                    for(j =0;j<10;j++)
                        if(uptask[j].sokectfd == 0)
                            break;
                    if(j==10)
                    {
                        printf("上传任务已达到上限，请稍后再试\n");
                        printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
                        fflush(stdout);
                        continue;
                    }
                    uptask[j].sokectfd= token_ident(epfd,EPOLLOUT);//新建fd
                    train.Len = 0;//用于服务器任务转移；
                    train.ctl_code = UPLOAD_Q;
                    send(uptask[j].sokectfd,&train,8+train.Len,0);
                    add_uptask(&dqbuf,uptask+j,&file,uptask[j].sokectfd,uppath,code);//已经发送请求;
                }
                else//下载；
                {
                    if(task_num >=10)//任务为0时关闭描述符cfd,并置0
                    {
                        printf("下载任务已达到上限，请稍后再试\n");
                        continue;
                    }
                    if(task_num ==0)//第一次连接;
                    {
                        cfd = token_ident(epfd,EPOLLIN);//新建fd
                        train.Len = 0;//用于服务器任务转移；
                        train.ctl_code = DOWNLOAD_Q;
                        send(cfd,&train,8+train.Len,0);
                    }
                    for(j =0;j<10;j++)
                        if(task[j].fd==0)//任务完成必须改为0；
                            break;
                    task_num++;
                    add_task(&dqbuf,task+j,&file,cfd);//已经发送请求;
                    //       printf("fd =%d,stat = %d,pos = %d,filename=%s\nfilesize = %d,md5sun= %s\n",task[j].fd,task[j].state,
                    //              task[j].pos,task[j].file.filename,task[j].file.filesize,
                    //              task[j].file.md5sum);
                    //       printf("tasknum =%d,已发送下载请求成功 file :%s\n",task_num,task[j].file.filename);
                }
            }
            if(evs[i].events == EPOLLIN && evs[i].data.fd == cfd)
            {
                ret = recvCYL(cfd,&len,4);
                error_jude(ret);
                ret = recvCYL(cfd,&k,4);
                error_jude(ret);
                for(j=0;j<10;j++)
                    if(task[j].fd ==k)
                        break;
                if(len !=0)
                {
                    if(task[j].state ==1)
                    {
                        ret = recvCYL(cfd,train.buf,len);
                        error_jude(ret);
                        task[j].pos += len;
                        write(task[j].fd,train.buf,len);
                        //            printf("-nomal file :%s have download %d 本次接收 %d\n",task[j].file.filename,
                        //                   task[j].pos,len);
                        //            fflush(stdout);
                    }
                    if(task[j].state ==2)
                    {
                        ret = recvCYL(cfd,task[j].p+task[j].pos,len);
                        error_jude(ret);
                        task[j].pos += len;
                        //                        printf("\r-mmap file :%s have download %d 本次接收 %d   ",task[j].file.filename,
                        //                               task[j].pos,len);
                        //                       fflush(stdout);
                    }

                    task[j].oldtime = task[j].newtime;
                    gettimeofday(&task[j].newtime,NULL);
                    task[j].eve_len = len;
                    //                    printf("网速 :%10.2f Kb/s",(float)(task[j].eve_len*1000)/
                    //                           ((task[j].newtime.tv_sec-task[j].oldtime.tv_sec)*1000000
                    //                            +task[j].newtime.tv_usec - task[j].oldtime.tv_usec));


                }else
                {
                    printf("file %s下载完成\n",task[j].file.filename);
                    printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
                    fflush(stdout);
                    task_num--;
                    if(task_num ==0)
                    {
                        close(cfd);
                        epoll_func(epfd,cfd,EPOLL_CTL_DEL,EPOLLIN);
                        cfd = 0;
                    }
                    if(task[j].state ==2)
                        munmap(task[j].p,task[j].file.filesize);
                    close(task[j].fd);
                    bzero(task+j,sizeof(CD_info));
                }
            }
            for(j=0;j<10;j++)
            {
                if(evs[i].events == EPOLLOUT && evs[i].data.fd == uptask[j].sokectfd)
                {   
                    usleep(1200);
                    train.ctl_code = UPLOAD_OK;
                    if(uptask[j].state ==1)
                    {
                        if(uptask[j].pos<uptask[j].file.filesize)
                        {
                            ret = read(uptask[j].fd,train.buf,BUFSIZE);
                            train.Len = ret;
                            uptask[j].pos += ret;
                            ret = send(uptask[j].sokectfd,&train,8+train.Len,0);
                            if(ret == -1)
                            {
                                printf("file : %s 上传出错\n",uptask[j].file.filename);
                                del_uptask(uptask+j,epfd);
                                break;
                            }
                            uptask[j].oldtime = uptask[j].newtime;
                            gettimeofday(&uptask[j].newtime,NULL);
                            uptask[j].eve_len = train.Len;
                            //                         printf("benci fasong :%d\n",ret);
                        }else
                        {
                            train.Len =0;
                            send(uptask[j].sokectfd,&train,8+train.Len,0);
                            printf("file : %s 上传完成\n",uptask[j].file.filename);
                            printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
                            fflush(stdout);
                            del_uptask(uptask+j,epfd);
                        }
                    }
                    if(uptask[j].state ==2)
                    {
                        if(uptask[j].pos+BUFSIZE<uptask[j].file.filesize)
                        {
                            memcpy(train.buf,uptask[j].p+uptask[j].pos,BUFSIZE);
                            train.Len = BUFSIZE;
                            uptask[j].pos += BUFSIZE;
                            ret = send(uptask[j].sokectfd,&train,8+train.Len,0);
                            if(ret == -1)
                            {
                                printf("file : %s 上传出错\n",uptask[j].file.filename);
                                del_uptask(uptask+j,epfd);
                                break;
                            }
                            uptask[j].oldtime = uptask[j].newtime;
                            gettimeofday(&uptask[j].newtime,NULL);
                            uptask[j].eve_len = train.Len;
                        }else
                        {
                            memcpy(train.buf,uptask[j].p+uptask[j].pos,uptask[j].file.filesize-uptask[j].pos);
                            train.Len = uptask[j].file.filesize-uptask[j].pos;
                            uptask[j].pos = uptask[j].file.filesize;
                            ret = send(uptask[j].sokectfd,&train,8+train.Len,0);
                            if(ret == -1)
                            {
                                printf("file : %s 上传出错\n",uptask[j].file.filename);
                                del_uptask(uptask+j,epfd);
                                break;
                            }
                            train.Len =0;
                            send(uptask[j].sokectfd,&train,8+train.Len,0);
                            printf("file : %s 上传完成\n",uptask[j].file.filename);
                            printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
                            fflush(stdout);
                            del_uptask(uptask+j,epfd);
                        }
                    }
                }
            }

        }
    }
}

