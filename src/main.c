#include "../include/factory.h"
#include "../include/ser_cli.h"
#include "../include/sql.h"

int main(int argc,char *argv[])
{
    ARGS_CHECK(argc,2);
    FILE *fp = fopen(argv[1],"r+");
    ERROR_CHECK(fp,NULL,"fopen");
    int socket_fd,new_fd;
    tcpInit(&socket_fd,fp);
    fclose(fp);
    client_t client[SOCKET_NUM];
    process_data down_thread[DOWN_THREAD_NUM];
    process_data upload_thread[UP_THREAD_NUM];
    factoryInit(down_thread,upload_thread,client);
    factoryStart(down_thread,upload_thread);
    int epfd = epoll_create(1);
    struct epoll_event evs[SOCKET_NUM];
    epoll_func(epfd,socket_fd,EPOLL_CTL_ADD,EPOLLIN);
    for(int i=0;i<DOWN_THREAD_NUM;i++)
        epoll_func(epfd,down_thread[i].fd,EPOLL_CTL_ADD,EPOLLIN);
    for(int i=0;i<UP_THREAD_NUM;i++)
        epoll_func(epfd,upload_thread[i].fd,EPOLL_CTL_ADD,EPOLLIN);
    int roud=0,client_sum=0,ret;
    struct sockaddr_in clien;
    struct timeval start,end;
    Train_t train;
    Zhuce login_msg;
    QUR_msg qq_msg;
    File_info file_info;
    MQ_buf mqbuf;
    char temp[50];
//    time_t now;
//    time(&now);
//    int log_fd = open("log.txt",O_RDWR|O_CREAT|O_APPEND,0666);
//    dprintf(log_fd,"程序开始运行，当前时间\n%s\n",ctime(&now));
    gettimeofday(&start,NULL);
    MYSQL *conn;
    sql_connect(&conn);//连接数据库
    Llog(conn,"Server Start","---","---","---");
    while(1)
    {
        int ready_num = epoll_wait(epfd,evs,SOCKET_NUM,5000);
        int nn_fd ;
        gettimeofday(&end,NULL);
        // 轮盘超时，关掉所有该轮盘值的new_fd；归零除name外三大状态；
        if((end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec >5000000)
        {
            roud = (roud+1)%5;
            //printf("round = %d\n",roud);
            start = end;
            for(int j=0;j<SOCKET_NUM;j++)
            {
                if(client[j].rotate == roud)
                {
                    epoll_func(epfd,j,EPOLL_CTL_DEL,EPOLLIN);
                    close(j);
                    client[j].state = 0;
                    client[j].code = 0;
                    client[j].rotate = -1;
                    client_sum--;
                    Llog(conn,"Timeout",client[j].name,"---","Disconnect");
                }
            }
        }
        for(int i=0;i<ready_num;i++)
        {
            nn_fd = evs[i].data.fd;
            //有人连接，更新client[fd]，lunpan更新,增加监控，client数目++
            if(evs[i].events == EPOLLIN && nn_fd == socket_fd)
            {
                ret = sizeof(clien);
                new_fd = accept(socket_fd,(struct sockaddr*)&clien,&ret);
                if(client_sum >= MAX_CONNECT_NUM)//有时间再写一个缓冲队列
                    close(new_fd);
                else
                {
                    client[new_fd].state = 1;//1为已连接未登录成功状态；
                    client_sum++;
                    client[new_fd].code = 0;
                    client[new_fd].rotate = roud;
                    epoll_func(epfd,new_fd,EPOLL_CTL_ADD,EPOLLIN);
                    sprintf(client[new_fd].name,"ip:%s port:%d",
                            inet_ntoa(clien.sin_addr),ntohs(clien.sin_port));
                    Llog(conn,"Connect","---",client[new_fd].name,"Request");
                }
                continue;
            }
            //下载文件完成；
            for(int j=0;j<DOWN_THREAD_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && nn_fd == down_thread[j].fd)
                {
                    int k;
                    read(nn_fd,&ret,4);
                    for( k=0;k<DOWN_THREAD_NUM;k++)
                        if(down_thread[k].fd == nn_fd)
                            break;
                    down_thread[k].busy_num--;
 //                       printf("下载完成\n");
                }
            }
            //上传文件完成；
            for(int j=0;j<DOWN_THREAD_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && nn_fd == upload_thread[j].fd)
                {
                    int k;
                    read(nn_fd,&ret,4);
                    for( k=0;k<UP_THREAD_NUM;k++)
                        if(upload_thread[k].fd == nn_fd)
                            break;
                    upload_thread[k].busy_num--;
 //                   printf("上传完成\n");
                }
            }
            //已连接客户端提出请求
            if(client[nn_fd].state>0)
            {
                //有任何请求都更新圆盘值，若上传或者下载则从新置为-1；
                client[nn_fd].rotate = roud;
                ret = one_recv(nn_fd,&train);
                if(ret == -1)
                {
                    Llog(conn,"Client quit",client[new_fd].name,"---","Sucess");
                    close(nn_fd);
                    client[nn_fd].state = 0;
                    client[nn_fd].code = 0;
                    client[nn_fd].rotate = -1;
                    client_sum--;
                    epoll_func(epfd,nn_fd,EPOLL_CTL_DEL,EPOLLIN);
                    continue;
                }
                switch(train.ctl_code)
                {
                case LOGIN_PRE:
                    Llog(conn,"Login request",train.buf,client[new_fd].name,"Sucess");
                    client[nn_fd].state =1;
                    strcpy(client[nn_fd].name,train.buf);
                    //判断这个id是否存在；
                    ret = find_name(conn,client[nn_fd].name,NULL);
                    if(ret ==0)
                    {
                        train.Len = 0;
                        train.ctl_code = LOGIN_NO;
                        send(nn_fd,&train,8,0);
                    }
                    else
                    {
                        get_salt(train.buf);
                        train.Len = strlen(train.buf)+1;
                        train.ctl_code = LOGIN_POK;
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    break;
                case LOGIN_Q://注册已经成功；
                    memcpy(&login_msg,train.buf,train.Len);
                    //printf("name = %s\nsalt = %s\npassward = %s\n",login_msg.name,
                    //       login_msg.token,login_msg.passward);
                    add_user(conn,login_msg.name,login_msg.token,login_msg.passward);
                    Olog(conn,client[new_fd].name,"Login ","---","Sucess");
                    break;
                case REGISTER_PRE://登录请求
                    Llog(conn,"Register request",train.buf,client[new_fd].name,"Sucess");
                    client[nn_fd].state =1;
                    strcpy(client[nn_fd].name,train.buf);
                    ret = find_name(conn,client[nn_fd].name,train.buf);
                    if(ret ==-1)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else
                    {
                        train.Len = strlen(train.buf)+1;
                        train.ctl_code = REGISTER_POK;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case REGISTER_Q:
                    memcpy(&login_msg,train.buf,train.Len);
                    ret = math_user(conn,login_msg.name,
                                    login_msg.passward,login_msg.token);
                    if(ret ==-1)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                        Olog(conn,client[new_fd].name,"Register ","---","False");
                    }else//登录成功，state=2，name,round更新;
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_OK;
                        strcpy(client[nn_fd].name,login_msg.name);
                        client[nn_fd].state =2;
                        Olog(conn,client[new_fd].name,"Register ","---","Success");
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case TOKEN_PLESE:
                    memcpy(&login_msg,train.buf,train.Len);
                    Llog(conn,"Token request",login_msg.name,client[new_fd].name,"Sucess");
                    strcpy(client[nn_fd].name,login_msg.name);
                    client[nn_fd].code = atoi(login_msg.passward);
                    ret = math_token(conn,login_msg.name,login_msg.token);
                    if(ret == 0)
                    {
                        client[nn_fd].state =2;
                        Olog(conn,client[new_fd].name,"Token ","---","Success");
                    }
                    else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                        send(nn_fd,&train,8+train.Len,0);
                        Olog(conn,client[new_fd].name,"Token ","---","False");
                    }
                    break;
                case LS_Q:
                    if(client[nn_fd].state ==2)
                    {
                        Olog(conn,client[nn_fd].name,"Ls","---","Success");
                        ls_func(conn,client[nn_fd].name,client[nn_fd].code,train.buf);
                        train.ctl_code = LS_OK;
                        train.Len = strlen(train.buf)+1;
                    }else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case OPERATE_Q:
                    if(client[nn_fd].state ==2)
                    {
                        memcpy(&qq_msg,train.buf,train.Len);
                        strcpy(temp,qq_msg.buf);
                        Olog(conn,client[nn_fd].name,qq_msg.buf,qq_msg.buf1,"Request");
                        ret = operate_func(conn,&train,&qq_msg,client[nn_fd].name,&client[nn_fd].code);
                        //printf("ret = %d\n",ret);
                        if(ret == -1)
                        {
                            Olog(conn,client[nn_fd].name,temp,"---","False");
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                        }else
                            Olog(conn,client[nn_fd].name,temp,"---","Success");
                    }else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case DOWNLOAD_PRE:
                    strcpy(temp,train.buf);
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else
                    {
                        strcpy(file_info.filename,train.buf);
                        ret = find_file_info(conn,&file_info,client[nn_fd].name,client[nn_fd].code);
                        if(ret ==-1)
                        {
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                            Olog(conn,client[nn_fd].name,"Download",temp,"Request false");
                        }else
                        {
                            memcpy(train.buf,&file_info,sizeof(File_info));
                            train.Len = sizeof(File_info);
                            train.ctl_code = DOWNLOAD_POK;
                            Olog(conn,client[nn_fd].name,"Download",temp,"Request success");
                        }
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    break;
                case DOWN_MORE_PRE:
                    strcpy(temp,train.buf);
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else
                    {
                        strcpy(file_info.filename,train.buf);
                        ret = find_file_info(conn,&file_info,client[nn_fd].name,client[nn_fd].code);
                        if(ret ==-1)
                        {
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                            Olog(conn,client[nn_fd].name,"MDownload",temp,"Request false");
                        }else
                        {
                            bzero(&mqbuf,sizeof(MQ_buf));
                            memcpy(&mqbuf.file,&file_info,sizeof(File_info));
                            ret = get_mqbuf(conn,&mqbuf);
                            if(ret ==0)
                            {
                                Olog(conn,client[nn_fd].name,"MDownload",temp,"Request success");
                                train.Len = sizeof(MQ_buf);
                                train.ctl_code = DOWN_MORE_POK;
                                memcpy(train.buf,&mqbuf,train.Len);
                            }else
                            {
                                train.Len = 0;
                                train.ctl_code = OPERATE_NO;
                                Olog(conn,client[nn_fd].name,"MDownload",temp,"Request false");
                            }
                        }
                        printf("ret = %d\n",ret);
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    break;
                case DOWNLOAD_Q:
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else//准备移交权限;
                    {
                        ret =math_task(down_thread,DOWN_THREAD_NUM,AVG_CLIENT_NUM);
                        if(ret == -1)
                        {
                            train.Len = 0;
                            //应该再加个控制码表示服务器繁忙；
                            train.ctl_code = OPERATE_NO;
                            send(nn_fd,&train,8+train.Len,0);
                            break;
                        }
                        //任务转移；
                        *temp = 0;
                        sprintf(temp,"%s download thread:%d",temp,ret);
                        Olog(conn,client[nn_fd].name,"Download move",temp,"Success");
     //                   printf("下载zuanyi\n");
                        write(down_thread[ret].fd,&nn_fd,4);
                        down_thread[ret].busy_num++;
                        epoll_func(epfd,nn_fd,EPOLL_CTL_DEL,EPOLLIN);
                        client[nn_fd].state = 0;
                        client[nn_fd].code = 0;
                        client[nn_fd].rotate = -1;
                        client_sum--;
                        break;
                    }
                case UPLOAD_PRE:
                    strcpy(temp,train.buf);
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else
                    {
                        //-1;失败;0可以上传;1上传成功
                        memcpy(&file_info,train.buf,train.Len);
                        ret = math_uoload(conn,&file_info,client[nn_fd].name,client[nn_fd].code);
//                        printf("main ret = %d \n",ret);
                        if(ret == -1)
                        {
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                            Olog(conn,client[nn_fd].name,"Upload",temp,"Request false");
                        }else if(ret == 1)
                        {
                            train.ctl_code = UPLOAD_OK;
                            Olog(conn,client[nn_fd].name,"Upload",temp,"Upload sucess");
                        }else
                        {
                            train.ctl_code = UPLOAD_POK;
                            Olog(conn,client[nn_fd].name,"Upload",temp,"Request sucess");
                        }
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    break;
                case UPLOAD_Q:
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else//准备移交权限;
                    {
                        ret =math_task(upload_thread,UP_THREAD_NUM,UP_TASK_NUM-1);
                        if(ret == -1)
                        {
                            train.Len = 0;
                            //应该再加个控制码表示服务器繁忙；
                            train.ctl_code = OPERATE_NO;
                            send(nn_fd,&train,8+train.Len,0);
                            break;
                        }
                        *temp = 0;
                        sprintf(temp,"%s upload thread:%d",temp,ret);
                        Olog(conn,client[nn_fd].name,"Upload move",temp,"Success");
                        //任务转移；
                        write(upload_thread[ret].fd,&nn_fd,4);
//                        printf("上传任务转移\n");
                        upload_thread[ret].busy_num++;
                        epoll_func(epfd,nn_fd,EPOLL_CTL_DEL,EPOLLIN);
                        client[nn_fd].state = 0;
                        client[nn_fd].code = 0;
                        client[nn_fd].rotate = -1;
                        client_sum--;
                        break;
                    }
                }
            }
        }
    }
    return 0;
}
